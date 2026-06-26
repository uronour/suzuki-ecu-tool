using win_app.Services;

namespace win_app;

public class MainForm : Form
{
    private readonly TcpConnection _tcp = new();
    private readonly EcuData _data = new();
    private readonly CsvLogger _logger = new();
    private readonly System.Windows.Forms.Timer _pollTimer = new();
    private readonly System.Windows.Forms.Timer _refreshTimer = new();

    // Connection controls
    private TextBox _hostBox, _portBox;
    private Button _connectBtn;
    private Label _connStatus;

    // Tabs
    private TabControl _tabs;
    private Panel _dashPanel;
    private ListView _liveGrid;
    private ListBox _dtcList;
    private TextBox _ecuInfoBox;
    private Panel _logPanel;

    // Dashboard gauges (simple progress bars)
    private ProgressBar _rpmBar, _spdBar;
    private Label _rpmLabel, _spdLabel;
    private Label _gearLabel, _coolLabel, _batLabel, _mapLabel, _iatLabel, _o2Label;
    private Label[] _indicatorLabels;

    // ECU Tools
    private Button _dealerOnBtn, _dealerOffBtn, _readDtcBtn, _clearDtcBtn, _getInfoBtn;

    // Logging
    private Button _logStartBtn, _logStopBtn;
    private Label _logStatus;
    private SaveFileDialog _logSaveDlg;

    public MainForm()
    {
        Text = "Suzuki ECU Tool - Windows Companion";
        Size = new Size(900, 650);
        StartPosition = FormStartPosition.CenterScreen;
        BackColor = Color.FromArgb(30, 30, 30);
        ForeColor = Color.White;
        Font = new Font("Segoe UI", 10);

        BuildConnectionBar();
        BuildTabs();

        _pollTimer.Interval = 1000;
        _pollTimer.Tick += (_, _) => { if (_tcp.Connected) _tcp.Send("status"); };

        _refreshTimer.Interval = 200;
        _refreshTimer.Tick += (_, _) => ProcessRx();

        _tcp.ConnectionChanged += connected =>
        {
            Invoke(() =>
            {
                _connStatus.Text = connected ? "Connected" : "Disconnected";
                _connStatus.ForeColor = connected ? Color.Lime : Color.Red;
                _connectBtn.Text = connected ? "Disconnect" : "Connect";
                if (connected) _pollTimer.Start(); else _pollTimer.Stop();
            });
        };

        _tcp.LineReceived += line =>
        {
            Invoke(() =>
            {
                if (line.StartsWith("status ")) { _data.ParseStatus(line); if (_logger.IsLogging) _logger.Log(_data); UpdateLiveGrid(); UpdateDashboard(); }
                else if (line.StartsWith("info ")) { _data.ParseInfo(line); ShowEcuInfo(); }
                else if (line.StartsWith("dtc ")) { _data.ParseDtc(line); ShowDtcList(); }
                else if (line == "pong") { /* keepalive */ }
                else if (line.StartsWith("ok ") || line.StartsWith("error ") || line.StartsWith("stopped")) { /* ack */ }
            });
        };
    }

    private void BuildConnectionBar()
    {
        var panel = new Panel { Dock = DockStyle.Top, Height = 40, BackColor = Color.FromArgb(20, 20, 20) };

        panel.Controls.Add(new Label { Text = "Host:", Location = new Point(10, 10), Size = new Size(40, 24), ForeColor = Color.Gray });
        _hostBox = new TextBox { Text = "192.168.1.200", Location = new Point(50, 8), Size = new Size(120, 24), BackColor = Color.FromArgb(50, 50, 50), ForeColor = Color.White, BorderStyle = BorderStyle.FixedSingle };
        panel.Controls.Add(_hostBox);

        panel.Controls.Add(new Label { Text = "Port:", Location = new Point(180, 10), Size = new Size(40, 24), ForeColor = Color.Gray });
        _portBox = new TextBox { Text = "8899", Location = new Point(215, 8), Size = new Size(60, 24), BackColor = Color.FromArgb(50, 50, 50), ForeColor = Color.White, BorderStyle = BorderStyle.FixedSingle };
        panel.Controls.Add(_portBox);

        _connectBtn = new Button { Text = "Connect", Location = new Point(290, 7), Size = new Size(90, 28), BackColor = Color.FromArgb(60, 60, 60), ForeColor = Color.White, FlatStyle = FlatStyle.Flat };
        _connectBtn.Click += (_, _) => { if (_tcp.Connected) _tcp.Disconnect(); else { _tcp.Host = _hostBox.Text; _tcp.Port = int.Parse(_portBox.Text); _tcp.Connect(); } };
        panel.Controls.Add(_connectBtn);

        _connStatus = new Label { Text = "Disconnected", Location = new Point(390, 10), Size = new Size(120, 24), ForeColor = Color.Red };
        panel.Controls.Add(_connStatus);

        var title = new Label { Text = "Suzuki ECU Tool v1.0", Location = new Point(600, 10), Size = new Size(250, 24), ForeColor = Color.FromArgb(0, 180, 255), TextAlign = ContentAlignment.MiddleRight };
        panel.Controls.Add(title);

        Controls.Add(panel);
    }

