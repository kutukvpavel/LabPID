//TODO: Finish GPIO implementation!!

using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;
using System.IO.Ports;
using System.IO;
using System.Text;
using System.Globalization;

namespace pid
{
	static class Program
	{
		public static Info frmInfo;
		public static Terminal frmTerm;
		public static Settings frmSet;
		public static AboutBox frmAbout;
		public static Profile frmProfile;
		public static Controller clsControl;
		public static Log clsLog;
		public static TemperatureProfile clsProfile; 
		public const string FloatFormat = "+000.0000;-000.0000";
		public const string ShortFloatFormat = "+000.00;-000.00";
		public static bool SkipLine = false;

		/// <summary>
		/// Главная точка входа для приложения.
		/// </summary>
		[STAThread]
		static void Main()
		{
			Application.EnableVisualStyles();
			Application.SetCompatibleTextRenderingDefault(false);
			frmInfo = new Info();
			frmTerm = new Terminal();
			frmSet = new Settings();
			frmAbout = new AboutBox();
			frmProfile = new Profile();
			clsControl = new Controller();
			clsProfile = new TemperatureProfile();
			clsLog = new Log(Log.GenerateFilename("log"));
			Application.Run(frmInfo);
		}

		public static IEnumerable<Control> FlattenChildren(Control control)
		{
			var children = control.Controls.Cast<Control>();
			return children.SelectMany(c => FlattenChildren(c)).Concat(children);
		}

		public class Log
		{
			public bool Enabled;
			public bool TimeStamp;
			public string _FilterInput;       //Pretty much reserved for (may be) future extended formatting capabilities
			StreamWriter stream; 
			string _FilePath;
			List<string> rules;

			public string FilePath
			{
				get { return _FilePath; }
			}
			/*
			 *  Replacement rules are separated with commas (,), first goes current string and then goes new string separated with HB (|).  
			 *  Replacement of formatting characters is supported and blank fields are supported.
			 *  Example: \r|,\n| replaces all \r and \n with a blank string (in other words, removes).
			 */
			public string FilterInput
			{
				get { return _FilterInput; }
				set {
					_FilterInput = value; 
					rules = new List<string>(FilterInput.Where(x => x == ',').Count());
					if (_FilterInput.Length > 0)
					{
						rules.Add(new string(new char[] { _FilterInput[0] }));
						short q = 0;
						for (int i = 1; i < _FilterInput.Length - 1; i++)
						{
							if (_FilterInput[i] == ',')
							{
								q++;
								if ((_FilterInput[i + 1] == ',' || _FilterInput[i - 1] == ',') && (q % 2 == 0))
								{
									rules[rules.Count - 1] = rules.Last() + ',';
								}
								else
								{
									rules.Add("");
								}
							}
							else
							{
								rules[rules.Count - 1] = rules.Last() + _FilterInput[i];
							}
						}
						rules[rules.Count - 1] = rules.Last() + _FilterInput.Last();
					}
				}
			}

			public Log(string Path)
			{
				SelectFile(Path);
				Enabled = false;
				TimeStamp = true;
				FilterInput = "";
			}

			public static string GenerateFilename(string ExtensionND)
			{
				return Path.Combine(pid.Properties.Settings.Default.LogPath, "LabPID " + DateTime.Now.ToString().Replace('/', '-').Replace(':', '-') + "." + ExtensionND);
			}
			public void SelectFile(string path)
			{
				if (File.Exists(path) && pid.Properties.Settings.Default.AppendLog)
				{
					stream = new StreamWriter(path, true);
				}
				else
				{
					string dir = Path.GetDirectoryName(path);
					if (!Directory.Exists(dir))
					{
						Directory.CreateDirectory(dir);
					}
					stream = File.CreateText(path);
				}
				_FilePath = path;
				stream.NewLine = Environment.NewLine;
			}
			public void Write(string Line)
			{
				if (Enabled)
				{            
					if (FilterInput.Length > 0)
					{
						foreach (string rule in rules)
						{
							string[] s = rule.Split('|');
							if (s.Length > 2)
							{
								if (s.First().Length > 0)
								{
									s[1] = "|";
								}
								else
								{
									s[0] = "|";
									s[1] = s[2];
								}
							}
							Line = Line.Replace(s[0], s[1]);
						}
					}
					if (TimeStamp)
					{
						Line = DateTime.Now.ToString() + " " + Line;
					}
					stream.WriteLine(Line);
				}
			}
			public void Dispose()
			{
				stream.Close();
				stream.Dispose();
			}
		}

		public class Controller
		{

