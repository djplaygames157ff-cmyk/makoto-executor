#!/usr/bin/env python3
"""
MAKOTO EXECUTOR v2.0 - DLL Injector
"""

import ctypes
import ctypes.wintypes
import os
import sys
import time
import subprocess
from pathlib import Path

PROCESS_ALL_ACCESS = 0x001F0FFF
MEM_COMMIT = 0x00001000
MEM_RESERVE = 0x00002000
PAGE_READWRITE = 0x04
THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER = 0x00000004

kernel32 = ctypes.windll.kernel32
ntdll = ctypes.windll.ntdll


def find_roblox():
    try:
        result = subprocess.run(
            ["tasklist", "/FO", "CSV", "/NH"],
            capture_output=True, text=True, timeout=5
        )
        for line in result.stdout.strip().split("\n"):
            parts = line.split('","')
            if len(parts) >= 2:
                name = parts[0].strip('"')
                if "RobloxPlayer" in name or "WindowsPlayer" in name:
                    pid = int(parts[1].strip('"'))
                    return pid, name
    except Exception:
        pass
    return None, None


def inject_dll(pid, dll_path):
    h_process = kernel32.OpenProcess(PROCESS_ALL_ACCESS, False, pid)
    if not h_process:
        return False

    h_kernel32 = kernel32.GetModuleHandleW("kernel32.dll")
    load_library = kernel32.GetProcAddress(h_kernel32, b"LoadLibraryA")

    dll_bytes = dll_path.encode('ascii') + b'\x00'
    alloc_mem = kernel32.VirtualAllocEx(
        h_process, None, len(dll_bytes),
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE
    )
    if not alloc_mem:
        kernel32.CloseHandle(h_process)
        return False

    written = ctypes.c_size_t(0)
    kernel32.WriteProcessMemory(
        h_process, alloc_mem, dll_bytes, len(dll_bytes),
        ctypes.byref(written)
    )

    thread_id = ctypes.c_ulong(0)
    h_thread = kernel32.CreateRemoteThread(
        h_process, None, 0, load_library, alloc_mem, 0,
        ctypes.byref(thread_id)
    )

    if h_thread:
        kernel32.WaitForSingleObject(h_thread, 5000)
        kernel32.CloseHandle(h_thread)

    kernel32.CloseHandle(h_process)
    return True


def main():
    dll_path = Path(__file__).parent / "build" / "MakotoExecutor.dll"

    # ASCII Art Header
    print("\033[91m")
    print("  __  __          _          __  __")
    print(" |  \\/  |        | |        / _|/ _|")
    print(" | \\  / |   ___  | | __ _  | |_| |_   ___")
    print(" | |\\/| |  / _ \\ | |/ _` | |  _|  _| / __|")
    print(" | |  | | |  __/ | | (_| | | | | |   \\__ \\")
    print(" |_|  |_|  \\___| |_|\\__,_| |_| |_|   |___/")
    print("\033[0m")
    print("\033[97m        MAKOTO EXECUTOR v2.0\033[0m")
    print("\033[90m        DLL Injector for Roblox\033[0m")
    print()

    if not dll_path.exists():
        print("\033[91m[ERROR] DLL not found: " + str(dll_path) + "\033[0m")
        input("Press Enter to exit...")
        return

    print("\033[90m[*] DLL: " + str(dll_path) + "\033[0m")
    print("\033[90m[*] Waiting for Roblox to open...\033[0m")
    print()

    while True:
        pid, name = find_roblox()
        if pid:
            print(f"\033[92m[+] Roblox detected! PID: {pid} | Process: {name}\033[0m")
            print()

            response = input("\033[97m[?] Inject MAKOTO EXECUTOR? (Y/N): \033[0m").strip().lower()
            if response == 'y':
                print()
                print("\033[91m[*] Injecting DLL...\033[0m")
                result = inject_dll(pid, str(dll_path))

                if result:
                    print()
                    print("\033[92m[+] SUCCESS! MAKOTO EXECUTOR injected!\033[0m")
                    print("\033[92m[+] The executor is now active in Roblox.\033[0m")
                    print()
                    input("Press Enter to exit...")
                    return
                else:
                    print()
                    print("\033[91m[-] FAILED! Could not inject DLL.\033[0m")
                    print("\033[90m[*] Try running as Administrator.\033[0m")
                    print()
                    time.sleep(2)
            else:
                print("\033[90m[*] Waiting...\033[0m")
                time.sleep(2)
        else:
            sys.stdout.write("\r\033[90m[*] Waiting for Roblox... (open Roblox to continue)\033[0m")
            sys.stdout.flush()
            time.sleep(2)


if __name__ == "__main__":
    main()
