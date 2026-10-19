# MAKOTO EXECUTOR - PowerShell Injector
Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
using System.Diagnostics;

public class NativeMethods {
    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern IntPtr OpenProcess(uint dwDesiredAccess, bool bInheritHandle, int dwProcessId);

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern IntPtr VirtualAllocEx(IntPtr hProcess, IntPtr lpAddress, uint dwSize, uint flAllocationType, uint flProtect);

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern bool WriteProcessMemory(IntPtr hProcess, IntPtr lpBaseAddress, byte[] lpBuffer, uint nSize, out UIntPtr lpNumberOfBytesWritten);

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern IntPtr CreateRemoteThread(IntPtr hProcess, IntPtr lpThreadAttributes, uint dwStackSize, IntPtr lpStartAddress, IntPtr lpParameter, uint dwCreationFlags, out uint lpThreadId);

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern IntPtr GetProcAddress(IntPtr hModule, string lpProcName);

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern IntPtr GetModuleHandle(string lpModuleName);

    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern bool CloseHandle(IntPtr hObject);
}
"@

function Get-RobloxProcess {
    $procs = Get-Process | Where-Object { $_.ProcessName -match "WindowsPlayer|RobloxPlayerBeta" }
    return $procs | Select-Object -First 1
}

function Inject-DLL {
    param(
        [int]$ProcessId,
        [string]$DllPath
    )

    $PROCESS_ALL_ACCESS = 0x001F0FFF
    $MEM_COMMIT = 0x1000
    $MEM_RESERVE = 0x2000
    $PAGE_READWRITE = 0x04

    $hProcess = [NativeMethods]::OpenProcess($PROCESS_ALL_ACCESS, $false, $ProcessId)
    if ($hProcess -eq [IntPtr]::Zero) {
        Write-Host "[ERRO] Não conseguiu abrir o processo. Código: $([Runtime.InteropServices.Marshal]::GetLastWin32Error())" -ForegroundColor Red
        return $false
    }

    $loadLibraryAddr = [NativeMethods]::GetProcAddress([NativeMethods]::GetModuleHandle("kernel32.dll"), "LoadLibraryA")
    if ($loadLibraryAddr -eq [IntPtr]::Zero) {
        Write-Host "[ERRO] Não encontrou LoadLibraryA" -ForegroundColor Red
        [NativeMethods]::CloseHandle($hProcess)
        return $false
    }

    $dllBytes = [System.Text.Encoding]::ASCII.GetBytes($DllPath)
    $allocMem = [NativeMethods]::VirtualAllocEx($hProcess, [IntPtr]::Zero, [uint32]$dllBytes.Length, $MEM_COMMIT -bor $MEM_RESERVE, $PAGE_READWRITE)
    if ($allocMem -eq [IntPtr]::Zero) {
        Write-Host "[ERRO] Não conseguiu alocar memória" -ForegroundColor Red
        [NativeMethods]::CloseHandle($hProcess)
        return $false
    }

    [NativeMethods]::WriteProcessMemory($hProcess, $allocMem, $dllBytes, [uint32]$dllBytes.Length, [ref][UIntPtr]::Zero) | Out-Null

    $thread = [NativeMethods]::CreateRemoteThread($hProcess, [IntPtr]::Zero, 0, $loadLibraryAddr, $allocMem, 0, [ref][uint32]::Zero)
    if ($thread -eq [IntPtr]::Zero) {
        Write-Host "[ERRO] Não criou thread remota. Código: $([Runtime.InteropServices.Marshal]::GetLastWin32Error())" -ForegroundColor Red
    } else {
        Write-Host "[OK] Thread remota criada: $thread" -ForegroundColor Green
    }

    [NativeMethods]::CloseHandle($hProcess)
    return $true
}

$dllPath = "C:\Users\Administrator\roblox-executor\build\MakotoExecutor.dll"

if (-not (Test-Path $dllPath)) {
    Write-Host "[ERRO] DLL não encontrada: $dllPath" -ForegroundColor Red
    Read-Host "Pressione Enter para sair"
    exit
}

Write-Host ""
Write-Host "  __  __          _          __  __" -ForegroundColor Red
Write-Host " |  \/  |        | |        / _|/ _|" -ForegroundColor Red
Write-Host " | \  / |   ___  | | __ _  | |_| |_   ___" -ForegroundColor Red
Write-Host " | |\/| |  / _ \ | |/ _` | |  _|  _| / __|" -ForegroundColor Red
Write-Host " | |  | | |  __/ | | (_| | | | | |   \__ \" -ForegroundColor Red
Write-Host " |_|  |_|  \___| |_|\__,_| |_| |_|   |___/" -ForegroundColor Red
Write-Host ""
Write-Host "        MAKOTO EXECUTOR - PowerShell Injector" -ForegroundColor Red
Write-Host "        Version: 2.0.0" -ForegroundColor Gray
Write-Host ""

while ($true) {
    $roblox = Get-RobloxProcess
    if ($roblox) {
        Write-Host "[OK] Roblox encontrado! PID: $($roblox.Id) | Nome: $($roblox.ProcessName)" -ForegroundColor Green
        Write-Host ""

        $response = Read-Host "Injetar MAKOTO EXECUTOR? (S/N)"
        if ($response -eq "S" -or $response -eq "s") {
            Write-Host ""
            Write-Host "[*] Injetando..." -ForegroundColor Yellow
            $result = Inject-DLL -ProcessId $roblox.Id -DllPath $dllPath
            if ($result) {
                Write-Host ""
                Write-Host "[SUCESSO] MAKOTO EXECUTOR injetado!" -ForegroundColor Green
                Write-Host ""
                Read-Host "Pressione Enter para sair"
                exit
            } else {
                Write-Host ""
                Write-Host "[FALHA] Tentando novamente..." -ForegroundColor Red
                Start-Sleep -Seconds 2
            }
        } else {
            Write-Host "[*] Aguardando Roblox..." -ForegroundColor Gray
            Start-Sleep -Seconds 2
        }
    } else {
        Write-Host "[*] Aguardando Roblox abrir..." -ForegroundColor Gray
        Start-Sleep -Seconds 2
    }
}
