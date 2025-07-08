using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.IO.Ports;
using System.Windows.Forms;

namespace VDK_PILL
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();

            // Cập nhật danh sách Thuốc
            cbValue.Items.Clear();
            cbValue.Items.AddRange(new object[] { "Thuốc đau bụng", "Thuốc hạ sốt", "Thuốc đau đầu" });
            cbValue.SelectedIndex = 0;

            CustomizeUI();
        }

        private void CustomizeUI()
        {
            // Cài đặt giao diện chung cho Form
            this.BackColor = Color.White;
            this.Font = new Font("Segoe UI", 11, FontStyle.Regular);
            txbValue.BorderStyle = BorderStyle.FixedSingle;

            this.Text = "VDK PILL";

            // Tùy chỉnh nút btnConnect
            btnConnect.FlatStyle = FlatStyle.Flat;
            btnConnect.BackColor = Color.White;
            btnConnect.ForeColor = Color.Black;
            btnConnect.Font = new Font("Segoe UI", 11, FontStyle.Bold);
            btnConnect.FlatAppearance.BorderColor = Color.Black;
            btnConnect.FlatAppearance.BorderSize = 1;
            btnConnect.MouseEnter += (s, e) =>
            {
                btnConnect.BackColor = Color.Black;
                btnConnect.ForeColor = Color.White;
            };
            btnConnect.MouseLeave += (s, e) =>
            {
                btnConnect.BackColor = Color.White;
                btnConnect.ForeColor = Color.Black;
            };

            // Tùy chỉnh nút btnConfirm
            btnConfirm.FlatStyle = FlatStyle.Flat;
            btnConfirm.BackColor = Color.White;
            btnConfirm.ForeColor = Color.Black;
            btnConfirm.Font = new Font("Segoe UI", 11, FontStyle.Bold);
            btnConfirm.FlatAppearance.BorderColor = Color.Black;
            btnConfirm.FlatAppearance.BorderSize = 1;
            btnConfirm.MouseEnter += (s, e) =>
            {
                btnConfirm.BackColor = Color.Black;
                btnConfirm.ForeColor = Color.White;
            };
            btnConfirm.MouseLeave += (s, e) =>
            {
                btnConfirm.BackColor = Color.White;
                btnConfirm.ForeColor = Color.Black;
            };

            // Tùy chỉnh ComboBox
            cbValue.FlatStyle = FlatStyle.Flat;
            cbValue.BackColor = Color.White;
            cbValue.ForeColor = Color.Black;
            cbValue.Font = new Font("Segoe UI", 11, FontStyle.Bold);

            // Tùy chỉnh TextBox
            txbValue.BackColor = Color.White;
            txbValue.ForeColor = Color.Black;
            txbValue.BorderStyle = BorderStyle.FixedSingle;
            txbValue.Font = new Font("Segoe UI", 11, FontStyle.Bold);

            // Tùy chỉnh Label hiển thị lựa chọn và trạng thái:
            // Loại bỏ BorderStyle mặc định và đăng ký sự kiện Paint để vẽ viền bo tròn
            lbValue.ForeColor = Color.Black;
            lbValue.Font = new Font("Segoe UI", 11, FontStyle.Bold);
            lbValue.BorderStyle = BorderStyle.None;
            lbValue.Padding = new Padding(5);
            lbValue.Paint += new PaintEventHandler(Label_Paint);

            lbStatus.ForeColor = Color.Black;
            lbStatus.Font = new Font("Segoe UI", 11, FontStyle.Bold);
            lbStatus.BorderStyle = BorderStyle.None;
            lbStatus.Padding = new Padding(5);
            lbStatus.Paint += new PaintEventHandler(Label_Paint);
        }

        /// <summary>
        /// Sự kiện Paint chung cho các Label để vẽ viền bo tròn tùy chỉnh.
        /// </summary>
        private void Label_Paint(object sender, PaintEventArgs e)
        {
            Label label = sender as Label;
            if (label == null) return;

            int borderRadius = 10;   // Bán kính bo tròn
            int borderWidth = 2;     // Độ dày viền
            Color borderColor = Color.Black;

            // Lấy vùng vẽ của Label
            Rectangle rect = new Rectangle(0, 0, label.Width - 1, label.Height - 1);
            using (GraphicsPath path = GetRoundedRect(rect, borderRadius))
            {
                using (Pen pen = new Pen(borderColor, borderWidth))
                {
                    e.Graphics.SmoothingMode = SmoothingMode.AntiAlias;
                    e.Graphics.DrawPath(pen, path);
                }
            }
        }

        /// <summary>
        /// Trả về một GraphicsPath biểu diễn hình chữ nhật bo tròn.
        /// </summary>
        private GraphicsPath GetRoundedRect(Rectangle bounds, int radius)
        {
            GraphicsPath path = new GraphicsPath();
            int diameter = radius * 2;
            Rectangle arcRect = new Rectangle(bounds.Location, new Size(diameter, diameter));

            // Góc trên bên trái
            path.AddArc(arcRect, 180, 90);

            // Góc trên bên phải
            arcRect.X = bounds.Right - diameter;
            path.AddArc(arcRect, 270, 90);

            // Góc dưới bên phải
            arcRect.Y = bounds.Bottom - diameter;
            path.AddArc(arcRect, 0, 90);

            // Góc dưới bên trái
            arcRect.X = bounds.Left;
            path.AddArc(arcRect, 90, 90);

            path.CloseFigure();
            return path;
        }

        // Hàm chuyển đổi tên Thuốc thành mô tả chi tiết
        private string GetDescription(string pillType)
        {
            switch (pillType)
            {
                case "Thuốc đau bụng":
                    return "Giúp giảm nhanh cơn đau bụng và hỗ trợ tiêu hóa.";
                case "Thuốc hạ sốt":
                    return "Hạ sốt hiệu quả, giảm cảm giác khó chịu.";
                case "Thuốc đau đầu":
                    return "Giảm đau đầu nhanh, giúp thư giãn tinh thần.";
                default:
                    return "";
            }
        }

        // Hàm chuyển đổi tên Thuốc đầy đủ thành mã ký tự gửi tới STM32
        private string GetPillCode(string pillType)
        {
            switch (pillType)
            {
                case "Thuốc đau bụng":
                    return "A";
                case "Thuốc hạ sốt":
                    return "B";
                case "Thuốc đau đầu":
                    return "C";
                default:
                    return "";
            }
        }

        private void btnConnect_Click(object sender, EventArgs e)
        {
            if (!serCOM.IsOpen)
            {
                btnConnect.Text = "DISCONNECT";
                serCOM.PortName = "COM3";
                serCOM.BaudRate = 115200;
                serCOM.Open();
            }
            else
            {
                btnConnect.Text = "CONNECT";
                serCOM.Close();
            }
        }

        private void btnConfirm_Click(object sender, EventArgs e)
        {
            string pillType = cbValue.SelectedItem?.ToString();
            string quantity = txbValue.Text;
            if (string.IsNullOrEmpty(pillType) || !int.TryParse(quantity, out int qty) || qty <= 0)
            {
                MessageBox.Show("Please enter a valid pill type and quantity.");
                return;
            }

            string description = GetDescription(pillType);
            lbValue.Text = $"Select:\nPill Type: {pillType}\nDescription: {description}\nQuantity: {qty}";

            if (serCOM.IsOpen)
            {
                // Lấy mã ký tự để gửi tới STM32
                string code = GetPillCode(pillType);
                string message = $"{code}{qty}\n"; // Ví dụ: "A10\n"
                serCOM.Write(message);
                MessageBox.Show("Data sent to STM32!");
            }
            else
            {
                MessageBox.Show("Please connect to COM first.");
            }
        }

        private void comboBox1_SelectedIndexChanged(object sender, EventArgs e)
        {
            // Xử lý nếu cần khi thay đổi lựa chọn
        }

        private void textBox1_TextChanged(object sender, EventArgs e)
        {
            string quantity = txbValue.Text;
            if (int.TryParse(quantity, out int qty) && qty > 0)
            {
                btnConfirm.Visible = true;
            }
            else
            {
                btnConfirm.Visible = false;
            }
        }

        private void txbValue_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter)
            {
                string pillType = cbValue.SelectedItem?.ToString();
                string quantity = txbValue.Text;
                if (int.TryParse(quantity, out int qty) && qty > 0)
                {
                    string description = GetDescription(pillType);
                    lbValue.Text = $"Selected:\nPill Type: {pillType}\nDescription: {description}\nQuantity: {qty}";
                    btnConfirm.Visible = true;
                }
                else
                {
                    MessageBox.Show("Please enter a valid quantity.");
                }
            }
        }

        private void serCOM_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            string response = serCOM.ReadExisting();
            Invoke((MethodInvoker)delegate
            {
                lbStatus.Text = response;
                MessageBox.Show("Received: " + response);
            });
        }

        private void label4_Click(object sender, EventArgs e)
        {
            // Xử lý sự kiện nếu cần
        }

        private void lbStatus_Click(object sender, EventArgs e)
        {
            // Xử lý sự kiện nếu cần
        }
        private void lbValue_Click(object sender, EventArgs e)
        {
            // Nếu không cần xử lý gì, có thể để trống
        }

        private void label2_Click(object sender, EventArgs e)
        {

        }
    }
}
