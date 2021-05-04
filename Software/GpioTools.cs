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
            foreach (var item in desc.Inputs)
            {
                dgdInputs.Rows.Add(item);
            }
            foreach (var item in desc.Outputs)
            {
                dgdOutputs.Rows.Add(item);
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
                item.HeaderCell.Value = item.Index;
            }
            foreach (DataGridViewRow item in dgdOutputs.Rows)
            {
                item.HeaderCell.Value = item.Index;
            }
            Loaded = true;
        }

        private void dgdOutputs_CellValueChanged(object sender, DataGridViewCellEventArgs e)
        {
            if (!Loaded) return;
            if (e.RowIndex < 0) return;
            if (!(dgdOutputs.Rows.Count > e.RowIndex)) return;
            Program.clsControl.SetGpioOutput(e.RowIndex, (bool)dgdOutputs[0, e.RowIndex].Value);
        }

        private void dgdInputs_CellValueChanged(object sender, DataGridViewCellEventArgs e)
        {

        }

        private void GpioTools_FormClosing(object sender, FormClosingEventArgs e)
        {
            e.Cancel = true;
            Hide();
        }
    }
}
