using System;
using System.Windows.Forms;

namespace LabPID
{
    public partial class GpioTools : Form
    {
        public GpioTools(GpioDescriptor desc)
        {
            InitializeComponent();
            Descriptor = desc;
            desc.Changed += Desc_Changed;
            for (int i = 0; i < desc.Inputs.Count; i++)
            {
                dgdInputs.Rows.Add(desc.InputLabels.ContainsKey(i) ? desc.InputLabels[i] : "", desc.Inputs[i]);
            }
            for (int i = 0; i < desc.Outputs.Count; i++)
            {
                dgdOutputs.Rows.Add(desc.OutputLabels.ContainsKey(i) ? desc.OutputLabels[i] : "", desc.Outputs[i]);
            }
        }

        private GpioDescriptor Descriptor;
        private bool Loaded = false;

        private void Desc_Changed(object sender, EventArgs e)
        {
            for (int i = 0; i < Descriptor.Inputs.Count; i++)
            {
                dgdInputs[0, i].Value = Descriptor.Inputs[i];
            }
            for (int i = 0; i < Descriptor.Outputs.Count; i++)
            {
                dgdOutputs[0, i].Value = Descriptor.Outputs[i];
            }
        }

        private void GpioTools_Load(object sender, EventArgs e)
        {
            foreach (DataGridViewRow item in dgdInputs.Rows)
            {
                item.HeaderCell.ToolTipText = item.Index.ToString();
            }
            foreach (DataGridViewRow item in dgdOutputs.Rows)
            {
                item.HeaderCell.ToolTipText = item.Index.ToString();
            }
            Loaded = true;
        }

        private void dgdOutputs_CellValueChanged(object sender, DataGridViewCellEventArgs e)
        {
            if (!Loaded) return;
            if (e.RowIndex < 0) return;
            if (!(dgdOutputs.Rows.Count > e.RowIndex)) return;
            if (e.ColumnIndex == 1)
            {
                Program.clsControl.SetGpioOutput(e.RowIndex, (bool)dgdOutputs[1, e.RowIndex].Value);
            }
            else
            {
                if (!Descriptor.OutputLabels.ContainsKey(e.RowIndex)) Descriptor.OutputLabels.Add(e.RowIndex, "");
                Descriptor.OutputLabels[e.RowIndex] = (string)dgdOutputs[0, e.RowIndex].Value;
            }
        }

        private void dgdInputs_CellValueChanged(object sender, DataGridViewCellEventArgs e)
        {
            if (!Loaded) return;
            if (e.RowIndex < 0) return;
            if (!(dgdInputs.Rows.Count > e.RowIndex)) return;
            if (e.ColumnIndex != 0) return;
            if (!Descriptor.InputLabels.ContainsKey(e.RowIndex)) Descriptor.InputLabels.Add(e.RowIndex, "");
            Descriptor.InputLabels[e.RowIndex] = (string)dgdInputs[0, e.RowIndex].Value;
        }

        private void GpioTools_FormClosing(object sender, FormClosingEventArgs e)
        {
            e.Cancel = true;
            Hide();
        }
    }
}