    private void BuildTabs()
    {
        _tabs = new TabControl { Dock = DockStyle.Fill, BackColor = Color.FromArgb(30, 30, 30), ForeColor = Color.White };

        _tabs.TabPages.Add(BuildDashTab());
        _tabs.TabPages.Add(BuildLiveTab());
        _tabs.TabPages.Add(BuildDtcTab());
        _tabs.TabPages.Add(BuildEcuTab());
        _tabs.TabPages.Add(BuildLogTab());

        Controls.Add(_tabs);
    }

    private void BuildDashPanel(Control parent)
    {
        _dashPanel = new Panel { Dock = DockStyle.Fill, BackColor = Color.FromArgb(30, 30, 30) };

        // RPM gauge
        _dashPanel.Controls.Add(new Label { Text = "RPM", Location = new Point(30, 15), Size = new Size(60, 20), ForeColor = Color.Gray, Font = new Font("Segoe UI", 10, FontStyle.Bold) });
        _rpmBar = new ProgressBar { Location = new Point(30, 38), Size = new Size(380, 28), Minimum = 0, Maximum = 15000, Style = ProgressBarStyle.Continuous, ForeColor = Color.Lime };
        _rpmLabel = new Label { Text = "0 RPM", Location = new Point(420, 38), Size = new Size(100, 28), ForeColor = Color.White, Font = new Font("Segoe UI", 14, FontStyle.Bold), TextAlign = ContentAlignment.MiddleLeft };
        _dashPanel.Controls.Add(_rpmBar);
        _dashPanel.Controls.Add(_rpmLabel);

        // Speed gauge
        _dashPanel.Controls.Add(new Label { Text = "Speed", Location = new Point(30, 75), Size = new Size(60, 20), ForeColor = Color.Gray, Font = new Font("Segoe UI", 10, FontStyle.Bold) });
        _spdBar = new ProgressBar { Location = new Point(30, 98), Size = new Size(380, 28), Minimum = 0, Maximum = 300, Style = ProgressBarStyle.Continuous, ForeColor = Color.Cyan };
        _spdLabel = new Label { Text = "0 km/h", Location = new Point(420, 98), Size = new Size(100, 28), ForeColor = Color.White, Font = new Font("Segoe UI", 14, FontStyle.Bold), TextAlign = ContentAlignment.MiddleLeft };
        _dashPanel.Controls.Add(_spdBar);
        _dashPanel.Controls.Add(_spdLabel);

        // Key values row
        int y = 145;
        AddDashValue(_dashPanel, "Gear", ref _gearLabel, 30, y);
        AddDashValue(_dashPanel, "Coolant", ref _coolLabel, 170, y);
        AddDashValue(_dashPanel, "Battery", ref _batLabel, 310, y);
        y += 55;
        AddDashValue(_dashPanel, "MAP", ref _mapLabel, 30, y);
        AddDashValue(_dashPanel, "IAT", ref _iatLabel, 170, y);
        AddDashValue(_dashPanel, "O2", ref _o2Label, 310, y);

        // Indicator lights
        y += 50;
        _dashPanel.Controls.Add(new Label { Text = "Indicators:", Location = new Point(30, y), Size = new Size(100, 20), ForeColor = Color.Gray });
        y += 25;
        string[] indNames = { "N", "FAN", "CLU", "MIL" };
        _indicatorLabels = new Label[indNames.Length];
        for (int i = 0; i < indNames.Length; i++)
        {
            var lbl = new Label
            {
                Text = indNames[i],
                Location = new Point(30 + i * 90, y),
                Size = new Size(75, 28),
                BackColor = Color.FromArgb(50, 50, 50),
                ForeColor = Color.Gray,
                TextAlign = ContentAlignment.MiddleCenter,
                Font = new Font("Consolas", 12, FontStyle.Bold)
            };
            _indicatorLabels[i] = lbl;
            _dashPanel.Controls.Add(lbl);
        }

        parent.Controls.Add(_dashPanel);
    }

