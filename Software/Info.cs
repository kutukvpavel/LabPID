using System;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;

namespace LabPID
{
    public partial class Info : Form
    {
        private bool load;
        internal delegate void StringDelegate(string data);

        public Info()
        {
            InitializeComponent();
            Program.clsControl.GpioState.Changed += GpioState_Changed;
        }

        #region Form event handlers

        private void Info_Load(object sender, EventArgs e)
        {
            string name = Properties.Settings.Default.PortName;
            string[] available = Program.clsControl.GetPorts();
            if (available.Count() > 0)
            {
                bool mem = true;
                if (!available.Contains(name))
                {
                    name = available.First();
                    mem = false;
                }
                UpdatePorts();  
                PortChanged(name, mem);
            }
            LoadSettings();
            Program.clsLog.FilterInput = "\r|,\n|";         //Enables basic line feed formatting
        }

        private void Info_FormClosing(object sender, FormClosingEventArgs e)
        {
            Program.clsControl.Dispose();
            Program.clsLog.Dispose();
            if (this.Size == this.MinimumSize)
            {
                Properties.Settings.Default.chrtShow = false;
            }
            else
            {
                Properties.Settings.Default.chrtShow = true;
                Properties.Settings.Default.frmInfoSize = this.Size;
                Properties.Settings.Default.frmInfoState = this.WindowState;
            }
            Properties.Settings.Default.Save();
        }

        #endregion

        #region Private Methods

        private bool ApplyMode()
        {
            try
            {
                Program.clsControl.Mode = (Controller.ModeType)updMode.SelectedIndex;
                return false;
            }
            catch (InvalidCastException)
            {
                MessageBox.Show("Выбор режима за пределами диапазона.");
            }
            return true;
        }

        private void DisplaySignIfCompleted(ref PictureBox pct, bool completed)
        {
            if (completed)
            {
                DisplaySign(ref pct);
            }
            else
            {
                pct.Image = null;
            }
        }
        private void DisplaySign(ref PictureBox pct)
        {
            Sign(ref pct, Properties.Resources.warning_icon, "По готовности, нажмите, чтобы отправить данные контроллеру.", "0");
        }
        private void RemoveSign(ref PictureBox pct)
        {
            if (pct.Tag.ToString() == "0")
            {
                Sign(ref pct, Properties.Resources.Actions_dialog_ok_apply_icon, "Данные отправлены.", "1");
            }
        }
        private void Sign(ref PictureBox pct, Image img, string tooltip, string tag)
        {
            pct.Image = img;
            pct.Tag = tag;
            toolTip1.SetToolTip(pct, tooltip);
        }

        private Tuple<float, float> GetTempAndSetpoint()
        {
            return new Tuple<float, float>(Program.clsControl.Temp[0], Program.clsControl.Setpoint);
        }

