#pragma once
#include <windows.h>
#include <winioctl.h>
#include <iostream>
#include <vector>
#include <tlhelp32.h>

#pragma pack(push, 1)
struct WriteRequest
{
    ULONG64 PhysicalAddress;
    UCHAR Data[8];
};
#pragma pack(pop)

class MemoryDriver
{
private:
    HANDLE DriverHandle;
    DWORD TargetProcessId;

    bool InitializeDriver()
    {
        DriverHandle = CreateFileW(L"\\\\.\\ThrottleStop",
            GENERIC_READ | GENERIC_WRITE,
            0, NULL, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, NULL);
        return DriverHandle != INVALID_HANDLE_VALUE;
    }

    DWORD GetProcessIdByName(const wchar_t* ProcessName)
    {
        HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (Snapshot == INVALID_HANDLE_VALUE)
        {
            std::wcout << L"\n [ - ] Failed To Create Process Snapshot";
            return 0;
        }

        PROCESSENTRY32W Entry;
        Entry.dwSize = sizeof(Entry);

        if (Process32FirstW(Snapshot, &Entry))
        {
            do
            {
                if (_wcsicmp(Entry.szExeFile, ProcessName) == 0)
                {
                    CloseHandle(Snapshot);
                    std::wcout << L"\n [ + ] Found Process: " << ProcessName << L" (PID: " << Entry.th32ProcessID << L")";
                    return Entry.th32ProcessID;
                }
            } while (Process32NextW(Snapshot, &Entry));
        }

        CloseHandle(Snapshot);
        std::wcout << L"\n [ - ] Process Not Found: " << ProcessName;
        return 0;
    }

    std::vector<std::wstring> GetRunningProcesses()
    {
        std::vector<std::wstring> Processes;
        HANDLE Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

        if (Snapshot != INVALID_HANDLE_VALUE)
        {
            PROCESSENTRY32W Entry;
            Entry.dwSize = sizeof(Entry);

            if (Process32FirstW(Snapshot, &Entry))
            {
                do
                {
                    Processes.push_back(Entry.szExeFile);
                } while (Process32NextW(Snapshot, &Entry));
            }
            CloseHandle(Snapshot);
        }

        return Processes;
    }

    bool ReadPhysicalMemoryRaw(ULONG64 Address, PVOID Buffer, SIZE_T Size)
    {
        if (Size > 8)
        {
            std::wcout << L"\n [ - ] ReadPhysical: Size Too Large: " << Size;
            return false;
        }

        ULONG64 PhysAddr = Address;
        DWORD BytesReturned = 0;

        bool Result = DeviceIoControl(DriverHandle,
            0x80006498,
            &PhysAddr, sizeof(PhysAddr),
            Buffer, (DWORD)Size,
            &BytesReturned, NULL);

        if (!Result)
        {
            DWORD Error = GetLastError();
            std::wcout << L"\n [ - ] ReadPhysical Failed: Address=0x" << std::hex << Address
                << L", Size=" << std::dec << Size << L", Error=" << Error;
        }

        return Result && BytesReturned == Size;
    }

    bool WritePhysicalMemoryRaw(ULONG64 Address, PVOID Buffer, SIZE_T Size)
    {
        if (Size > 8) return false;

        UCHAR InputBuffer[16] = { 0 };
        *(ULONG64*)InputBuffer = Address;
        memcpy(InputBuffer + 8, Buffer, Size);

        DWORD BytesReturned = 0;

        bool Result = DeviceIoControl(DriverHandle,
            0x8000649C,
            InputBuffer, 8 + Size,
            nullptr, 0,
            &BytesReturned, NULL);

        if (!Result)
        {
            DWORD Error = GetLastError();
            std::wcout << L"\n [ - ] WritePhysical Failed: Address=0x" << std::hex << Address
                << L", Size=" << std::dec << Size << L", Error=" << Error;
        }

        return Result;
    }

