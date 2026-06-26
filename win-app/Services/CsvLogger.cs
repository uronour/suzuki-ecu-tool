using System.IO;
using System.Text;

namespace win_app.Services;

public class CsvLogger : IDisposable
{
    private StreamWriter? _writer;
    private bool _headerWritten;

    public bool IsLogging => _writer != null;
    public string? FilePath { get; private set; }

    public bool Start(string filePath)
    {
        try
        {
            _writer = new StreamWriter(filePath, false, Encoding.UTF8);
            FilePath = filePath;
            _headerWritten = false;
            return true;
        }
        catch { return false; }
    }

    public void Log(EcuData data)
    {
        if (_writer == null) return;
        var ts = DateTime.Now;
        if (!_headerWritten)
        {
            _writer.WriteLine("Timestamp,RPM,Speed,Coolant,IAT,TPS,Battery,Gear,MAP,O2,STPS,InjPulse,IgnTiming,IAC,Baro,Neutral,Clutch,Fan");
            _headerWritten = true;
        }
        _writer.WriteLine(
            $"{ts:yyyy-MM-dd HH:mm:ss.fff},{data.Rpm},{data.Speed},{data.CoolantTemp}," +
            $"{data.IntakeAirTemp},{data.ThrottlePos},{data.BatteryVolt},{data.GearPos}," +
            $"{data.MapKpa},{data.O2Sensor},{data.Stps},{data.InjectorPulse}," +
            $"{data.IgnitionTiming},{data.IacStep},{data.BaroKpa},{data.Neutral},{data.ClutchIn},{data.FanOn}");
        _writer.Flush();
    }

    public void Stop()
    {
        _writer?.Close();
        _writer = null;
        FilePath = null;
    }

    public void Dispose() => Stop();
}
