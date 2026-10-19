using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace MakotoInjector
{
    public class Injector : Form
    {
        private const string DLL_NAME = "MakotoExecutor.dll";
        private static string dllPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, DLL_NAME);

        private Button btnAttach;
        private Label lblStatus;
        private Label lblDllPath;
        private Timer timer;

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr OpenProcess(uint dwDesiredAccess, bool bInheritHandle, int dwProcessId);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr VirtualAllocEx(IntPtr hProcess, IntPtr lpAddress, uint dwSize, uint flAllocationType, uint flProtect);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool WriteProcessMemory(IntPtr hProcess, IntPtr lpBaseAddress, byte[] lpBuffer, uint nSize, out UIntPtr lpNumberOfBytesWritten);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr CreateRemoteThread(IntPtr hProcess, IntPtr lpThreadAttributes, uint dwStackSize, IntPtr lpStartAddress, IntPtr lpParameter, uint dwCreationFlags, out uint lpThreadId);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr GetProcAddress(IntPtr hModule, string lpProcName);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr GetModuleHandle(string lpModuleName);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool CloseHandle(IntPtr hObject);

        [DllImport("user32.dll", SetLastError = true)]
        static extern bool SetWindowText(IntPtr hWnd, string lpString);

        [DllImport("user32.dll")]
        static extern bool SetForegroundWindow(IntPtr hWnd);

        public Injector()
        {
            this.Text = "MAKOTO EXECUTOR - Injector";
            this.Size = new System.Drawing.Size(450, 250);
            this.FormBorderStyle = FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.StartPosition = FormStartPosition.CenterScreen;
            this.KeyPreview = true;

            lblStatus = new Label();
            lblStatus.Location = new System.Drawing.Point(20, 20);
            lblStatus.Size = new System.Drawing.Size(400, 30);
            lblStatus.Font = new System.Drawing.Font("Segoe UI", 12F, System.Drawing.FontStyle.Bold);
            lblStatus.Text = "Status: Aguardando Roblox...";
            lblStatus.DrawingColor = System.Drawing.Color.FromArgb(229, 57, 53);
            this.Controls.Add(lblStatus);

            lblDllPath = new Label();
            lblDllPath.Location = new System.Drawing.Point(20, 55);
            lblDllPath.Size = new System.Drawing.Size(400, 20);
            lblDllPath.Font = new System.Drawing.Font("Segoe UI", 9F);
            lblDllPath.Text = "DLL: " + dllPath;
            lblDllPath.DrawingColor = System.Drawing.Color.Gray;
            this.Controls.Add(lblDllPath);

            btnAttach = new Button();
            btnAttach.Location = new System.Drawing.Point(20, 90);
            btnAttach.Size = new System.Drawing.Size(400, 50);
            btnAttach.Text = "Attach to Roblox";
            btnAttach.Font = new System.Drawing.Font("Segoe UI", 14F, System.Drawing.FontStyle.Bold);
            btnAttach.FlatStyle = FlatStyle.Flat;
            btnAttach.FlatAppearance.BorderSize = 0;
            btnAttach.DrawingColor = System.Drawing.Color.White;
            btnAttach.DrawingBackColor = System.Drawing.Color.FromArgb(229, 57, 53);
            btnAttach.Enabled = false;
            btnAttach.Click += BtnAttach_Click;
            this.Controls.Add(btnAttach);

            Button btnRefresh = new Button();
            btnRefresh.Location = new System.Drawing.Point(20, 150);
            btnRefresh.Size = new System.Drawing.Size(190, 35);
            btnRefresh.Text = "Atualizar";
            btnRefresh.Font = new System.Drawing.Font("Segoe UI", 10F);
            btnRefresh.FlatStyle = FlatStyle.Flat;
            btnRefresh.FlatAppearance.BorderSize = 0;
            btnRefresh.DrawingColor = System.Drawing.Color.White;
            btnRefresh.DrawingBackColor = System.Drawing.Color.FromArgb(183, 28, 28);
            btnRefresh.Click += (s, e) => CheckRoblox();
            this.Controls.Add(btnRefresh);

            Button btnExit = new Button();
            btnExit.Location = new System.Drawing.Point(230, 150);
            btnExit.Size = new System.Drawing.Size(190, 35);
            btnExit.Text = "Sair";
            btnExit.Font = new System.Drawing.Font("Segoe UI", 10F);
            btnExit.FlatStyle = FlatStyle.Flat;
            btnExit.FlatAppearance.BorderSize = 0;
            btnExit.DrawingColor = System.Drawing.Color.White;
            btnExit.DrawingBackColor = System.Drawing.Color.FromArgb(26, 26, 26);
            btnExit.Click += (s, e) => Application.Exit();
            this.Controls.Add(btnExit);

            this.KeyDown += Injector_KeyDown;

            CheckRoblox();
            timer = new Timer();
            timer.Interval = 1000;
            timer.Tick += (s, e) => CheckRoblox();
            timer.Start();
        }

        private void Injector_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter)
            {
                if (btnAttach.Enabled)
                {
                    BtnAttach_Click(sender, e);
                }
            }
        }

        private void CheckRoblox()
        {
            var proc = GetRobloxProcess();
            if (proc != null)
            {
                lblStatus.Text = $"Status: Roblox encontrado! (PID {proc.Id})";
                lblStatus.DrawingColor = System.Drawing.Color.FromArgb(76, 175, 80);
                btnAttach.Enabled = true;
                btnAttach.Text = "Attach to Roblox";
            }
            else
            {
                lblStatus.Text = "Status: Aguardando Roblox...";
                lblStatus.DrawingColor = System.Drawing.Color.FromArgb(255, 152, 0);
                btnAttach.Enabled = false;
                btnAttach.Text = "Aguardando Roblox...";
            }
        }

        private Process GetRobloxProcess()
        {
            foreach (var p in Process.GetProcesses())
            {
                try
                {
                    if (p.ProcessName.Contains("WindowsPlayer") ||
                        p.ProcessName.Contains("RobloxPlayerBeta"))
                    {
                        return p;
                    }
                }
                catch { }
            }
            return null;
        }

        private void BtnAttach_Click(object sender, EventArgs e)
        {
            var proc = GetRobloxProcess();
            if (proc == null)
            {
                MessageBox.Show("Roblox não encontrado! Abra o Roblox primeiro.", "Erro",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            if (!File.Exists(dllPath))
            {
                MessageBox.Show("DLL não encontrada em:\n" + dllPath, "Erro",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            btnAttach.Enabled = false;
            btnAttach.Text = "Injetando...";
            lblStatus.Text = "Status: Injetando...";
            lblStatus.DrawingColor = System.Drawing.Color.FromArgb(255, 152, 0);

            try
            {
                bool success = Inject(proc.Id, dllPath);
                if (success)
                {
                    lblStatus.Text = "Status: Injetado com sucesso!";
                    lblStatus.DrawingColor = System.Drawing.Color.FromArgb(76, 175, 80);
                    btnAttach.Text = "Injetado!";
                    MessageBox.Show("MAKOTO EXECUTOR injetado com sucesso!\n\nO executor já está ativo no Roblox.", "Sucesso",
                        MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
                else
                {
                    lblStatus.Text = "Status: Falha ao injetar!";
                    lblStatus.DrawingColor = System.Drawing.Color.FromArgb(229, 57, 53);
                    btnAttach.Text = "Falha - Tentar Novamente";
                    btnAttach.Enabled = true;
                }
            }
            catch (Exception ex)
            {
                lblStatus.Text = "Status: Erro - " + ex.Message;
                lblStatus.DrawingColor = System.Drawing.Color.FromArgb(229, 57, 53);
                btnAttach.Text = "Erro - Tentar Novamente";
                btnAttach.Enabled = true;
            }
        }

        public bool Inject(int processId, string dllPath)
        {
            IntPtr hProcess = OpenProcess(0x001F0FFF, false, processId);
            if (hProcess == IntPtr.Zero) return false;

            IntPtr loadLibraryAddr = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");
            if (loadLibraryAddr == IntPtr.Zero) return false;

            byte[] dllBytes = System.Text.Encoding.ASCII.GetBytes(dllPath);
            IntPtr allocMemAddress = VirtualAllocEx(hProcess, IntPtr.Zero, (uint)dllBytes.Length, 0x1000 | 0x2000, 0x40);
            if (allocMemAddress == IntPtr.Zero) return false;

            WriteProcessMemory(hProcess, allocMemAddress, dllBytes, (uint)dllBytes.Length, out UIntPtr bytesWritten);

            CreateRemoteThread(hProcess, IntPtr.Zero, 0, loadLibraryAddr, allocMemAddress, 0, out uint threadId);

            CloseHandle(hProcess);
            return true;
        }

        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new Injector());
        }
    }
}