    private void AddDashValue(Panel parent, string name, ref Label valueLabel, int x, int y)
    {
        parent.Controls.Add(new Label { Text = name, Location = new Point(x, y), Size = new Size(60, 20), ForeColor = Color.Gray });
        valueLabel = new Label { Text = "---", Location = new Point(x, y + 20), Size = new Size(120, 24), ForeColor = Color.White, Font = new Font("Segoe UI", 12, FontStyle.Bold) };
        parent.Controls.Add(valueLabel);
    }

    private TabPage BuildDashTab()
    {
        var page = new TabPage("Dashboard");
        BuildDashPanel(page);
        return page;
    }

    private TabPage BuildLiveTab()
    {
        var page = new TabPage("Live Data");
        _liveGrid = new ListView
        {
            Dock = DockStyle.Fill,
            View = View.Details,
            BackColor = Color.FromArgb(40, 40, 40),
            ForeColor = Color.White,
            HeaderStyle = ColumnHeaderStyle.Nonclickable,
            GridLines = true
        };
        _liveGrid.Columns.Add("Sensor", 140);
        _liveGrid.Columns.Add("Value", 100);
        _liveGrid.Columns.Add("Unit", 80);

        string[] sensors = {
            "Engine RPM","Speed","Coolant Temp","Intake Air Temp","Throttle Position",
            "Battery Voltage","Gear Position","MAP","O2 Sensor","STPS",
            "Injector Pulse","Ignition Timing","IAC Steps","Barometric Press",
            "Neutral","Clutch","Fan"
        };
        foreach (var s in sensors)
            _liveGrid.Items.Add(new ListViewItem(new[] { s, "---", "" }));

        page.Controls.Add(_liveGrid);
        return page;
    }

    private TabPage BuildDtcTab()
    {
        var page = new TabPage("DTCs");
        var topPanel = new Panel { Dock = DockStyle.Top, Height = 40, BackColor = Color.FromArgb(30, 30, 30) };

        _readDtcBtn = new Button { Text = "Read DTCs", Location = new Point(10, 8), Size = new Size(100, 28), BackColor = Color.FromArgb(60, 60, 60), ForeColor = Color.White, FlatStyle = FlatStyle.Flat };
        _readDtcBtn.Click += (_, _) => _tcp.Send("dtc");
        topPanel.Controls.Add(_readDtcBtn);

        _clearDtcBtn = new Button { Text = "Clear DTCs", Location = new Point(120, 8), Size = new Size(100, 28), BackColor = Color.FromArgb(60, 60, 60), ForeColor = Color.White, FlatStyle = FlatStyle.Flat };
        topPanel.Controls.Add(_clearDtcBtn);

        _dtcList = new ListBox
        {
            Dock = DockStyle.Fill,
            BackColor = Color.FromArgb(40, 40, 40),
            ForeColor = Color.White,
            Font = new Font("Consolas", 11)
        };
        page.Controls.Add(_dtcList);
        page.Controls.Add(topPanel);
        return page;
    }

