//TODO: 

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Windows.Forms;

namespace LabPID
{
	public static class Program
	{
		public static Info frmInfo;
		public static Terminal frmTerm;
		public static Settings frmSet;
		public static AboutBox frmAbout;
		public static Profile frmProfile;
		public static GpioTools frmGpioTools;
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
			clsControl = new Controller();
			clsProfile = new TemperatureProfile();
			clsLog = new Log(Log.GenerateFilename("log", true));
			frmInfo = new Info();
			frmTerm = new Terminal();
			frmSet = new Settings();
			frmAbout = new AboutBox();
			frmProfile = new Profile();
			frmGpioTools = new GpioTools(clsControl.GpioState);
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
			List<string> rules;

			public string FilePath { get; private set; }
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

			public static string GenerateFilename(string extensionNoDot, bool truncateTime = false)
			{
				return Path.Combine(Properties.Settings.Default.LogPath, 
					string.Format(truncateTime ? Properties.Settings.Default.LogFilenameFormatDate
					: Properties.Settings.Default.LogFilenameFormatFull, DateTime.Now, extensionNoDot));
			}
			public void SelectFile(string path)
			{
				if (File.Exists(path))
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
				FilePath = path;
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
	}
}