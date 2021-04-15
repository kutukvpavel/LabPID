using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.IO.Ports;
using System.Linq;
using System.Text;

//TODO: switch from quick-and-dirty approach of invoking Program.<...> to event-based one

namespace LabPID
{
    public class Controller
    {

        #region Static
        public enum TempType
        {
            CurrentChannel,
            Ambient,
            DsChannel //If available
        }
        private enum ResponseType
        {
            Log,
            DataDump,
            Error,
            DeviceInfo,
            None
        }
        public enum ModeType : byte
        {
            Normal = 0,
            Aggressive,
            Distillation,
            Manual
        }
        public enum PidCoeff : byte
        {
            Proportional = 0,
            Integral,
            Differential
        }
        private static readonly Dictionary<ResponseType, string> _ResponseKeyword = new Dictionary<ResponseType, string>
            {
                { ResponseType.Log, ">L:" },
                { ResponseType.Error, ">E!" },
                { ResponseType.DataDump, ">D:" },
                { ResponseType.DeviceInfo, ">I:" }
            };
        public static readonly Dictionary<ModeType, char> ModeDesignator = new Dictionary<ModeType, char>
            {
                { ModeType.Normal, 'N' },
                { ModeType.Aggressive, 'A' },
                { ModeType.Distillation, 'D' },
                { ModeType.Manual, 'M' }
            };
        public static readonly Dictionary<PidCoeff, char> CoeffDesignator = new Dictionary<PidCoeff, char>
            {
                { PidCoeff.Proportional, 'P' },
                { PidCoeff.Integral, 'I' },
                { PidCoeff.Differential, 'D' },
            };
        public const short ChannelNumber = 3;
        #endregion

        #region Private fields
        string partialLine;
        float _Setpoint;
        float[] _Temp = new float[3]; //{Selected channel, ambient, ds18b20 channel}
        float[] _Calibration = new float[ChannelNumber]; //{channel 1, ...}
        float[][] _K = new float[][]
        {
                new float[Enum.GetValues(typeof(PidCoeff)).Length],
                new float[Enum.GetValues(typeof(PidCoeff)).Length]
        };
        float _Amplifier;
        float _Ambient;
        float _ExtraPower;
        float _Integral;
        float _Overshoot;
        float _RampStep;
        short _Power;
        short _CurrentLineIndex = 0;
        ModeType _Mode;
        byte _Channel;
        bool _IsConnected = false;
        bool _Logging = false;
        bool[] _Average = new bool[ChannelNumber];
        bool _Error = false;
        bool _CJC = false;
        ResponseType _CurrentResponseType = ResponseType.None;

        CommPort devSerial;
        #endregion

        public Controller()
        {
            devSerial = CommPort.Instance;
            devSerial.DataReceived += Receive;
            devSerial.StatusChanged += StatusChange;
        }

        #region Properties
        public float this[TempType type]
        {
            get { return Temp[(int)type]; }
        }
        public string[] GetPorts()
        {
            return devSerial.GetAvailablePorts();
        }
        public float Setpoint
        {
            get { return _Setpoint; }
            set
            {
                if (value == _Setpoint) return;
                _Setpoint = value;
                Send('0', value);
            }
        }
        public float[] Temp
        {
            get { return _Temp; }
        }
        public float CurrentCalibration
        {
            get { return _Calibration[_Channel]; }
            set
            {
                if (value == _Calibration[_Channel]) return;
                _Calibration[_Channel] = value;
                Send('C', value);
            }
        }
        public float[] Calibrations
        {
            get { return _Calibration; }
        }
        public float[][] K
        {
            get { return _K; }
        }
        public float this[ModeType mode, PidCoeff coeff]
        {
            get { return K[(int)mode][(int)coeff]; }
            set
            {
                if (value == _K[(int)mode][(int)coeff]) return;
                _K[(int)mode][(int)coeff] = value;
                char cmd = (mode == ModeType.Aggressive ? CoeffDesignator[coeff] : (char)((int)coeff + 1 + '0'));
                Send(cmd, value);
            }
        }
        public float Amplifier
        {
            get { return _Amplifier; }
            set
            {
                if (value == _Amplifier) return;
                _Amplifier = value;
                Send('4', value);
            }
        }
        public float DefaultAmbient
        {
            get { return _Ambient; }
            set
            {
                if (value == _Ambient) return;
                _Ambient = value;
                Send('T', value);
            }
        }
        public float Overshoot
        {
            get { return _Overshoot; }
            set
            {
                if (value == _Overshoot) return;
                _Overshoot = value;
                Send('O', value);
            }
        }
        public float ExtraPower
        {
            get { return _ExtraPower; }
            set
            {
                if (value == _ExtraPower) return;
                _ExtraPower = value;
                Send('B', value);
            }
        }
        public float Integral
        {
            get { return _Integral; }
            set
            {
                if (value == _Integral) return;
                _Integral = value;
                Send('5', value);
            }
        }
        public float RampStepLimit
        {
            get { return _RampStep; }
            set
            {
                if (value == _RampStep) return;
                _RampStep = value;
                Send('R', value);
            }
        }
        public short Power
        {
            get { return _Power; }
            set
            {
                if (value == _Power) return;
                _Power = value;
                Send('W', value);
            }
        }
        public ModeType Mode
        {
            get { return _Mode; }
            set
            {
                if (value == _Mode) return;
                _Mode = value;
                Send('A', (float)_Mode);
            }
        }
        public byte Channel
        {
            get { return _Channel; }
            set
            {
                if (value == _Channel) return;
                _Channel = value;
                Send('7', value);
            }
        }
        public bool IsPortOpen
        {
            get { return devSerial.IsOpen; }
        }
        public bool IsConnected
        {
            get { return _IsConnected; }
        }
        public bool Logging
        {
            get { return _Logging; }
            set
            {
                if (value == _Logging) return;
                _Logging = value;
                Send('9', value ? 1f : 0f);
            }
        }
        public bool IsBusy
        {
            get { return _CurrentResponseType != ResponseType.None; }
        }
        public bool Error
        {
            get { return _Error; }
        }
        public bool CurrentAveraging
        {
            get { return _Average[_Channel]; }
            set
            {
                if (value == _Average[_Channel]) return;
                _Average[_Channel] = value;
                Send('6', value ? 1f : 0f);
            }
        }
        public bool EnableCjc
        {
            get { return _CJC; }
            set
            {
                if (value == _CJC) return;
                _CJC = value;
                Send('J', value ? 1f : 0f);
            }
        }
        public GpioDescriptor GpioState { get; } = new GpioDescriptor();
        public bool[] Averages
        {
            get { return _Average; }
        }
        #endregion