    private TabPage BuildEcuTab()
    {
        var page = new TabPage("ECU Tools");
        var topPanel = new Panel { Dock = DockStyle.Top, Height = 45, BackColor = Color.FromArgb(30, 30, 30) };

        _getInfoBtn = new Button { Text = "Get ECU Info", Location = new Point(10, 10), Size = new Size(110, 28), BackColor = Color.FromArgb(60, 60, 60), ForeColor = Color.White, FlatStyle = FlatStyle.Flat };
        _getInfoBtn.Click += (_, _) => _tcp.Send("info");
        topPanel.Controls.Add(_getInfoBtn);

        _dealerOnBtn = new Button { Text = "Dealer ON", Location = new Point(130, 10), Size = new Size(90, 28), BackColor = Color.FromArgb(60, 60, 60), ForeColor = Color.White, FlatStyle = FlatStyle.Flat };
        _dealerOnBtn.Click += (_, _) => _tcp.Send("dealer_on");
        topPanel.Controls.Add(_dealerOnBtn);

        _dealerOffBtn = new Button { Text = "Dealer OFF", Location = new Point(230, 10), Size = new Size(90, 28), BackColor = Color.FromArgb(60, 60, 60), ForeColor = Color.White, FlatStyle = FlatStyle.Flat };
        _dealerOffBtn.Click += (_, _) => _tcp.Send("dealer_off");
        topPanel.Controls.Add(_dealerOffBtn);

        _ecuInfoBox = new TextBox
        {
            Dock = DockStyle.Fill,
            Multiline = true,
            ReadOnly = true,
            BackColor = Color.FromArgb(40, 40, 40),
            ForeColor = Color.White,
            Font = new Font("Consolas", 11),
            BorderStyle = BorderStyle.None
        };
        page.Controls.Add(_ecuInfoBox);
        page.Controls.Add(topPanel);
        return page;
    }

    private TabPage BuildLogTab()
    {
        var page = new TabPage("Data Logging");
        var topPanel = new Panel { Dock = DockStyle.Top, Height = 45, BackColor = Color.FromArgb(30, 30, 30) };

        _logStartBtn = new Button { Text = "Start Logging", Location = new Point(10, 10), Size = new Size(110, 28), BackColor = Color.FromArgb(60, 60, 60), ForeColor = Color.White, FlatStyle = FlatStyle.Flat };
        _logStartBtn.Click += (_, _) =>
        {
            if (_logSaveDlg == null)
                _logSaveDlg = new SaveFileDialog { Filter = "CSV Files|*.csv", FileName = $"suzuki_log_{DateTime.Now:yyyyMMdd_HHmmss}.csv" };
            if (_logSaveDlg.ShowDialog() == DialogResult.OK && _logger.Start(_logSaveDlg.FileName))
            {
                _logStatus.Text = $"Logging to: {Path.GetFileName(_logSaveDlg.FileName)}";
                _logStatus.ForeColor = Color.Lime;
                _logStartBtn.Enabled = false;
                _logStopBtn.Enabled = true;
            }
        };
        topPanel.Controls.Add(_logStartBtn);

        _logStopBtn = new Button { Text = "Stop Logging", Location = new Point(130, 10), Size = new Size(110, 28), BackColor = Color.FromArgb(60, 60, 60), ForeColor = Color.White, FlatStyle = FlatStyle.Flat, Enabled = false };
        _logStopBtn.Click += (_, _) =>
        {
            _logger.Stop();
            _logStatus.Text = "Logging stopped";
            _logStatus.ForeColor = Color.Yellow;
            _logStartBtn.Enabled = true;
            _logStopBtn.Enabled = false;
        };
        topPanel.Controls.Add(_logStopBtn);

        _logStatus = new Label { Text = "Not logging", Location = new Point(260, 12), Size = new Size(400, 24), ForeColor = Color.Gray };
        topPanel.Controls.Add(_logStatus);

        var infoLabel = new Label
        {
            Text = "Data logging saves all sensor values to a CSV file at 1-second intervals.\nUse this for dyno runs, troubleshooting, or performance analysis.",
            Location = new Point(10, 50),
            Size = new Size(800, 40),
            ForeColor = Color.Gray
        };
        page.Controls.Add(infoLabel);
        page.Controls.Add(topPanel);
        return page;
    }

    private void ProcessRx()
    {
        while (_tcp.RxQueueSize > 0)
        {
            // processed via LineReceived callback
        }
    }

    private void UpdateDashboard()
    {
        _rpmBar.Value = Math.Min((int)_data.Rpm, 15000);
        _rpmLabel.Text = $"{_data.Rpm} RPM";
        _spdBar.Value = Math.Min((int)_data.Speed, 300);
        _spdLabel.Text = $"{_data.Speed} km/h";

        _gearLabel.Text = _data.GearPos == 0 ? "N" : _data.GearPos.ToString();
        _coolLabel.Text = $"{_data.CoolantTemp}°C";
        _batLabel.Text = $"{_data.BatteryVolt / 10.0:F1}V";
        _mapLabel.Text = $"{_data.MapKpa} kPa";
        _iatLabel.Text = $"{_data.IntakeAirTemp}°C";
        _o2Label.Text = $"{_data.O2Sensor / 10.0:F2}V";

        // RPM bar color zones
        _rpmBar.ForeColor = _data.Rpm < 8000 ? Color.Lime : _data.Rpm < 12000 ? Color.Orange : Color.Red;

        // Indicator lights
        SetIndicator(0, _data.Neutral, Color.Yellow);
        SetIndicator(1, _data.FanOn, Color.Cyan);
        SetIndicator(2, _data.ClutchIn, Color.Orange);
        // MIL: placeholder for future DTC-based MIL
    }

