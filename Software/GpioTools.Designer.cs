namespace LabPID
{
    partial class GpioTools
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
            this.dgdOutputs = new System.Windows.Forms.DataGridView();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.dgdInputs = new System.Windows.Forms.DataGridView();
            this.OutputLabels = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Outputs = new System.Windows.Forms.DataGridViewCheckBoxColumn();
            this.InputLabels = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Inputs = new System.Windows.Forms.DataGridViewCheckBoxColumn();
            ((System.ComponentModel.ISupportInitialize)(this.dgdOutputs)).BeginInit();
            this.tableLayoutPanel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.dgdInputs)).BeginInit();
            this.SuspendLayout();
            // 
            // dgdOutputs
            // 
            this.dgdOutputs.AllowUserToAddRows = false;
            this.dgdOutputs.AllowUserToDeleteRows = false;
            this.dgdOutputs.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.dgdOutputs.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.OutputLabels,
            this.Outputs});
            this.dgdOutputs.Dock = System.Windows.Forms.DockStyle.Fill;
            this.dgdOutputs.Location = new System.Drawing.Point(3, 3);
            this.dgdOutputs.Name = "dgdOutputs";
            this.dgdOutputs.RowHeadersWidth = 45;
            this.dgdOutputs.Size = new System.Drawing.Size(297, 370);
            this.dgdOutputs.TabIndex = 0;
            this.dgdOutputs.CellValueChanged += new System.Windows.Forms.DataGridViewCellEventHandler(this.dgdOutputs_CellValueChanged);
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.ColumnCount = 2;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
            this.tableLayoutPanel1.Controls.Add(this.dgdOutputs, 0, 0);
            this.tableLayoutPanel1.Controls.Add(this.dgdInputs, 1, 0);
            this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.tableLayoutPanel1.Location = new System.Drawing.Point(0, 0);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 1;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tableLayoutPanel1.Size = new System.Drawing.Size(606, 376);
            this.tableLayoutPanel1.TabIndex = 1;
            // 
            // dgdInputs
            // 
            this.dgdInputs.AllowUserToAddRows = false;
            this.dgdInputs.AllowUserToDeleteRows = false;
            this.dgdInputs.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.dgdInputs.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.InputLabels,
            this.Inputs});
            this.dgdInputs.Dock = System.Windows.Forms.DockStyle.Fill;
            this.dgdInputs.Location = new System.Drawing.Point(306, 3);
            this.dgdInputs.Name = "dgdInputs";
            this.dgdInputs.ReadOnly = true;
            this.dgdInputs.RowHeadersWidth = 45;
            this.dgdInputs.Size = new System.Drawing.Size(297, 370);
            this.dgdInputs.TabIndex = 1;
            this.dgdInputs.CellValueChanged += new System.Windows.Forms.DataGridViewCellEventHandler(this.dgdInputs_CellValueChanged);
            // 
            // OutputLabels
            // 
            this.OutputLabels.HeaderText = "Имя";
            this.OutputLabels.MinimumWidth = 6;
            this.OutputLabels.Name = "OutputLabels";
            this.OutputLabels.Width = 110;
            // 
            // Outputs
            // 
            this.Outputs.HeaderText = "Состояние";
            this.Outputs.MinimumWidth = 6;
            this.Outputs.Name = "Outputs";
            this.Outputs.Width = 110;
            // 
            // InputLabels
            // 
            this.InputLabels.HeaderText = "Имя";
            this.InputLabels.MinimumWidth = 6;
            this.InputLabels.Name = "InputLabels";
            this.InputLabels.ReadOnly = true;
            this.InputLabels.Width = 110;
            // 
            // Inputs
            // 
            this.Inputs.HeaderText = "Состояние";
            this.Inputs.MinimumWidth = 6;
            this.Inputs.Name = "Inputs";
            this.Inputs.ReadOnly = true;
            this.Inputs.Width = 110;
            // 
            // GpioTools
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(606, 376);
            this.Controls.Add(this.tableLayoutPanel1);
            this.Name = "GpioTools";
            this.Text = "GpioTools";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.GpioTools_FormClosing);
            this.Load += new System.EventHandler(this.GpioTools_Load);
            ((System.ComponentModel.ISupportInitialize)(this.dgdOutputs)).EndInit();
            this.tableLayoutPanel1.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.dgdInputs)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.DataGridView dgdOutputs;
        private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
        private System.Windows.Forms.DataGridView dgdInputs;
        private System.Windows.Forms.DataGridViewTextBoxColumn OutputLabels;
        private System.Windows.Forms.DataGridViewCheckBoxColumn Outputs;
        private System.Windows.Forms.DataGridViewTextBoxColumn InputLabels;
        private System.Windows.Forms.DataGridViewCheckBoxColumn Inputs;
    }
}