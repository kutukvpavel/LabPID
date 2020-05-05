namespace LabPID
{
    partial class Profile
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea1 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            System.Windows.Forms.DataVisualization.Charting.Series series1 = new System.Windows.Forms.DataVisualization.Charting.Series();
            this.menuStrip1 = new System.Windows.Forms.MenuStrip();
            this.файлToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.открытьToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.сохранитьToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.настройкиToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.chkSegmentMode = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.погрешностьВремениСтабилизацииToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.txtTolerance = new System.Windows.Forms.ToolStripTextBox();
            this.oKToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.предпросмотрToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.имяПрофиляToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.txtName = new System.Windows.Forms.ToolStripTextBox();
            this.txtProfile = new System.Windows.Forms.TextBox();
            this.openFileDialog1 = new System.Windows.Forms.OpenFileDialog();
            this.saveFileDialog1 = new System.Windows.Forms.SaveFileDialog();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.chart1 = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this.menuStrip1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).BeginInit();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.chart1)).BeginInit();
            this.SuspendLayout();
            // 
            // menuStrip1
            // 
            this.menuStrip1.ImageScalingSize = new System.Drawing.Size(18, 18);
            this.menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.файлToolStripMenuItem,
            this.настройкиToolStripMenuItem,
            this.oKToolStripMenuItem,
            this.предпросмотрToolStripMenuItem,
            this.имяПрофиляToolStripMenuItem,
            this.txtName});
            this.menuStrip1.Location = new System.Drawing.Point(0, 0);
            this.menuStrip1.Name = "menuStrip1";
            this.menuStrip1.Size = new System.Drawing.Size(820, 29);
            this.menuStrip1.TabIndex = 0;
            this.menuStrip1.Text = "menuStrip1";
            // 
            // файлToolStripMenuItem
            // 
            this.файлToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.открытьToolStripMenuItem,
            this.сохранитьToolStripMenuItem});
            this.файлToolStripMenuItem.Name = "файлToolStripMenuItem";
            this.файлToolStripMenuItem.Size = new System.Drawing.Size(53, 25);
            this.файлToolStripMenuItem.Text = "Файл";
            // 
            // открытьToolStripMenuItem
            // 
            this.открытьToolStripMenuItem.Name = "открытьToolStripMenuItem";
            this.открытьToolStripMenuItem.Size = new System.Drawing.Size(148, 24);
            this.открытьToolStripMenuItem.Text = "Открыть";
            this.открытьToolStripMenuItem.Click += new System.EventHandler(this.открытьToolStripMenuItem_Click);
            // 
            // сохранитьToolStripMenuItem
            // 
            this.сохранитьToolStripMenuItem.Name = "сохранитьToolStripMenuItem";
            this.сохранитьToolStripMenuItem.Size = new System.Drawing.Size(148, 24);
            this.сохранитьToolStripMenuItem.Text = "Сохранить";
            this.сохранитьToolStripMenuItem.Click += new System.EventHandler(this.сохранитьToolStripMenuItem_Click);
            // 
            // настройкиToolStripMenuItem
            // 
            this.настройкиToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.chkSegmentMode,
            this.toolStripSeparator1,
            this.погрешностьВремениСтабилизацииToolStripMenuItem,
            this.txtTolerance});
            this.настройкиToolStripMenuItem.Name = "настройкиToolStripMenuItem";
            this.настройкиToolStripMenuItem.Size = new System.Drawing.Size(89, 25);
            this.настройкиToolStripMenuItem.Text = "Настройки";
            // 
            // chkSegmentMode
            // 
            this.chkSegmentMode.CheckOnClick = true;
            this.chkSegmentMode.Name = "chkSegmentMode";
            this.chkSegmentMode.Size = new System.Drawing.Size(354, 24);
            this.chkSegmentMode.Text = "Отсчёт сегмента от времени стабилизации";
            this.chkSegmentMode.CheckedChanged += new System.EventHandler(this.отсчётСегментаОтВремениСтабилизацииToolStripMenuItem_CheckedChanged);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(351, 6);
            // 
            // погрешностьВремениСтабилизацииToolStripMenuItem
            // 
            this.погрешностьВремениСтабилизацииToolStripMenuItem.Enabled = false;
            this.погрешностьВремениСтабилизацииToolStripMenuItem.Name = "погрешностьВремениСтабилизацииToolStripMenuItem";
            this.погрешностьВремениСтабилизацииToolStripMenuItem.Size = new System.Drawing.Size(354, 24);
            this.погрешностьВремениСтабилизацииToolStripMenuItem.Text = "Погрешность времени стабилизации (абс.):";
            this.погрешностьВремениСтабилизацииToolStripMenuItem.Click += new System.EventHandler(this.погрешностьВремениСтабилизацииToolStripMenuItem_Click);
            // 
            // txtTolerance
            // 
            this.txtTolerance.Name = "txtTolerance";
            this.txtTolerance.Size = new System.Drawing.Size(100, 25);
            this.txtTolerance.Text = "5";
            this.txtTolerance.Validating += new System.ComponentModel.CancelEventHandler(this.txtTolerance_Validating);
            this.txtTolerance.Click += new System.EventHandler(this.txtTolerance_Click);
            this.txtTolerance.TextChanged += new System.EventHandler(this.txtTolerance_TextChanged);
            // 
            // oKToolStripMenuItem
            // 
            this.oKToolStripMenuItem.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this.oKToolStripMenuItem.Name = "oKToolStripMenuItem";
            this.oKToolStripMenuItem.Size = new System.Drawing.Size(40, 25);
            this.oKToolStripMenuItem.Text = "OK";
            this.oKToolStripMenuItem.Click += new System.EventHandler(this.oKToolStripMenuItem_Click);
            // 
            // предпросмотрToolStripMenuItem
            // 
            this.предпросмотрToolStripMenuItem.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this.предпросмотрToolStripMenuItem.Name = "предпросмотрToolStripMenuItem";
            this.предпросмотрToolStripMenuItem.Size = new System.Drawing.Size(116, 25);
            this.предпросмотрToolStripMenuItem.Text = "Предпросмотр";
            this.предпросмотрToolStripMenuItem.Click += new System.EventHandler(this.предпросмотрToolStripMenuItem_Click);
            // 
            // имяПрофиляToolStripMenuItem
            // 
            this.имяПрофиляToolStripMenuItem.Enabled = false;
            this.имяПрофиляToolStripMenuItem.Name = "имяПрофиляToolStripMenuItem";
            this.имяПрофиляToolStripMenuItem.Size = new System.Drawing.Size(111, 25);
            this.имяПрофиляToolStripMenuItem.Text = "Имя профиля:";
            // 
            // txtName
            // 
            this.txtName.Name = "txtName";
            this.txtName.Size = new System.Drawing.Size(100, 25);
            this.txtName.Text = "Default";
            // 
            // txtProfile
            // 
            this.txtProfile.Dock = System.Windows.Forms.DockStyle.Fill;
            this.txtProfile.Location = new System.Drawing.Point(0, 0);
            this.txtProfile.Multiline = true;
            this.txtProfile.Name = "txtProfile";
            this.txtProfile.Size = new System.Drawing.Size(273, 305);
            this.txtProfile.TabIndex = 1;
            // 
            // openFileDialog1
            // 
            this.openFileDialog1.FileName = "openFileDialog1";
            this.openFileDialog1.Filter = "LabPID profiles|*.pid";
            // 
            // saveFileDialog1
            // 
            this.saveFileDialog1.DefaultExt = "pid";
            this.saveFileDialog1.Filter = "LabPID profiles|*.pid";
            // 
            // splitContainer1
            // 
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.Location = new System.Drawing.Point(0, 29);
            this.splitContainer1.Name = "splitContainer1";
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.txtProfile);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.chart1);
            this.splitContainer1.Size = new System.Drawing.Size(820, 305);
            this.splitContainer1.SplitterDistance = 273;
            this.splitContainer1.TabIndex = 2;
            // 
            // chart1
            // 
            chartArea1.Name = "ChartArea1";
            this.chart1.ChartAreas.Add(chartArea1);
            this.chart1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.chart1.Location = new System.Drawing.Point(0, 0);
            this.chart1.Name = "chart1";
            series1.BorderWidth = 2;
            series1.ChartArea = "ChartArea1";
            series1.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.StepLine;
            series1.MarkerSize = 8;
            series1.MarkerStyle = System.Windows.Forms.DataVisualization.Charting.MarkerStyle.Circle;
            series1.Name = "Series1";
            series1.XValueType = System.Windows.Forms.DataVisualization.Charting.ChartValueType.Int32;
            series1.YValueType = System.Windows.Forms.DataVisualization.Charting.ChartValueType.Single;
            this.chart1.Series.Add(series1);
            this.chart1.Size = new System.Drawing.Size(543, 305);
            this.chart1.TabIndex = 0;
            this.chart1.Text = "chart1";
            // 
            // Profile
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(820, 334);
            this.Controls.Add(this.splitContainer1);
            this.Controls.Add(this.menuStrip1);
            this.MainMenuStrip = this.menuStrip1;
            this.Name = "Profile";
            this.Text = "Profile";
            this.Load += new System.EventHandler(this.Profile_Load);
            this.menuStrip1.ResumeLayout(false);
            this.menuStrip1.PerformLayout();
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel1.PerformLayout();
            this.splitContainer1.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).EndInit();
            this.splitContainer1.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.chart1)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.MenuStrip menuStrip1;
        private System.Windows.Forms.ToolStripMenuItem файлToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem открытьToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem сохранитьToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem настройкиToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem chkSegmentMode;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripMenuItem погрешностьВремениСтабилизацииToolStripMenuItem;
        private System.Windows.Forms.ToolStripTextBox txtTolerance;
        private System.Windows.Forms.ToolStripMenuItem oKToolStripMenuItem;
        private System.Windows.Forms.TextBox txtProfile;
        private System.Windows.Forms.OpenFileDialog openFileDialog1;
        private System.Windows.Forms.SaveFileDialog saveFileDialog1;
        private System.Windows.Forms.ToolStripMenuItem предпросмотрToolStripMenuItem;
        private System.Windows.Forms.SplitContainer splitContainer1;
        private System.Windows.Forms.DataVisualization.Charting.Chart chart1;
        private System.Windows.Forms.ToolStripMenuItem имяПрофиляToolStripMenuItem;
        private System.Windows.Forms.ToolStripTextBox txtName;
    }
}