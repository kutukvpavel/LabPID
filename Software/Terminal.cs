using System;
using System.Drawing;
using System.Windows.Forms;

namespace pid
{
    public partial class Terminal : Form
    {
        public Terminal()
        {
            InitializeComponent();
        }

        internal delegate void StringDelegate(string data);

        public void AppendLine(string Line)
        {
            if (rtbTerm.InvokeRequired)
            {
                rtbTerm.Invoke(new MethodInvoker(delegate {
                    rtbTerm.AppendText(Environment.NewLine + Line);
                }));
            }
            else
            {
                rtbTerm.AppendText(Environment.NewLine + Line);
            }
            Program.clsLog.Write(Line);
        }
        public void AppendLine(string Line, bool format)
        {
            if (format)
            {
                if (rtbTerm.InvokeRequired)
                {
                    rtbTerm.Invoke(new MethodInvoker(delegate
                    {
                        rtbTerm.DeselectAll();
                        rtbTerm.SelectionColor = Properties.Settings.Default.OutTermColor;
                    }));
                }
                else
                {
                    rtbTerm.DeselectAll();
                    rtbTerm.SelectionColor = Properties.Settings.Default.OutTermColor;
                }
            }
            AppendLine(Line);
            if (rtbTerm.InvokeRequired)
            {
                rtbTerm.Invoke(new MethodInvoker(delegate
                {
                    rtbTerm.SelectionColor = Color.Black;
                }));
            }
            else
            {
                rtbTerm.SelectionColor = Color.Black;
            }
        }

        private void Terminal_SizeChanged(object sender, EventArgs e)
        {
            txtInput.Width = toolStrip1.ClientRectangle.Width - 2 * btnSend.Width;      //  Remove this doggy stuff somehow (2*)
        }

        private void btnSend_Click(object sender, EventArgs e)
        {
            Program.clsControl.SendCustom(txtInput.Text);
            txtInput.Clear();
        }

        private void Terminal_Load(object sender, EventArgs e)
        {
            if (lblStatus.Text == "")
            {
                this.обновитьСтатусToolStripMenuItem_Click(this, null);
            }
            this.Size = Properties.Settings.Default.frmTermSize;
            this.Terminal_SizeChanged(this, null);
        }

        public void SetStatus(string status)
        {
            if (InvokeRequired)
            {
                Invoke(new StringDelegate(SetStatus), new object[] { status });
                return;
            }
            lblStatus.Text = status;
        }

        private void обновитьСтатусToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (Program.clsControl.IsPortOpen)
            {
                if (Program.clsControl.IsConnected)
                {
                    SetStatus("Подключено.");
                }
                else
                {
                    SetStatus("Нет подключения.");
                }
            }
            else
            {
                SetStatus("Порт закрыт.");
            }
        }

        private void очиститьЭкранToolStripMenuItem_Click(object sender, EventArgs e)
        {
            rtbTerm.Clear();
        }

        private void Terminal_FormClosing(object sender, FormClosingEventArgs e)
        {
            Properties.Settings.Default.frmTermSize = this.Size;
            if (e.CloseReason == CloseReason.UserClosing)
            {
                e.Cancel = true;
                this.Hide();
            }
        }

        private void toolStripSplitButton1_ButtonClick(object sender, EventArgs e)
        {
            
        }
    }
}