        private void DisplayQSigns()
        {
            foreach (PictureBox item in Program.FlattenChildren(tableLayoutPanel1).OfType<PictureBox>())
            {
                item.Image = Properties.Resources.question_faq_icon;
                item.Tag = "2";
                toolTip1.SetToolTip(item, "Данные не загружены.");
            }
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

        private void LoadSettings()
        {
            chart1.Series[0].Color = Properties.Settings.Default.Color1;
            chart1.Series[1].Color = Properties.Settings.Default.Color2;
            chart1.Series[2].Color = Properties.Settings.Default.Color3;
            chart1.Series[3].Color = Properties.Settings.Default.Color4;
            foreach (Series item in chart1.Series)
            {
                item.ChartType = Properties.Settings.Default.ChartType;
            }
            this.Size = Properties.Settings.Default.chrtShow ? Properties.Settings.Default.frmInfoSize : this.MinimumSize;
            this.WindowState = Properties.Settings.Default.frmInfoState;
        }

        private void PortChanged(string newName, bool mem = true)
        {
            for (short i = 1; i < mnPorts.DropDownItems.Count; i++)
            {
                ((ToolStripMenuItem)mnPorts.DropDownItems[i]).CheckState = CheckState.Unchecked;
            }
            ((ToolStripMenuItem)mnPorts.DropDownItems[newName]).CheckState = CheckState.Checked;
            Program.clsControl.OpenPort(newName);
            проверитьПодключениеToolStripMenuItem_Click(this, null);
            DisplayQSigns();
            if(mem) Properties.Settings.Default.PortName = newName;
        }

        private void UpdatePorts()
        {
            Program.clsControl.ClosePort();
            SetStatus("Порт закрыт.");
            Program.clsControl.PollDevicePresence();
            включитьЗаписьToolStripMenuItem.Enabled = Program.clsControl.IsConnected;
            запроситьИнформациюToolStripMenuItem.Enabled = Program.clsControl.IsConnected;
            short cnt = (short)mnPorts.DropDownItems.Count;
            for (short i = 1; i < cnt; i++)
            {
                mnPorts.DropDownItems.RemoveAt(1);
            }
            foreach (string port in Program.clsControl.GetPorts())
            {
                ToolStripMenuItem buf = new ToolStripMenuItem(port, null, buf_Click);
                buf.Name = port;
                mnPorts.DropDownItems.Add(buf);
            }
        }

        private void UpdateDisplay(bool partially = false)
        {
            lblTemp1.Text = Program.clsControl.Temp[0].ToString(Properties.Settings.Default.InfoFormTemperatureFormat) + "°C";
            lblTemp2.Text = Program.clsControl.Temp[1].ToString(Properties.Settings.Default.InfoFormTemperatureFormat) + "°C";
            if (Program.clsControl.Mode != Controller.ModeType.Manual)
            {
                mtbPower.Text = Program.clsControl.Power.ToString("+000;-000");
            }
            mtbSetpoint.Text = Program.clsControl.Setpoint.ToString(Program.ShortFloatFormat);
            updMode.SelectedIndex = updMode.Items.IndexOf(Controller.ModeDesignator[Program.clsControl.Mode].ToString());
            updChannel.SelectedIndex = Program.clsControl.Channel;
            if (!partially)
            {
                mtbC0.Text = Program.clsControl.Calibrations[0].ToString(Program.ShortFloatFormat);
                mtbC1.Text = Program.clsControl.Calibrations[1].ToString(Program.ShortFloatFormat);
                mtbC2.Text = Program.clsControl.Calibrations[2].ToString(Program.ShortFloatFormat);
                mtbKPP.Text = Program.clsControl.K[0][0].ToString(Program.FloatFormat);
                mtbKPI.Text = Program.clsControl.K[0][1].ToString(Program.FloatFormat);
                mtbKPD.Text = Program.clsControl.K[0][2].ToString(Program.FloatFormat);
                mtbKAP.Text = Program.clsControl.K[1][0].ToString(Program.FloatFormat);
                mtbKAI.Text = Program.clsControl.K[1][1].ToString(Program.FloatFormat);
                mtbKAD.Text = Program.clsControl.K[1][2].ToString(Program.FloatFormat);
                mtbAmplifier.Text = Program.clsControl.Amplifier.ToString(Program.FloatFormat);
                mtbIntegral.Text = (Program.clsControl.Integral * 100).ToString("000.00");
                mtbDisitill.Text = (Program.clsControl.ExtraPower * 100).ToString("000.00");
                mtbOver.Text = Program.clsControl.Overshoot.ToString(Program.ShortFloatFormat);
                chkAuto.Checked = (Program.clsControl.Mode != Controller.ModeType.Manual);
                chkAv1.Checked = Program.clsControl.Averages[0];
                chkAv2.Checked = Program.clsControl.Averages[1];
                chkAv3.Checked = Program.clsControl.Averages[2];
                chkCooler0.Checked = Program.clsControl.Coolers[0];
                chkCooler1.Checked = Program.clsControl.Coolers[1];
                chkCooler2.Checked = Program.clsControl.Coolers[2];
                chkCJC.Checked = Program.clsControl.EnableCjc;
                Program.frmSet.AmbientUpdate();
                foreach (PictureBox item in Program.FlattenChildren(tableLayoutPanel1).OfType<PictureBox>())
                {
                    item.Image = null;
                    item.Tag = "-1";
                }
                toolTip1.RemoveAll();
                bool b = (updChannel.SelectedIndex == 0);
                mtbC0.Enabled = b;
                chkAv1.Enabled = b;
                chkCooler0.Enabled = b;
                b = (updChannel.SelectedIndex == 1);
                mtbC1.Enabled = b;
                chkAv2.Enabled = b;
                chkCooler1.Enabled = b;
                b = (updChannel.SelectedIndex == 2);
                mtbC2.Enabled = b;
                chkAv3.Enabled = b;
                chkCooler2.Enabled = b;
            }
            if (Program.clsControl.Error)
            {
                Sign(ref pctWarning, Properties.Resources.Warning_icon__1_, "Сработало защитное отключение и/или не подключен необходимый датчик.", "3");
            }
            else
            {
                MarkLoggingState();
            }
            NamedPipeService.Instance.BroadcastTemperature(Program.clsControl.Temp[0]);
        }

        private void MarkLoggingState()
        {
            if (Program.clsControl.Logging)
            {
                Sign(ref pctWarning, Properties.Resources.Actions_dialog_ok_apply_icon, "Автообновление включено.", "1");
                Sign(ref pctAutoPwr, Properties.Resources.Actions_dialog_ok_apply_icon, "Автообновление включено.", "1");
                Sign(ref pctSet, Properties.Resources.Actions_dialog_ok_apply_icon, "Автообновление включено.", "1");
                Sign(ref pctMode, Properties.Resources.Actions_dialog_ok_apply_icon, "Автообновление включено.", "1");
            }
            else
            {
                Sign(ref pctWarning, Properties.Resources.question_faq_icon, "Данные не обновляются.", "2");
                Sign(ref pctAutoPwr, Properties.Resources.question_faq_icon, "Данные не обновляются.", "2");
                Sign(ref pctSet, Properties.Resources.question_faq_icon, "Данные не обновляются.", "2");
                Sign(ref pctMode, Properties.Resources.question_faq_icon, "Данные не обновляются.", "2");
            }
        }

        public void ChartDataAdd()
        {
            if (chart1.InvokeRequired)
            {
                chart1.Invoke(new MethodInvoker(delegate {
                    chart1.Series[0].Points.AddY(Program.clsControl.Temp[0]);
                    chart1.Series[1].Points.AddY(Program.clsControl.Temp[1]);
                    chart1.Series[2].Points.AddY(Program.clsControl.Power);
                    chart1.Series[3].Points.AddY(Program.clsControl.Setpoint);
                }));
            }
            else
            {
                chart1.Series[0].Points.AddY(Program.clsControl.Temp[0]);
                chart1.Series[1].Points.AddY(Program.clsControl.Temp[1]);
                chart1.Series[2].Points.AddY(Program.clsControl.Power);
                chart1.Series[3].Points.AddY(Program.clsControl.Setpoint);
            }
            //RefreshDisplay();
        }

        public void RefreshDisplay(bool part = true)
        {
            if (InvokeRequired)
            {
                Invoke(new MethodInvoker(delegate
                {
                    UpdateDisplay(part);
                }));
            }
            else
            {
                UpdateDisplay(part);
            }
        }

        #endregion

        #region UI event handlers

        void buf_Click(object sender, EventArgs e)
        {
            PortChanged(((ToolStripItem)sender).Text);
        }

        private void mnPortsUpdate_Click(object sender, EventArgs e)
        {
            UpdatePorts();
        }

        private void запроситьИнформациюToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Program.clsControl.Update();
            timer1.Start();
            SetStatus("Обновление информации...");
        }