        #region Public methods
        public void SetGpioOutput(int index, bool value)
        {
            byte code = (byte)(index);
            if (value) code |= 1 << 7;
            Send('G', code);
        }
        public void Update()
        {
            Send('8', 0f);
        }
        public void SendCustom(string str)
        {
            if (devSerial.IsOpen && (!IsBusy))
            {
                devSerial.Send(str);
                Program.frmTerm.AppendLine(str, true);
            }
            else
            {
                Program.frmTerm.SetStatus("Порт закрыт или занят.");
            }
        }
        public void ClearError()
        {
            _Error = false;
        }
        public void Dispose()
        {
            if (devSerial.IsOpen)
            {
                devSerial.Close();
            }
        }
        public void OpenPort(string name)
        {
            devSerial.Dtr(false);
            if (devSerial.IsOpen)
            {
                devSerial.Close();
            }
            devSerial.Open(name, 9600, Parity.None, 8, StopBits.One, Handshake.None);
        }
        public void PollDevicePresence()
        {
            Send('V', 0f);
        }
        public void ClosePort()
        {
            if (devSerial.IsOpen)
            {
                devSerial.Close();
            }
        }
        #endregion

        #region Private methods
        private void Send(char Cmd, float Value)
        {
            if (devSerial.IsOpen)
            {
                if (IsBusy)
                {
                    Program.frmInfo.SetStatus("Контроллер занят. Попробуйте ещё раз.");
                }
                else
                {
                    string buf = Cmd + Value.ToString(Program.FloatFormat, CultureInfo.InvariantCulture);
                    devSerial.Send(buf);
                    Program.frmTerm.AppendLine(buf, true);
                }
            }
            else
            {
                Program.frmInfo.SetStatus("Порт закрыт.");
            }
        }
        private float Convert(string str)
        {
            str = str.Trim('\0', '\n', '\r');
            if (str.Contains("NAN")) return -85f;
            return float.Parse(str, CultureInfo.InvariantCulture);
        }
        private void UpdateFromLog(string line)
        {
            string[] b = line.Replace("\r\n", "").Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries).Skip(1).ToArray();
            _Setpoint = Convert(b[0]);
            _Temp[0] = Convert(b[1]);
            _Temp[1] = Convert(b[2]);
            _Power = short.Parse(b[3]);
            _Mode = ModeDesignator.First(x => x.Value == b[4][0]).Key;
            _Channel = (byte)(b[4][1] - '0');
            if (b.Length > 5)
            {
                _Temp[3] = Convert(b[5]);
                GpioState.Parse((uint)int.Parse(b[6]));
            }
        }
        private void LineReceived(string line)
        {
            if (Program.SkipLine)
            {
                Program.SkipLine = false;
                return;
            }
            line = line.Replace("\r\n", "").Trim('\0');
            if (line.Length == 0) return;
            Program.frmTerm.AppendLine(line);
            string[] b = _CurrentResponseType == ResponseType.None ? null : line.Split(' ');
            bool reset = false;
            switch (_CurrentResponseType)
            {
                case ResponseType.DataDump:
                    reset = ParseDataDumpLine(b, ++_CurrentLineIndex);
                    break;
                case ResponseType.DeviceInfo:
                    reset = ParseDeviceInfo(b, ++_CurrentLineIndex);
                    break;
                case ResponseType.None:
                    _CurrentResponseType = ProcessFirstLine(line);
                    Program.frmInfo.RefreshDisplay();
                    break;
                default:
                    break;
            }
            if (reset) ResetCurrentResponse();
        }
        private void ResetCurrentResponse()
        {
            _CurrentResponseType = ResponseType.None;
            _CurrentLineIndex = 0;
        }
        private bool ParseDeviceInfo(string[] b, short lineIndex)
        {
            switch (lineIndex)
            {
                case 1:
                    Program.frmAbout.SetFirmwareVersion(b.Last());
                    break;
                case 2:
                    Program.frmAbout.SetAdditionalInfo(string.Join(" ", b.Skip(2).ToArray()));
                    return true;
                default:
                    return true;
            }
            return false;
        }
        private bool ParseDataDumpLine(string[] b, short lineIndex)
        {
            switch (lineIndex)
            {
                case 1:
                    _Temp[0] = Convert(b[0]);
                    _Temp[1] = Convert(b[1]);
                    _Mode = ModeDesignator.First(x => x.Value == b[2][0]).Key;
                    _Channel = (byte)(b[2][1] - '0');
                    break;
                case 2:
                    for (int i = 0; i < 3; i++)
                    {
                        _K[0][i] = Convert(b[i]);
                    }
                    break;
                case 3:
                    for (int i = 0; i < 3; i++)
                    {
                        _K[1][i] = Convert(b[i]);
                    }
                    break;
                case 4:
                    _Integral = Convert(b[0]);
                    _ExtraPower = Convert(b[1]);
                    _Amplifier = Convert(b[2]);
                    break;
                case 5:
                    for (int i = 0; i < 3; i++)
                    {
                        _Calibration[i] = Convert(b[i]);
                    }
                    break;
                case 6:
                    for (int i = 0; i < 3; i++)
                    {
                        _Average[i] = (Convert(b[i]) > 0);
                    }
                    break;
                case 7:
                    _Setpoint = Convert(b[0]);
                    _Ambient = Convert(b[1]);
                    _Overshoot = Convert(b[2]);
                    break;
                case 8:
                    _RampStep = Convert(b[0]);
                    GpioState.Parse((uint)int.Parse(b[1]));
                    _CJC = (b[2][0] - '0') > 0;
                    return true;
                default:
                    return true;
            }
            return false;
        }
        private ResponseType ProcessFirstLine(string firstLine)
        {
            ResponseType t;
            try
            {
                t = _ResponseKeyword.First(x => firstLine.Contains(x.Value)).Key;
                switch (t)
                {
                    case ResponseType.Log:
                        _Logging = true;
                        UpdateFromLog(firstLine);
                        Program.frmInfo.ChartDataAdd();
                        break;
                    case ResponseType.Error:
                        _Error = true;
                        break;
                    case ResponseType.DeviceInfo:
                        _IsConnected = true;
                        break;
                    default:
                        break;
                }
                return t;
            }
            catch (InvalidOperationException) { }
            return ResponseType.None;
        }
        private string AddData(string StringOut)
        {
            // if we have a partial line, add to it.
            if (partialLine != null)
            {
                // tack it on
                partialLine += StringOut;
                return partialLine;
            }
            return StringOut;
        }
        private void Receive(string dataIn)
        {
            // if we detect a line terminator, add line to output
            int index;
            while (dataIn.Length > 0 &&
                ((index = dataIn.IndexOf("\r")) != -1 ||
                (index = dataIn.IndexOf("\n")) != -1))
            {
                string StringIn = dataIn.Substring(0, index);
                dataIn = dataIn.Remove(0, index + 1);
                LineReceived(AddData(StringIn));
                partialLine = null; // terminate partial line
            }
            // if we have data remaining, add a partial line
            if (dataIn.Length > 0)
            {
                partialLine = AddData(dataIn);
            }
        }
        private void StatusChange(string Status)
        {
            Program.frmInfo.SetStatus(Status);
            Program.frmTerm.SetStatus(Status);
        }
        #endregion
    }

    public class GpioDescriptor
    {
        #region Private

        private const int InputCount = 4;
        private const int OutputCount = 12;
        private bool HoldEvents = false;
        private void RaiseChanged(object sender, EventArgs e)
        {
            if (!HoldEvents) Changed?.Invoke(sender, e);
        }

        #endregion

        public event EventHandler Changed;

        ObservableCollection<bool> Inputs { get; }
        ObservableCollection<bool> Outputs { get; }

        public GpioDescriptor()
        {
            Inputs = new ObservableCollection<bool>(new bool[InputCount]);
            Outputs = new ObservableCollection<bool>(new bool[OutputCount]);
            Inputs.CollectionChanged += RaiseChanged;
            Outputs.CollectionChanged += RaiseChanged;
        }

        public void Parse(uint raw)
        {
            HoldEvents = true;
            for (int i = 0; i < OutputCount; i++)
            {
                Outputs[i] = (raw & (1u << i)) != 0;
            }
            for (int i = 0; i < InputCount; i++)
            {
                Inputs[i] = (raw & (1u << (i + OutputCount))) != 0;
            }
            HoldEvents = false;
            RaiseChanged(this, null);
        }
    }
}
