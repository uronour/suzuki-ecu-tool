using System.Net.Sockets;
using System.Text;

namespace win_app.Services;

public class TcpConnection : IDisposable
{
    private TcpClient? _client;
    private NetworkStream? _stream;
    private Thread? _reader;
    private readonly Queue<string> _rxQueue = new();
    private readonly object _lock = new();
    private volatile bool _running;

    public string Host { get; set; } = "192.168.1.200";
    public int Port { get; set; } = 8899;
    public bool Connected => _client?.Connected ?? false;
    public int RxQueueSize { get { lock (_lock) { return _rxQueue.Count; } } }

    public event Action<string>? LineReceived;
    public event Action<bool>? ConnectionChanged;

    public bool Connect()
    {
        try
        {
            _client?.Close();
            _client = new TcpClient();
            _client.Connect(Host, Port);
            _stream = _client.GetStream();
            _stream.ReadTimeout = 3000;
            _running = true;
            _reader = new Thread(ReaderLoop) { IsBackground = true };
            _reader.Start();
            ConnectionChanged?.Invoke(true);
            return true;
        }
        catch (Exception ex)
        {
            ConnectionChanged?.Invoke(false);
            return false;
        }
    }

    public void Disconnect()
    {
        _running = false;
        _stream?.Close();
        _client?.Close();
        ConnectionChanged?.Invoke(false);
    }

    public void Send(string cmd)
    {
        if (_stream?.CanWrite == true)
        {
            var buf = Encoding.ASCII.GetBytes(cmd + "\r\n");
            _stream.Write(buf);
        }
    }

    public string? TryDequeue()
    {
        lock (_lock)
        {
            return _rxQueue.Count > 0 ? _rxQueue.Dequeue() : null;
        }
    }

    private void ReaderLoop()
    {
        var sb = new StringBuilder();
        while (_running)
        {
            try
            {
                int b = _stream?.ReadByte() ?? -1;
                if (b < 0) { Disconnect(); break; }
                char c = (char)b;
                if (c == '\r') continue;
                if (c == '\n')
                {
                    if (sb.Length > 0)
                    {
                        var line = sb.ToString();
                        sb.Clear();
                        lock (_lock) _rxQueue.Enqueue(line);
                        LineReceived?.Invoke(line);
                    }
                }
                else
                {
                    sb.Append(c);
                }
            }
            catch { break; }
        }
    }

    public void Dispose()
    {
        Disconnect();
    }
}
