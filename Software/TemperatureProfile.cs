using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;

namespace LabPID
{
    public class TemperatureEventArgs : EventArgs
    {
        public TemperatureEventArgs(float temperature)
        {
            Temperature = temperature;
        }

        public float Temperature { get; }
    }

    public class CustomCommandEventArgs : EventArgs
    {
        public CustomCommandEventArgs(string cmd)
        {
            Command = cmd;
        }

        public string Command { get; }
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

        public event EventHandler<CustomCommandEventArgs> TimeToIssueCustomCommand;
        public event EventHandler<TemperatureEventArgs> TimeToChangeSetpoint;
        public event EventHandler ExecutionFinished;

        public Dictionary<int, string> CustomCommands { get; } = new Dictionary<int, string>();
        public Func<Tuple<float, float>> TemperatureValidationCallback { get; set; }
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
                return;
            }
            if (DelaySegmentStart && !_Reached && (TemperatureValidationCallback != null))
            {
                var t = TemperatureValidationCallback.Invoke();
                _Reached = (Math.Abs(t.Item1 - t.Item2) < DelayTolerance);
                if (!_Reached) return;
            }
            if (CustomCommands.ContainsKey(ElapsedTime))
            {
                TimeToIssueCustomCommand?.Invoke(this, new CustomCommandEventArgs(CustomCommands[ElapsedTime]));
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
            _Reached = true;
            ElapsedTime = 0;
        }

        public void Start()
        {
            if (Count == 0) throw new InvalidOperationException("Can't start a profile that contains no points.");
            if (CurrentState == State.Running) throw new InvalidOperationException("The profile is already running!");
            if (CurrentState != State.Suspended)
            {
                InitVariables();
                _Enumerator = GetEnumerator();
                _Enumerator.MoveNext(); //Initially enumerator is positioned before the first element
            }
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
                this.Select(x => string.Format(CultureInfo.InvariantCulture, "{0} {1:F2}", x.Key, x.Value))
                .Concat(CustomCommands.Select(x => string.Format(CultureInfo.InvariantCulture, "{0} \"{1}\"", x.Key, x.Value))));
            string dt = DelayTolerance.ToString();
            StringBuilder s = new StringBuilder(arr.Length + Environment.NewLine.Length * (this.Count + CustomCommands.Count) +
                Name.Length + bool.FalseString.Length + dt.Length);
            s.AppendLine(Name);
            s.AppendLine(DelaySegmentStart.ToString());
            s.AppendLine(dt);
            s.Append(arr);
            return s.ToString();
        }
    }
}