        private void maskedTextBox8_MaskInputRejected(object sender, MaskInputRejectedEventArgs e)
        {

        }

        private void mnGeneralSettings_Click(object sender, EventArgs e)
        {
            if(Program.frmSet.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                LoadSettings(); 
            }
        }

        private void входToolStripMenuItem_Click(object sender, EventArgs e)
        {
            chart1.Series[0].Enabled = входToolStripMenuItem.Checked;
        }

        private void dS18B20ToolStripMenuItem_Click(object sender, EventArgs e)
        {
            chart1.Series[1].Enabled = dS18B20ToolStripMenuItem.Checked;
        }

        private void мощностьToolStripMenuItem_Click(object sender, EventArgs e)
        {
            chart1.Series[2].Enabled = мощностьToolStripMenuItem.Checked;
        }

        private void включитьЗаписьToolStripMenuItem_Click(object sender, EventArgs e)
        {         
            Program.clsControl.Logging = включитьЗаписьToolStripMenuItem.Checked;
            запроситьИнформациюToolStripMenuItem.Enabled = !включитьЗаписьToolStripMenuItem.Checked;
            проверитьПодключениеToolStripMenuItem.Enabled = !включитьЗаписьToolStripMenuItem.Checked;
            updChannel.Enabled = !включитьЗаписьToolStripMenuItem.Checked;
            updMode.Enabled = !включитьЗаписьToolStripMenuItem.Checked;
            SetStatus(включитьЗаписьToolStripMenuItem.Checked ? "Запись включена." : "Запись отключена.");
            //Program.SkipLine = true;
            MarkLoggingState();
        }

        private void проверитьПодключениеToolStripMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                Program.clsControl.PollDevicePresence();
                SetStatus("Проверка подключения...");
            }
            catch (Exception ex)
            {
                SetStatus("Ошибка...");
                Program.clsLog.WriteError(ex);
            }
            load = true;
            timer1.Start();
        }

        private void mnTerminal_Click(object sender, EventArgs e)
        {
            Program.frmTerm.Show();
        }

        private void mnAbout_Click(object sender, EventArgs e)
        {
            Program.frmAbout.Show();
        }

        private void mnChart_Click(object sender, EventArgs e)
        {
            if (this.Size == this.MinimumSize)
            {
                if (!(Properties.Settings.Default.frmInfoSize.Width > this.MinimumSize.Width))
                {
                    Properties.Settings.Default.frmInfoSize = new Size(this.MinimumSize.Width + 200, this.MinimumSize.Height);
                }
                this.Size = Properties.Settings.Default.frmInfoSize;
            }
            else
            {
                this.Size = this.MinimumSize;
            }
        }

        private void toolStripDropDownButton1_Click(object sender, EventArgs e)
        {

        }

        private void включитьЖурналированиеToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Program.clsLog.Enabled = включитьЖурналированиеToolStripMenuItem.Checked;
            Properties.Settings.Default.EnableHistory = включитьЖурналированиеToolStripMenuItem.Checked;
        }

        private void chkAuto_CheckedChanged(object sender, EventArgs e)
        {
            //DisplaySign(ref pctAutoPwr);
        }

        private void chkAv1_CheckedChanged(object sender, EventArgs e)
        {
            DisplaySign(ref pctAv1);
        }

        private void chkAv2_CheckedChanged(object sender, EventArgs e)
        {
            DisplaySign(ref pctAv2);
        }

        private void chkAv3_CheckedChanged(object sender, EventArgs e)
        {
            DisplaySign(ref pctAv3);
        }

        private void updMode_SelectedItemChanged(object sender, EventArgs e)
        {
            DisplaySign(ref pctMode);
            if ((updMode.SelectedItem.ToString()[0] == 'M') == chkAuto.Checked)
            {
                chkAuto.Checked = !chkAuto.Checked;     
            }
        }

        private void updChannel_SelectedItemChanged(object sender, EventArgs e)
        {
            DisplaySign(ref pctMode);
        }

        private void pctWarning_Click(object sender, EventArgs e)
        {
            if (((Control)sender).Tag.ToString() == "3")
            {
                MessageBox.Show("Ошибка автоматически снимется по возвращении устройства в допустимое состояние. Может потребоваться перезагрузить устройство." + Environment.NewLine + "Данная пиктограмма будет сейчас скрыта. Для получения обновлённой информации о статусе устройства см. дисплей.");
                pctWarning.Tag = null;
                pctWarning.Image = null;
                Program.clsControl.ClearError();
            }
        }

        private void pctAutoPwr_Click(object sender, EventArgs e)
        {
            if (((Control)sender).Tag.ToString() != "0") return;
            ApplyMode();
            if (Program.clsControl.Mode == Controller.ModeType.Manual)
            {
                Program.clsControl.Power = short.Parse(mtbPower.Text.Remove(3));
            }
            RemoveSign(ref pctAutoPwr);
        }

        private void pctMode_Click(object sender, EventArgs e)
        {
            if (((Control)sender).Tag.ToString() != "0") return;
            ApplyMode();
            Program.clsControl.Channel = (byte)(updChannel.SelectedItem.ToString()[0] - '0');
            RemoveSign(ref pctMode);
        }

        private void pctC0_Click(object sender, EventArgs e)
        {
            if (((Control)sender).Tag.ToString() != "0") return;
            Program.clsControl.CurrentCalibration = float.Parse(mtbC0.Text.Remove(7));
            RemoveSign(ref pctC0);
        }

        private void pctAI_Click(object sender, EventArgs e)
        {
            if (((Control)sender).Tag.ToString() != "0") return;
            Program.clsControl[Controller.ModeType.Aggressive,
                Controller.PidCoeff.Integral] = float.Parse(mtbKAI.Text);
            RemoveSign(ref pctAI);
        }

        private void pctC1_Click(object sender, EventArgs e)
        {
            if (((Control)sender).Tag.ToString() != "0") return;
            Program.clsControl.CurrentCalibration = float.Parse(mtbC1.Text.Remove(7));
            RemoveSign(ref pctC1);
        }

        private void pctC2_Click(object sender, EventArgs e)
        {
            if (((Control)sender).Tag.ToString() != "0") return;
            Program.clsControl.CurrentCalibration = float.Parse(mtbC2.Text.Remove(7));
            RemoveSign(ref pctC2);
        }

        private void pctAP_Click(object sender, EventArgs e)
        {
            if (((Control)sender).Tag.ToString() != "0") return;
            Program.clsControl[Controller.ModeType.Aggressive, 
                Controller.PidCoeff.Proportional] = float.Parse(mtbKAP.Text);
            RemoveSign(ref pctAP);
        }

        private void pctAD_Click(object sender, EventArgs e)
        {
            if (((Control)sender).Tag.ToString() != "0") return;
            Program.clsControl[Controller.ModeType.Aggressive,
                Controller.PidCoeff.Differential] = float.Parse(mtbKAD.Text);
            RemoveSign(ref pctAD);
        }

        private void pctNP_Click(object sender, EventArgs e)
        {
            if (((Control)sender).Tag.ToString() != "0") return;
            Program.clsControl[Controller.ModeType.Normal,
                Controller.PidCoeff.Proportional] = float.Parse(mtbKPP.Text);
            RemoveSign(ref pctNP);
        }

        private void pctNI_Click(object sender, EventArgs e)
        {
            if (((Control)sender).Tag.ToString() != "0") return;
            Program.clsControl[Controller.ModeType.Normal,
                Controller.PidCoeff.Integral] = float.Parse(mtbKPI.Text);
            RemoveSign(ref pctNI);
        }

        private void pctND_Click(object sender, EventArgs e)
        {
            if (((Control)sender).Tag.ToString() != "0") return;
            Program.clsControl[Controller.ModeType.Normal,
                Controller.PidCoeff.Differential] = float.Parse(mtbKPD.Text);
            RemoveSign(ref pctND);
        }

        private void pctIntegral_Click(object sender, EventArgs e)
        {
            if (((Control)sender).Tag.ToString() != "0") return;
            Program.clsControl.Integral = float.Parse(mtbIntegral.Text.Remove(6)) / 100;
            RemoveSign(ref pctIntegral);
        }

        private void pctPower_Click(object sender, EventArgs e)
        {
            if (((Control)sender).Tag.ToString() != "0") return;
            Program.clsControl.ExtraPower = float.Parse(mtbDisitill.Text.Remove(6)) / 100;
            RemoveSign(ref pctPower);
        }

        private void pctAmplifier_Click(object sender, EventArgs e)
        {
            if (((Control)sender).Tag.ToString() != "0") return;
            Program.clsControl.Amplifier = float.Parse(mtbAmplifier.Text);
            RemoveSign(ref pctAmplifier);
        }

        private void pctAv1_Click(object sender, EventArgs e)
        {
            if (((Control)sender).Tag.ToString() != "0") return;
            Program.clsControl.CurrentAveraging = chkAv1.Checked;
            RemoveSign(ref pctAv1);
        }

        private void pctAv2_Click(object sender, EventArgs e)
        {
            if (((Control)sender).Tag.ToString() != "0") return;
            Program.clsControl.CurrentAveraging = chkAv2.Checked;
            RemoveSign(ref pctAv2);
        }

        private void pctAv3_Click(object sender, EventArgs e)
        {
            if (((Control)sender).Tag.ToString() != "0") return;
            Program.clsControl.CurrentAveraging = chkAv3.Checked;
            RemoveSign(ref pctAv3);
        }

        private void очиститьToolStripMenuItem_Click(object sender, EventArgs e)
        {
            foreach (Series item in chart1.Series)
            {
                item.Points.Clear();
            }
        }

        private void уставкаToolStripMenuItem_Click(object sender, EventArgs e)
        {
            chart1.Series[3].Enabled = уставкаToolStripMenuItem.Checked;
        }

        private void pctSet_Click(object sender, EventArgs e)
        {
            if (((Control)sender).Tag.ToString() != "0") return;
            Program.clsControl.Setpoint = float.Parse(mtbSetpoint.Text.Remove(7));
            RemoveSign(ref pctSet);
        }

        private void pctOver_Click(object sender, EventArgs e)
        {
            if (((Control)sender).Tag.ToString() != "0") return;
            Program.clsControl.Overshoot = float.Parse(mtbOver.Text.Remove(6));
            RemoveSign(ref pctOver);
        }

        private void mtbOver_TextChanged(object sender, EventArgs e)
        {
            DisplaySignIfCompleted(ref pctOver, mtbOver.MaskCompleted);
        }

        private void mtbPower_TextChanged(object sender, EventArgs e)
        {
            DisplaySignIfCompleted(ref pctAutoPwr, mtbPower.MaskCompleted);
        }

        private void mtbSetpoint_TextChanged(object sender, EventArgs e)
        {
            DisplaySignIfCompleted(ref pctSet, mtbSetpoint.MaskCompleted);
        }

        private void mtbC0_TextChanged(object sender, EventArgs e)
        {
            DisplaySignIfCompleted(ref pctC0, mtbC0.MaskCompleted);
        }

        private void mtbC1_TextChanged(object sender, EventArgs e)
        {
            DisplaySignIfCompleted(ref pctC1, mtbC1.MaskCompleted);
        }

        private void mtbC2_TextChanged(object sender, EventArgs e)
        {
            DisplaySignIfCompleted(ref pctC2, mtbC2.MaskCompleted);
        }

        private void mtbKPP_TextChanged(object sender, EventArgs e)
        {
            DisplaySignIfCompleted(ref pctNP, mtbKPP.MaskCompleted);
        }

        private void mtbKPI_TextChanged(object sender, EventArgs e)
        {
            DisplaySignIfCompleted(ref pctNI, mtbKPI.MaskCompleted);
        }

        private void mtbKPD_TextChanged(object sender, EventArgs e)
        {
            DisplaySignIfCompleted(ref pctND, mtbKPD.MaskCompleted);
        }

        private void mtbKAP_TextChanged(object sender, EventArgs e)
        {
            DisplaySignIfCompleted(ref pctAP, mtbKAP.MaskCompleted);
        }

        private void mtbKAI_TextChanged(object sender, EventArgs e)
        {
            DisplaySignIfCompleted(ref pctAI, mtbKAI.MaskCompleted);
        }

        private void mtbKAD_TextChanged(object sender, EventArgs e)
        {
            DisplaySignIfCompleted(ref pctAD, mtbKAD.MaskCompleted);
        }

        private void mtbIntegral_TextChanged(object sender, EventArgs e)
        {
            DisplaySignIfCompleted(ref pctIntegral, mtbIntegral.MaskCompleted);
        }

        private void mtbDisitill_TextChanged(object sender, EventArgs e)
        {
            DisplaySignIfCompleted(ref pctPower, mtbDisitill.MaskCompleted);
        }

        private void mtbAmplifier_TextChanged(object sender, EventArgs e)
        {
            DisplaySignIfCompleted(ref pctAmplifier, mtbAmplifier.MaskCompleted);
        }

        private void сохранитьКакИзображениеToolStripMenuItem_Click(object sender, EventArgs e)
        {
            chart1.SaveImage(Program.Log.GenerateFilename("png"), ChartImageFormat.Png);
        }

        private void задатьПрофильToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (Program.clsProfile.Count > 0) Program.frmProfile.CurrentProfile = Program.clsProfile;
            if (Program.frmProfile.ShowDialog() == DialogResult.OK)
            {
                Program.clsProfile = Program.frmProfile.CurrentProfile;
            }
        }

        private void начатьВыполнениеToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Program.clsProfile.TimeToChangeSetpoint += ClsProfile_TimeToChangeSetpoint;
            Program.clsProfile.ExecutionFinished += ClsProfile_ExecutionFinished;
            Program.clsProfile.TemperatureValidationCallback += GetTempAndSetpoint;
            Program.clsProfile.TimeToIssueCustomCommand += ClsProfile_TimeToIssueCustomCommand;
            Program.clsProfile.Start();
            if (!включитьЗаписьToolStripMenuItem.Checked)
            {
                включитьЗаписьToolStripMenuItem.Checked = true;
                включитьЗаписьToolStripMenuItem_Click(this, new EventArgs());
            } 
            btnSession.Enabled = false;
            lblProfile.Text = (lblProfile.Tag as string) + Program.clsProfile.Name;
        }

        private void остановитьToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Program.clsProfile.Stop();                       
        }

        private void приостановитьToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Program.clsProfile.Pause();
        }

        private void btnProfile_Click(object sender, EventArgs e)
        {
            switch (Program.clsProfile.CurrentState)
            {
                case TemperatureProfile.State.Stopped:
                    начатьВыполнениеToolStripMenuItem.Enabled = Program.clsProfile.Count > 0;
                    приостановитьToolStripMenuItem.Enabled = false;
                    остановитьToolStripMenuItem.Enabled = false;
                    break;
                case TemperatureProfile.State.Suspended:
                    начатьВыполнениеToolStripMenuItem.Enabled = true;
                    приостановитьToolStripMenuItem.Enabled = false;
                    остановитьToolStripMenuItem.Enabled = true;
                    break;
                case TemperatureProfile.State.Running:
                    начатьВыполнениеToolStripMenuItem.Enabled = false;
                    приостановитьToolStripMenuItem.Enabled = true;
                    остановитьToolStripMenuItem.Enabled = true;
                    break;
                default:
                    break;
            }
        }

        private void сохранитьКакCSVToolStripMenuItem_Click(object sender, EventArgs e)
        {                      
            StringBuilder s = new StringBuilder();
            s.Append("Время");
            foreach (var item in chart1.Series)
            {
                s.AppendFormat(",{0}", item.Name);
            }                      
            s.AppendLine();
            int c = chart1.Series.Max(x => x.Points.Count);
            for (int i = 0; i < c; i++)
            {
                s.Append(i);
                foreach (var item in chart1.Series)
                {
                    if (!item.Enabled) continue;
                    string t = "";
                    try
                    {
                        t = item.Points[i].YValues[0].ToString(CultureInfo.InvariantCulture);
                    }
                    catch (ArgumentOutOfRangeException) { }
                    catch (IndexOutOfRangeException) { }
                    s.AppendFormat(",{0}", t);
                }
                s.AppendLine();
            }
            File.WriteAllText(Program.Log.GenerateFilename("csv"), s.ToString());
        }

        private void chkCJC_CheckedChanged(object sender, EventArgs e)
        {
            DisplaySign(ref pctCJC);
        }

        private void pctCJC_Click(object sender, EventArgs e)
        {
            if (((Control)sender).Tag.ToString() != "0") return;
            Program.clsControl.EnableCjc = chkCJC.Checked;
            RemoveSign(ref pctCJC);
        }

        private void label6_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            if (!Program.clsControl.Logging) Program.clsControl.SetGpioOutput(0xFF, false);
            Program.frmGpioTools.Show();
        }

        private void chkCooler0_CheckedChanged(object sender, EventArgs e)
        {
            DisplaySign(ref pctCooler0);
        }

        private void chkCooler1_CheckedChanged(object sender, EventArgs e)
        {
            DisplaySign(ref pctCooler1);
        }

        private void chkCooler2_CheckedChanged(object sender, EventArgs e)
        {
            DisplaySign(ref pctCooler2);
        }

        private void pctCooler0_Click(object sender, EventArgs e)
        {
            if (((Control)sender).Tag.ToString() != "0") return;
            Program.clsControl.CurrentCooler = chkCooler0.Checked;
            RemoveSign(ref pctCooler0);
        }

        private void pctCooler1_Click(object sender, EventArgs e)
        {
            if (((Control)sender).Tag.ToString() != "0") return;
            Program.clsControl.CurrentCooler = chkCooler1.Checked;
            RemoveSign(ref pctCooler1);
        }

        private void pctCooler2_Click(object sender, EventArgs e)
        {
            if (((Control)sender).Tag.ToString() != "0") return;
            Program.clsControl.CurrentCooler = chkCooler2.Checked;
            RemoveSign(ref pctCooler2);
        }

        #endregion

        #region Timer, GPIO, Profile events

        private void timer1_Tick(object sender, EventArgs e)
        {
            timer1.Stop();
            if (load)
            {
                включитьЗаписьToolStripMenuItem.Enabled = Program.clsControl.IsConnected;
                запроситьИнформациюToolStripMenuItem.Enabled = Program.clsControl.IsConnected;
                if (Program.clsControl.IsConnected)
                {
                    SetStatus("Подключено.");
                }
                else
                {
                    SetStatus("Устройство не обнаружено.");
                    включитьЗаписьToolStripMenuItem.Checked = false;
                }
                load = false;
            }
            else
            {
                UpdateDisplay();
                SetStatus("Готов.");
            }
        }

        private void GpioState_Changed(object sender, System.ComponentModel.HandledEventArgs e)
        {
            string s = Program.clsControl.GpioState.ToString();
            if (!e.Handled)
                Invoke((Action)(() => { lblGpio.Text = s; }));
            NamedPipeService.Instance.BroadcastCustomCommand(s);
        }

        private void ClsProfile_TimeToIssueCustomCommand(object sender, CustomCommandEventArgs e)
        {
            Program.clsControl.SendCustom(e.Command);
        }

        private void ClsProfile_ExecutionFinished(object sender, EventArgs e)
        {
            Program.clsProfile.ExecutionFinished -= ClsProfile_ExecutionFinished;
            Program.clsProfile.TimeToChangeSetpoint -= ClsProfile_TimeToChangeSetpoint;
            Program.clsProfile.TemperatureValidationCallback -= GetTempAndSetpoint;
            Invoke((Action)(() =>
            {
                btnSession.Enabled = true;
                включитьЗаписьToolStripMenuItem.Checked = false;
                включитьЗаписьToolStripMenuItem_Click(this, new EventArgs());
                lblProfile.Text = (lblProfile.Tag as string) + "нет";
            }));
        }

        private void ClsProfile_TimeToChangeSetpoint(object sender, TemperatureEventArgs e)
        {
            Program.clsControl.Setpoint = e.Temperature;
        }

        #endregion
    }
}
