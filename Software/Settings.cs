using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;

namespace LabPID
{
    public partial class Settings : Form
    {
        private IEnumerable<SeriesChartType> types = Enum.GetValues(typeof(SeriesChartType)).Cast<SeriesChartType>();

        public Settings()
        {
            InitializeComponent();
        }
            
        private void Settings_Load(object sender, EventArgs e)
        {
            pctColor1.BackColor = Properties.Settings.Default.Color1;
            pctColor2.BackColor = Properties.Settings.Default.Color2;
            pctColor3.BackColor = Properties.Settings.Default.Color3;
            pctColor4.BackColor = Properties.Settings.Default.Color4;
            lblPath.Text = Properties.Settings.Default.LogPath;
            foreach (SeriesChartType item in types)
            {
                comboBox1.Items.Add(item.ToString());
            }
            comboBox1.SelectedItem = Properties.Settings.Default.ChartType.ToString();
        }

        public void AmbientUpdate()
        {
            mtbAmbient.Text = Program.clsControl.DefaultAmbient.ToString(Program.ShortFloatFormat);
            pctAmbient.Image = null;
            pctAmbient.Tag = null;
            toolTip1.RemoveAll();
        }

        private void btnColor1_Click(object sender, EventArgs e)
        {
            colorDialog1.Color = pctColor1.BackColor;
            if (colorDialog1.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                pctColor1.BackColor = colorDialog1.Color;
            }
        }

        private void btnColor2_Click(object sender, EventArgs e)
        {
            colorDialog1.Color = pctColor2.BackColor;
            if (colorDialog1.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                pctColor2.BackColor = colorDialog1.Color;
            }
        }

        private void btnColor3_Click(object sender, EventArgs e)
        {
            colorDialog1.Color = pctColor3.BackColor;
            if (colorDialog1.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                pctColor3.BackColor = colorDialog1.Color;
            }
        }

        private void btnPath_Click(object sender, EventArgs e)
        {
            folderBrowserDialog1.SelectedPath = Properties.Settings.Default.LogPath;
            if (folderBrowserDialog1.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                lblPath.Text = folderBrowserDialog1.SelectedPath;
                if (lblPath.Text.Contains(Environment.CurrentDirectory))
                {
                    lblPath.Text = lblPath.Text.Replace(Environment.CurrentDirectory, "");
                    lblPath.Text.Insert(0, lblPath.Text.StartsWith("\\") ? ".." : "..\\");
                }
            }
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            this.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.Hide();
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            Properties.Settings.Default.ChartType = types.Where(x => x.ToString() == comboBox1.SelectedItem.ToString()).FirstOrDefault();
            Properties.Settings.Default.Color1 = pctColor1.BackColor;
            Properties.Settings.Default.Color2 = pctColor2.BackColor;
            Properties.Settings.Default.Color3 = pctColor3.BackColor;
            Properties.Settings.Default.Color4 = pctColor4.BackColor;
            Properties.Settings.Default.LogPath = lblPath.Text;
            this.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.Hide();
        }

        private void Settings_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (e.CloseReason == CloseReason.UserClosing)
            {
                e.Cancel = true;
                this.Hide();
            }
        }

        private void btnColor4_Click(object sender, EventArgs e)
        {
            colorDialog1.Color = pctColor4.BackColor;
            if (colorDialog1.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                pctColor4.BackColor = colorDialog1.Color;
            }
        }

        private void pctAmbient_Click(object sender, EventArgs e)
        {
            if ((pctAmbient.Tag.ToString() == "0") && mtbAmbient.MaskCompleted)
            {
                Program.clsControl.DefaultAmbient = float.Parse(mtbAmbient.Text);
                pctAmbient.Image = Properties.Resources.Actions_dialog_ok_apply_icon;
                toolTip1.SetToolTip(pctAmbient, "Данные отправлены.");
                pctAmbient.Tag = "1";
            }
        }

        private void mtbAmbient_MaskInputRejected(object sender, MaskInputRejectedEventArgs e)
        {
            
        }

        private void mtbAmbient_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (mtbAmbient.MaskCompleted)
            {
                pctAmbient.Image = Properties.Resources.warning_icon;
                pctAmbient.Tag = "0";
                toolTip1.SetToolTip(pctAmbient, "По готовности, нажмите, чтобы отправить данные контроллеру.");
            }
        }
    }
}
