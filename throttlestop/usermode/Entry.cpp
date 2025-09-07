#include "driver/driver.h"

int main()
{
    MemoryDriver Driver;

    if (!Driver.Initialize())
    {
        std::wcout << L"\n [ - ] Failed To Initialize Driver";
        Sleep(2000);
        return 1;
    }

    if (!Driver.SetTargetProcess(L"notepad.exe"))
    {
        std::wcout << L"\n [ - ] Failed To Set Target Process";
        Sleep(2000);
        return 1;
    }

    std::wcout << L"\n [ # ] Current Target PID: " << Driver.GetTargetProcessId();

    std::wcout << L"\n [ ? ] Testing Multiple Read Addresses...";
    ULONG64 TestAddresses[] = { 0x1000, 0x2000, 0x10000, 0x100000, 0x500000 };

    for (auto Addr : TestAddresses)
    {
        ULONG64 Value = Driver.ReadPhysical<ULONG64>(Addr);
        std::wcout << L"\n [ # ] Address 0x" << std::hex << Addr << L": 0x" << Value;
    }

    std::wcout << L"\n [ ? ] Testing Write To Physical Memory...";
    ULONG64 OriginalValue = Driver.ReadPhysical<ULONG64>(0x1000);
    std::wcout << L"\n [ # ] Original Value At 0x1000: 0x" << std::hex << OriginalValue;

    if (Driver.WritePhysical<ULONG64>(0x1000, 0x1337))
    {
        ULONG64 NewValue = Driver.ReadPhysical<ULONG64>(0x1000);
        std::wcout << L"\n [ # ] After Write: 0x" << std::hex << NewValue;

        if (Driver.WritePhysical<ULONG64>(0x1000, OriginalValue))
        {
            std::wcout << L"\n [ + ] Restored Original Value";
        }
    }

    std::wcout << L"\n [ + ] All Tests Completed For Target Process: " << Driver.GetTargetProcessId();
    Sleep(2000);
    return 0;
}