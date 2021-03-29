#include <iostream>
#include <cmath>
#include <windows.h>
#include <TlHelp32.h>
#include <processthreadsapi.h>

#define M_PI 3.14159265358979323846264338327950288

struct Vec3 {
    float x, y, z;
} HeadPos, EnemyPos, RelativePos;

uintptr_t GetModuleBaseAddr(DWORD procID, const wchar_t* modName)
{
    uintptr_t modBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procID);
    if (hSnap != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32First(hSnap, &modEntry))
        {
            do
            {
                if (!_wcsicmp(modEntry.szModule, modName)) {
                    modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32Next(hSnap, &modEntry));
        }
    }
    CloseHandle(hSnap);
    return modBaseAddr;
}

DWORD FindPID(const std::wstring& modName)
{
    PROCESSENTRY32 processInfo;
    processInfo.dwSize = sizeof(processInfo);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    Process32First(snapshot, &processInfo);
    if (!modName.compare(processInfo.szExeFile))
    {
        CloseHandle(snapshot);
        return processInfo.th32ProcessID;
    }

    while (Process32Next(snapshot, &processInfo))
    {
        if (!modName.compare(processInfo.szExeFile))
        {
            CloseHandle(snapshot);
            return processInfo.th32ProcessID;
        }
    }

    CloseHandle(snapshot);
    return 0;
}

int main()
{
    EnemyPos = { 161, 158, 20 };

    DWORD procID = FindPID(L"ac_client.exe");
    std::cout << "PID: " << procID << std::endl;

    uintptr_t moduleBase = GetModuleBaseAddr(procID, L"ac_client.exe");
    std::cout << "Module Base Address: 0x" << std::hex << moduleBase << std::endl;

    HANDLE hProcess = 0;
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, procID);

    uintptr_t LocalPlayer;
    ReadProcessMemory(hProcess, (LPCVOID)(moduleBase + 0x10F4F4), &LocalPlayer, sizeof(LocalPlayer), nullptr);
    std::cout << "LocalPlayer Base Address: 0x" << std::hex << LocalPlayer << std::endl;

    while (true) {
        // Read x,y,z head pos of local player
        ReadProcessMemory(hProcess, (LPCVOID)(LocalPlayer + 0x0004), &HeadPos, sizeof(HeadPos), nullptr);

        // Transform enemy pos to make head pos relative to origin
        RelativePos.x = EnemyPos.x - HeadPos.x;
        RelativePos.y = EnemyPos.y - HeadPos.y;
        RelativePos.z = EnemyPos.z - HeadPos.z;

        const float hypotenuse = sqrt(pow(RelativePos.x, 2) + pow(RelativePos.y, 2) + pow(RelativePos.z, 2));
        
        const float yaw = (atan2(RelativePos.y, RelativePos.x) * 180/M_PI) + 90;
        const float pitch = atan2(RelativePos.z, hypotenuse) * 180/M_PI;

        WriteProcessMemory(hProcess, (LPVOID)(LocalPlayer + 0x0040), &yaw, sizeof(yaw), nullptr);
        WriteProcessMemory(hProcess, (LPVOID)(LocalPlayer + 0x0044), &pitch, sizeof(pitch), nullptr);
    }
}