    private void SetIndicator(int idx, bool active, Color activeColor)
    {
        if (idx < 0 || idx >= _indicatorLabels.Length) return;
        _indicatorLabels[idx].BackColor = active ? activeColor : Color.FromArgb(50, 50, 50);
        _indicatorLabels[idx].ForeColor = active ? Color.Black : Color.Gray;
    }

    private void UpdateLiveGrid()
    {
        if (_liveGrid.Items.Count < 17) return;
        var i = _liveGrid.Items;
        i[0].SubItems[1].Text = _data.Rpm.ToString();     i[0].SubItems[2].Text = "RPM";
        i[1].SubItems[1].Text = _data.Speed.ToString();   i[1].SubItems[2].Text = "km/h";
        i[2].SubItems[1].Text = $"{_data.CoolantTemp}";    i[2].SubItems[2].Text = "°C";
        i[3].SubItems[1].Text = $"{_data.IntakeAirTemp}";  i[3].SubItems[2].Text = "°C";
        i[4].SubItems[1].Text = $"{_data.ThrottlePos / 10.0:F1}"; i[4].SubItems[2].Text = "%";
        i[5].SubItems[1].Text = $"{_data.BatteryVolt / 10.0:F1}"; i[5].SubItems[2].Text = "V";
        i[6].SubItems[1].Text = _data.GearPos == 0 ? "N" : $"G{_data.GearPos}"; i[6].SubItems[2].Text = "";
        i[7].SubItems[1].Text = $"{_data.MapKpa}";         i[7].SubItems[2].Text = "kPa";
        i[8].SubItems[1].Text = $"{_data.O2Sensor / 10.0:F2}"; i[8].SubItems[2].Text = "V";
        i[9].SubItems[1].Text = $"{_data.Stps / 10.0:F1}";  i[9].SubItems[2].Text = "%";
        i[10].SubItems[1].Text = _data.InjectorPulse.ToString(); i[10].SubItems[2].Text = "ms";
        i[11].SubItems[1].Text = $"{_data.IgnitionTiming}"; i[11].SubItems[2].Text = "°BTDC";
        i[12].SubItems[1].Text = _data.IacStep.ToString();  i[12].SubItems[2].Text = "steps";
        i[13].SubItems[1].Text = $"{_data.BaroKpa}";        i[13].SubItems[2].Text = "kPa";
        i[14].SubItems[1].Text = _data.Neutral ? "ON" : "OFF"; i[14].SubItems[2].Text = "";
        i[15].SubItems[1].Text = _data.ClutchIn ? "IN" : "OUT"; i[15].SubItems[2].Text = "";
        i[16].SubItems[1].Text = _data.FanOn ? "ON" : "OFF";   i[16].SubItems[2].Text = "";
    }

    private void ShowEcuInfo()
    {
        _ecuInfoBox.Text = $"VIN:          {_data.Vin}\r\n" +
                           $"Flash Size:   {_data.FlashSize} bytes\r\n" +
                           $"Cal Offset:   0x{_data.CalOffset:X8}\r\n" +
                           $"Cal Size:     {_data.CalSize} bytes";
    }

    private void ShowDtcList()
    {
        _dtcList.Items.Clear();
        if (_data.DtcCodes.Count == 0)
        {
            _dtcList.Items.Add("No DTCs stored");
            return;
        }
        foreach (var code in _data.DtcCodes)
            _dtcList.Items.Add($"DTC {code:X2}");
    }

    protected override void OnFormClosed(FormClosedEventArgs e)
    {
        _pollTimer.Stop();
        _refreshTimer.Stop();
        _logger.Stop();
        _tcp.Disconnect();
        base.OnFormClosed(e);
    }
}
