using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Globalization;

namespace LabPID
{
    public partial class Profile : Form
    {
        public Profile()
        {
            InitializeComponent();
        }

        TemperatureProfile _profile = new TemperatureProfile();
        public TemperatureProfile CurrentProfile
        {
            get { return _profile; }
            set
            {
                _profile = value;
                ParseProfileString(_profile.ToString());
            }
        }

        private bool ParseProfile()
        {
            _profile = new TemperatureProfile();
            TextReader reader = new StringReader(txtProfile.Text);
            string line = reader.ReadLine();
            try
            {
                while (line != null)
                {
                    string[] split = line.Split(' ');
                    int time;
                    if (split[0][0] == '+')
                    {
                        time = int.Parse(split[0].Remove(0, 1)) + _profile.Last().Key;
                    }
                    else
                    {
                        time = int.Parse(split[0]);
                    }
                    if (split[1][0] == '"')
                    {
                        _profile.CustomCommands.Add(time, split[1].Trim('"'));
                    }
                    else
                    {
                        _profile.Add(time, float.Parse(split[1], CultureInfo.InvariantCulture));
                    }
                    line = reader.ReadLine();
                }
                _profile.Name = txtName.Text;
                _profile.DelaySegmentStart = chkSegmentMode.Checked;
                _profile.DelayTolerance = int.Parse(txtTolerance.Text);
                return true;
            }
            catch (Exception ex)
            {
                MessageBox.Show("Неверный формат профиля:" + Environment.NewLine + ex.ToString(),
                    "PID Profile", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            }
            return false;
        }

        private void Profile_Load(object sender, EventArgs e)
        {
            oKToolStripMenuItem.Enabled = (Program.clsProfile.CurrentState == TemperatureProfile.State.Stopped);
            try
            {
                if (File.Exists(Properties.Settings.Default.LastProfile))
                    ParseProfileString(File.ReadAllText(Properties.Settings.Default.LastProfile));
            }
            catch (Exception)
            {

            }
        }

        private void погрешностьВремениСтабилизацииToolStripMenuItem_Click(object sender, EventArgs e)
        {

        }

        private void отсчётСегментаОтВремениСтабилизацииToolStripMenuItem_CheckedChanged(object sender, EventArgs e)
        {

        }

        private void oKToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (ParseProfile())
            {
                DialogResult = DialogResult.OK;
                Close();
            }
            else
            {
                _profile.Clear();
            }
        }

        private void сохранитьToolStripMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                saveFileDialog1.InitialDirectory = Path.GetDirectoryName(Properties.Settings.Default.LastProfile);
            }
            catch (Exception)
            {

            }
            if (saveFileDialog1.ShowDialog() == DialogResult.OK)
            {
                ParseProfile();
                try
                {
                    File.WriteAllText(saveFileDialog1.FileName, _profile.ToString());
                    Properties.Settings.Default.LastProfile = saveFileDialog1.FileName;
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Невозможно сохранить файл:" + Environment.NewLine + ex.ToString(),
                        "PID Profile", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                }
            }
        }

        private void txtTolerance_Click(object sender, EventArgs e)
        {

        }

        private void txtTolerance_TextChanged(object sender, EventArgs e)
        {
            
        }

        private void txtTolerance_Validating(object sender, CancelEventArgs e)
        {
            int t;
            if (int.TryParse(txtTolerance.Text, out t))
            {
                _profile.DelayTolerance = t;
            }
            else
            {
                e.Cancel = true;
                MessageBox.Show("Поддерживаются только целочисленные значения!",
                    "PID Profile", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            }
        }

        private void открытьToolStripMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                openFileDialog1.FileName = Path.GetFileName(Properties.Settings.Default.LastProfile);
                openFileDialog1.InitialDirectory = Path.GetDirectoryName(Properties.Settings.Default.LastProfile);
            }
            catch (Exception)
            {

            }
            if (openFileDialog1.ShowDialog() == DialogResult.OK)
            {
                ParseProfileString(File.ReadAllText(openFileDialog1.FileName));
                Properties.Settings.Default.LastProfile = openFileDialog1.FileName;
            }
        }

        private void ParseProfileString(string s)
        {
            TextReader reader = new StringReader(s);
            try
            {
                string n = reader.ReadLine();
                bool sm = bool.Parse(reader.ReadLine());
                int t = int.Parse(reader.ReadLine());
                string p = reader.ReadToEnd();
                txtName.Text = n;
                chkSegmentMode.Checked = sm;
                txtTolerance.Text = t.ToString();
                txtProfile.Text = p;
            }
            catch (FormatException ex)
            {
                MessageBox.Show("Файл имеет неверный формат! Подробнее:" + Environment.NewLine + ex.ToString(),
                    "PID Profile", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            }
        }

        private void предпросмотрToolStripMenuItem_Click(object sender, EventArgs e)
        {
            ParseProfile();
            chart1.Series[0].Points.Clear();
            foreach (var item in _profile)
            {
                chart1.Series[0].Points.AddXY(item.Key, item.Value);
            }
            chart1.Invalidate();
        }
    }
}
