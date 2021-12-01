using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;

namespace LabPID
{
    public class GpioDescriptor
    {
        #region Private

        private const int InputCount = 4;
        private const int OutputCount = 12;
        private bool HoldEvents = false;
        private void RaiseChanged(object sender, EventArgs e)
        {
            if (!HoldEvents) Changed?.Invoke(sender, new HandledEventArgs(false));
        }

        #endregion

        public event EventHandler<HandledEventArgs> Changed;

        public ObservableCollection<bool> Inputs { get; }
        public ObservableCollection<bool> Outputs { get; }

        public Dictionary<int, string> InputLabels { get; set; } = new Dictionary<int, string>()
        {
            { 0, "IN 0" },
            { 1, "IN 1" }
        };
        public Dictionary<int, string> OutputLabels { get; set; } = new Dictionary<int, string>()
        {
            { 0, "OUT 0" },
            { 1, "OUT 1" }
        };

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

        public override string ToString()
        {
            StringBuilder b = new StringBuilder((int)Math.Ceiling(
                1.25 * Inputs.Count + 1.25 * Outputs.Count + Environment.NewLine.Length));
            for (int i = 0; i < Inputs.Count; i++)
            {
                if ((i % 4 == 0) && (i != 0) && (i != (Inputs.Count - 1))) b.Append(' ');
                b.Append(Inputs[i] ? '1' : '0');
            }
            b.AppendLine();
            for (int i = 0; i < Outputs.Count; i++)
            {
                if ((i % 4 == 0) && (i != 0) && (i != (Outputs.Count - 1))) b.Append(' ');
                b.Append(Outputs[i] ? '1' : '0');
            }
            return b.ToString();
        }
    }
}