			#region Static
			public enum GpioDirectionType : byte
			{
				Input = 0,
				InputPullup,
				Output
			}
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
			public static readonly Dictionary<GpioDirectionType, string> DirectionDesignator = new Dictionary<GpioDirectionType, string>
			{
				{ GpioDirectionType.Input, "I" },
				{ GpioDirectionType.InputPullup, "IP" },
				{ GpioDirectionType.Output, "O" }
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
			byte _GPIOMode = 0;
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
				set {
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
				set {
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
				set {
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
				set {
					if (value == _ExtraPower) return;
					_ExtraPower = value;
					Send('B', value);
				}
			}
			public float Integral
			{
				get { return _Integral; }
				set {
					if (value == _Integral) return;
					_Integral = value;
					Send('5', value);
				}
			}
			public float RampStepLimit
			{
				get { return _RampStep; }
				set {
					if (value == _RampStep) return;
					_RampStep = value;
					Send('R', value);
				}
			}
			public short Power
			{
				get { return _Power; }
				set {
					if (value == _Power) return;
					_Power = value;
					Send('W', value);
				}
			}
			public ModeType Mode
			{
				get { return _Mode; }
				set {
					if (value == _Mode) return;
					_Mode = value;
					Send('A', (float)_Mode);
				}
			}
			public byte Channel
			{
				get { return _Channel; }
				set {
					if (value == _Channel) return;
					_Channel = value;
					Send('7', value);
				}
			}
			public byte GpioMode
			{
				get { return _GPIOMode; }
				set
				{
					if (value == _GPIOMode) return;
					_GPIOMode = value;
					Send('G', value);
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
				set {
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
			public GpioDirectionType GpioDirection
			{
				get { return (GpioDirectionType)(_GPIOMode >> 1); }
				set
				{
					GpioDirectionType cur = (GpioDirectionType)(_GPIOMode >> 1);
					if (value == cur) return;
					_GPIOMode &= 1;
					_GPIOMode |= (byte)((int)value << 1);
					Send('G', _GPIOMode);
				}
			}
			public bool GpioState
			{
				get { return (_GPIOMode & 1u) > 0; }
				set
				{
					bool cur = (_GPIOMode & 1u) > 0;
					if (value == cur) return;
					SetGpioState(value);
					Send('G', _GPIOMode);
				}
			}
			public bool[] Averages
			{
				get { return _Average; }
			}
            #endregion

            #region Public methods
            public void CheckDevicePresent()
			{
				Send('V', 0f);
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
					frmTerm.AppendLine(str, true);
				}
				else
				{
					frmTerm.SetStatus("Порт закрыт или занят.");
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
						frmInfo.SetStatus("Контроллер занят. Попробуйте ещё раз.");
					}
					else
					{
						string buf = Cmd + Value.ToString(FloatFormat, CultureInfo.InvariantCulture);
						devSerial.Send(buf);
						frmTerm.AppendLine(buf, true);
					}
				}
				else
				{
					frmInfo.SetStatus("Порт закрыт.");
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
				string[] b = line.Replace("\r\n", "").Split(new char[] {' '}, StringSplitOptions.RemoveEmptyEntries).Skip(1).ToArray();
				_Setpoint = Convert(b[0]);
				_Temp[0] = Convert(b[1]);
				_Temp[1] = Convert(b[2]);
				_Power = short.Parse(b[3]);
				_Mode = ModeDesignator.First(x => x.Value == b[4][0]).Key;
				_Channel = (byte)(b[4][1] - '0');
				if (b.Length > 5)
				{
					_Temp[3] = Convert(b[5]);
					SetGpioState((b[6][0] - '0') > 0);
				}
			}
			private void SetGpioState(bool state)
			{
				if (state)
				{
					_GPIOMode |= 1;
				}
				else
				{
					_GPIOMode &= (byte)((~1u) & 0xFFu);
				}
			}
			private void LineReceived(string line)
			{
				if (SkipLine)
				{
					SkipLine = false;
					return;
				}
				line = line.Replace("\r\n", "").Trim('\0');
				if (line.Length == 0) return;
				frmTerm.AppendLine(line);
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
						frmInfo.RefreshDisplay();
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
						frmAbout.SetFirmwareVersion(b.Last());
						break;
					case 2:
						frmAbout.SetAdditionalInfo(string.Join(" ", b.Skip(2).ToArray()));
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
						_GPIOMode = byte.Parse(b[1]);
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
							frmInfo.ChartDataAdd();
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
					partialLine = null;	// terminate partial line
				}
				// if we have data remaining, add a partial line
				if (dataIn.Length > 0)
				{
					partialLine = AddData(dataIn);
				}
			}
			private void StatusChange(string Status)
			{
				frmInfo.SetStatus(Status);
				frmTerm.SetStatus(Status);
			}
			#endregion
		}
	}

	public class TemperatureEventArgs : EventArgs
	{
		public TemperatureEventArgs(float temperature)
		{
			Temperature = temperature;
		}

		public float Temperature { get; }
	}

	public class TemperatureProfile : SortedDictionary<int, float>   //ms, °C
	{
		public enum State
		{
			Stopped,
			Suspended,
			Running
		}

		public TemperatureProfile() : base()
		{
			InitVariables();
			_OneSecondTimer.Elapsed += OneSecondTimer_Elapsed;
		}

		public event EventHandler<TemperatureEventArgs> TimeToChangeSetpoint;
		public event EventHandler ExecutionFinished;

		public Func<Tuple<float, float>> TemperatureValidationCallback;
		public int ElapsedTime { get; private set; }                
		public State CurrentState { get; private set; } = State.Stopped;
		public string Name { get; set; } = "Default";
		public bool DelaySegmentStart { get; set; } = false;
		public int DelayTolerance { get; set; } = 5;

		private System.Timers.Timer _OneSecondTimer = new System.Timers.Timer(1000) { Enabled = false, AutoReset = true };
		private Enumerator _Enumerator;
		private bool _Reached;
		private int _Finish;  

		private void OneSecondTimer_Elapsed(object sender, System.Timers.ElapsedEventArgs e)
		{
			if (_Finish > 0)   //A little delay for the plot not to miss the last setpoint
			{
				if (_Finish++ > 3) Stop();
			}
			if (DelaySegmentStart && !_Reached)
			{
				var t = TemperatureValidationCallback.Invoke();
				_Reached = (Math.Abs(t.Item1 - t.Item2) < DelayTolerance);
				if (!_Reached) return;
			}
			if (++ElapsedTime >= _Enumerator.Current.Key)
			{
				_Reached = false;
				TimeToChangeSetpoint?.Invoke(this, new TemperatureEventArgs(this[_Enumerator.Current.Key]));
				if (!_Enumerator.MoveNext()) _Finish++;
			}
		}

		private void InitVariables()
		{
			_Finish = 0;
			_Reached = false;
			ElapsedTime = 0;
		}

		public void Start()
		{
			if (Count == 0) throw new InvalidOperationException("Can't start a profile that contains no points.");
			if (CurrentState == State.Running) throw new InvalidOperationException("The profile is already running!");
			InitVariables();
			_Enumerator = GetEnumerator();
			_Enumerator.MoveNext(); //Initially enumerator is positioned before the first element            
			_OneSecondTimer.Start();
			CurrentState = State.Running;
		}

		public void Pause()
		{
			switch (CurrentState)
			{
				case State.Stopped:
					throw new InvalidOperationException("Can't pause a stopped profile!");
				case State.Suspended:
					throw new InvalidOperationException("The profile has already been paused!");
				default:
					break;
			}
			_OneSecondTimer.Stop();
			CurrentState = State.Suspended;
		}

		public void Stop()
		{
			if (CurrentState == State.Stopped) throw new InvalidOperationException("The profile has already been stopped!");
			_OneSecondTimer.Stop();
			CurrentState = State.Stopped;
			ExecutionFinished?.Invoke(this, new EventArgs());
		}

		public new void Add(int key, float value)
		{
			if (CurrentState != State.Stopped)
			{
				throw new InvalidOperationException("Can't modify a running profile!");
			}
			base.Add(key, value);
		}

		public new void Clear()
		{
			if (CurrentState != State.Stopped)
			{
				throw new InvalidOperationException("Can't modify a running profile!");
			}
			base.Clear();
		}

		public new void Remove(int key)
		{
			if (CurrentState != State.Stopped)
			{
				throw new InvalidOperationException("Can't modify a running profile!");
			}
			base.Remove(key);
		}

		public override string ToString()
		{
			string arr = string.Join(Environment.NewLine,
				this.Select(x => string.Format(CultureInfo.InvariantCulture, "{0} {1:F2}", x.Key, x.Value)));
			StringBuilder s = new StringBuilder(arr.Length + Environment.NewLine.Length * 10 + 
				Name.Length + bool.FalseString.Length + DelayTolerance.ToString().Length);
			s.AppendLine(Name);
			s.AppendLine(DelaySegmentStart.ToString());
			s.AppendLine(DelayTolerance.ToString());
			s.Append(arr);
			return s.ToString();
		}
	}
}
																	   