    bool TestDriverCommunication()
    {
        std::wcout << L"\n [ ? ] Testing Driver Communication...";

        ULONG64 TestAddr = 0x1000;
        UCHAR Buffer[8] = { 0 };
        DWORD BytesReturned = 0;

        bool Result = DeviceIoControl(DriverHandle,
            0x80006498,
            &TestAddr, 8,
            Buffer, 8,
            &BytesReturned, NULL);

        if (Result && BytesReturned == 8)
        {
            std::wcout << L"\n [ + ] Driver Communication Working";
            return true;
        }

        std::wcout << L"\n [ - ] Driver Communication Failed";
        return false;
    }

public:
    MemoryDriver() : DriverHandle(INVALID_HANDLE_VALUE), TargetProcessId(0) {}

    ~MemoryDriver()
    {
        if (DriverHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(DriverHandle);
        }
    }

    bool Initialize()
    {
        std::wcout << L"\n [ ? ] Initializing Memory Driver...";

        if (!InitializeDriver())
        {
            std::wcout << L"\n [ - ] Failed To Open Driver Handle";
            return false;
        }

        if (!TestDriverCommunication())
        {
            std::wcout << L"\n [ - ] Driver Communication Test Failed";
            return false;
        }

        std::wcout << L"\n [ + ] Memory Driver Initialized Successfully";
        return true;
    }

    bool SetTargetProcess(const wchar_t* ProcessName)
    {
        std::wcout << L"\n [ ? ] Setting Target Process: " << ProcessName;

        TargetProcessId = GetProcessIdByName(ProcessName);
        if (TargetProcessId == 0)
        {
            return false;
        }

        std::wcout << L"\n [ + ] Target Process Set Successfully";
        return true;
    }

    bool SetTargetProcess(DWORD ProcessId)
    {
        std::wcout << L"\n [ ? ] Setting Target Process By PID: " << ProcessId;
        TargetProcessId = ProcessId;
        std::wcout << L"\n [ + ] Target Process Set Successfully";
        return true;
    }

    DWORD GetTargetProcessId() const
    {
        return TargetProcessId;
    }

    template<typename T>
    T ReadPhysical(ULONG64 Address)
    {
        T Value = {};
        ReadPhysicalMemoryRaw(Address, &Value, sizeof(T));
        return Value;
    }

    template<typename T>
    bool WritePhysical(ULONG64 Address, const T& Value)
    {
        return WritePhysicalMemoryRaw(Address, (PVOID)&Value, sizeof(T));
    }

    bool ReadPhysicalBuffer(ULONG64 Address, PVOID Buffer, SIZE_T Size)
    {
        std::wcout << L"\n [ ? ] Reading Physical Buffer: Address=0x" << std::hex << Address
            << L", Size=" << std::dec << Size;

        UCHAR* ByteBuffer = (UCHAR*)Buffer;
        SIZE_T BytesRead = 0;

        while (BytesRead < Size)
        {
            SIZE_T ChunkSize = min(Size - BytesRead, 8);

            if (!ReadPhysicalMemoryRaw(Address + BytesRead,
                ByteBuffer + BytesRead,
                ChunkSize))
            {
                std::wcout << L"\n [ - ] Failed To Read Chunk At Offset: " << BytesRead;
                return false;
            }

            BytesRead += ChunkSize;
        }

        std::wcout << L"\n [ + ] Successfully Read Physical Buffer";
        return true;
    }

    bool WritePhysicalBuffer(ULONG64 Address, PVOID Buffer, SIZE_T Size)
    {
        std::wcout << L"\n [ ? ] Writing Physical Buffer: Address=0x" << std::hex << Address
            << L", Size=" << std::dec << Size;

        UCHAR* ByteBuffer = (UCHAR*)Buffer;
        SIZE_T BytesWritten = 0;

        while (BytesWritten < Size)
        {
            SIZE_T ChunkSize = min(Size - BytesWritten, 8);

            if (!WritePhysicalMemoryRaw(Address + BytesWritten,
                ByteBuffer + BytesWritten,
                ChunkSize))
            {
                std::wcout << L"\n [ - ] Failed To Write Chunk At Offset: " << BytesWritten;
                return false;
            }

            BytesWritten += ChunkSize;
        }

        std::wcout << L"\n [ + ] Successfully Wrote Physical Buffer";
        return true;
    }
};