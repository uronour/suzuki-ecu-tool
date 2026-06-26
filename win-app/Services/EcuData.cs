namespace win_app.Services;

public class EcuData
{
    public uint Rpm { get; set; }
    public uint Speed { get; set; }
    public uint CoolantTemp { get; set; }
    public uint IntakeAirTemp { get; set; }
    public uint ThrottlePos { get; set; }
    public uint BatteryVolt { get; set; }
    public uint GearPos { get; set; }
    public uint MapKpa { get; set; }
    public uint O2Sensor { get; set; }
    public uint Stps { get; set; }
    public uint InjectorPulse { get; set; }
    public uint IgnitionTiming { get; set; }
    public uint IacStep { get; set; }
    public uint BaroKpa { get; set; }
    public bool Neutral { get; set; }
    public bool ClutchIn { get; set; }
    public bool FanOn { get; set; }

    public string Vin { get; set; } = "";
    public uint FlashSize { get; set; }
    public uint CalOffset { get; set; }
    public uint CalSize { get; set; }

    public List<byte> DtcCodes { get; set; } = new();

    public void ParseStatus(string line)
    {
        // format: status rpm=...,spd=...,cool=...,...
        if (!line.StartsWith("status ")) return;
        var parts = line[7..].Split(',');
        foreach (var part in parts)
        {
            var kv = part.Split('=');
            if (kv.Length != 2) continue;
            var val = kv[1].Trim();
            switch (kv[0].Trim())
            {
                case "rpm":   Rpm = SafeParse(val); break;
                case "spd":   Speed = SafeParse(val); break;
                case "cool":  CoolantTemp = SafeParse(val); break;
                case "iat":   IntakeAirTemp = SafeParse(val); break;
                case "tps":   ThrottlePos = SafeParse(val); break;
                case "bat":   BatteryVolt = SafeParse(val); break;
                case "gear":  GearPos = SafeParse(val); break;
                case "map":   MapKpa = SafeParse(val); break;
                case "o2":    O2Sensor = SafeParse(val); break;
                case "stps":  Stps = SafeParse(val); break;
                case "inj":   InjectorPulse = SafeParse(val); break;
                case "ign":   IgnitionTiming = SafeParse(val); break;
                case "iac":   IacStep = SafeParse(val); break;
                case "baro":  BaroKpa = SafeParse(val); break;
                case "neut":  Neutral = val == "1"; break;
                case "clutch": ClutchIn = val == "1"; break;
                case "fan":   FanOn = val == "1"; break;
            }
        }
    }

    public void ParseInfo(string line)
    {
        if (!line.StartsWith("info ")) return;
        var parts = line[5..].Split(',');
        foreach (var part in parts)
        {
            var kv = part.Split('=');
            if (kv.Length != 2) continue;
            var val = kv[1].Trim();
            switch (kv[0].Trim())
            {
                case "vin":      Vin = val; break;
                case "flash":    FlashSize = SafeParse(val); break;
                case "cal_off":  CalOffset = SafeParse(val); break;
                case "cal_sz":   CalSize = SafeParse(val); break;
            }
        }
    }

    public void ParseDtc(string line)
    {
        if (!line.StartsWith("dtc ")) return;
        DtcCodes.Clear();
        if (line == "dtc none") return;
        var codes = line[4..].Split(',');
        foreach (var c in codes)
        {
            if (byte.TryParse(c.Trim(), out var b))
                DtcCodes.Add(b);
        }
    }

    private static uint SafeParse(string s) =>
        uint.TryParse(s, out var v) ? v : 0;
}
