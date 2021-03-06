#include "lib-gamehack.h"
#include <tlhelp32.h>
#include <iostream>

namespace libGameHack
{

  DWORD fetch_pid_from_bin_name(std::wstring exeName)
  {
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (Process32First(snapshot, &entry) == TRUE)
    {
      while (Process32Next(snapshot, &entry) == TRUE)
      {
        std::wstring binPath = entry.szExeFile;
        if (binPath.find(exeName) != std::wstring::npos)
        {
          CloseHandle(snapshot);
          return entry.th32ProcessID;
        }
      }
    }

    CloseHandle(snapshot);
    return NULL;
  }

  DWORD fetch_pid_from_window_name(std::wstring window_name)
  {
    HWND gameWindow = FindWindowW(NULL, window_name.c_str());
    DWORD pid;
    GetWindowThreadProcessId(gameWindow, &pid);
    return pid;
  }

  HANDLE fetch_proces_handle(DWORD pid, DesiredAcess desiredAcess, BOOL inheritHandle)
  {
    HANDLE procesHandle = OpenProcess(static_cast<DWORD>(desiredAcess), inheritHandle, pid);

    if (procesHandle == INVALID_HANDLE_VALUE)
      return NULL;
    else
      return procesHandle;
  }

  template <typename T>
  T readMemory(HANDLE process, LPVOID address)
  {
    T val;
    ReadProcessMemory(process, address, &val, sizeof(T), NULL);
    return val;
  }

  template <typename T>
  T readMemory(HANDLE process, DWORD address)
  {
    T val;
    ReadProcessMemory(process, (LPVOID)address, &val, sizeof(T), NULL);
    return val;
  }

  template <typename T>
  void writeMemory(HANDLE proc, LPVOID adr, T val)
  {
    WriteProcessMemory(proc, adr, &val, sizeof(T), NULL);
  }

  template <typename T>
  void writeMemory(HANDLE proc, DWORD adr, T val)
  {
    WriteProcessMemory(proc, (LPVOID)adr, &val, sizeof(T), NULL);
  }

  template <typename T>
  void writeMemory(LPVOID adr, T val)
  {
    *((T *)adr) = val;
  }

  template <typename T>
  ProtectMemoryArg protectMemory(HANDLE proc, LPVOID adr, ProtectMemoryArg prot)
  {
    DWORD oldProt;
    if (T == void || T == void *)
      VirtualProtectEx(proc, adr, 0, prot, &oldProt);
    else
      VirtualProtectEx(proc, adr, sizeof(T), prot, &oldProt);
    return static_cast<ProtectMemoryArg>(oldProt);
  }

  template <typename T>
  ProtectMemoryArg protectMemory(HANDLE proc, DWORD adr, ProtectMemoryArg prot)
  {
    DWORD oldProt;
    VirtualProtectEx(proc, (LPVOID)adr, sizeof(T), static_cast<DWORD>(prot), &oldProt);
    return static_cast<ProtectMemoryArg>(oldProt);
  }

  DWORD rebase(HANDLE process, DWORD address)
  {
    DWORD newBase;
    // get the address of kernel32.dll
    HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
    // get the address of GetModuleHandle()
    LPVOID funcAdr = GetProcAddress(k32, "GetModuleHandleA");
    if (!funcAdr)
      funcAdr = GetProcAddress(k32, "GetModuleHandleW");
    // create the thread
    HANDLE thread = CreateRemoteThread(process, NULL, NULL, (LPTHREAD_START_ROUTINE)funcAdr, NULL, NULL, NULL);

    // let the thread finish
    WaitForSingleObject(thread, INFINITE);
    // get the exit code
    GetExitCodeThread(thread, &newBase);
    // clean up the thread handle
    CloseHandle(thread);

    DWORD diff = address - 0x400'000;
    return diff + newBase;
  }

  void showHowToDisableAslr()
  {
    const char *instruction = "To keep development simple, you can disable ASLR and use addresses with "
                              "the transparent XP-base. To do so, enter a single command in the Visual "
                              "Studio Command Prompt:"
                              "\n"
                              "\t> editbin /DYNAMICBASE:NO \"C:\\path\\to\\game.exe\""
                              "\n"
                              "To renable it, enter:"
                              "\n"
                              "\t> editbin /DYNAMICBASE \"C:\\path\\to\\game.exe\"";
    if (GetActiveWindow() != NULL)
      MessageBoxA(NULL, instruction, "How to disable ASLR", MB_ICONASTERISK);
    else
      std::cout << instruction << "\n";
  }

  DWORD GetProcessThreadID(HANDLE Process)
  {
    THREADENTRY32 entry;
    entry.dwSize = sizeof(THREADENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (Thread32First(snapshot, &entry) == TRUE)
    {
      DWORD PID = GetProcessId(Process);
      while (Thread32Next(snapshot, &entry) == TRUE)
      {
        if (entry.th32OwnerProcessID == PID)
        {
          CloseHandle(snapshot);
          return entry.th32ThreadID;
        }
      }
    }
    CloseHandle(snapshot);
    return NULL;
  }

  template <typename T>
  T readMemory(LPVOID adr)
  {
    return *((T *)adr);
  }

  template <typename T>
  T readMemory(DWORD adr)
  {
    return *((T *)adr);
  }

  template <typename T>
  T *pointMemory(LPVOID adr)
  {
    return ((T *)adr);
  }

  template <typename T>
  T *pointMemory(DWORD adr)
  {
    return ((T *)adr);
  }

  template <int SIZE>
  void writeNop(DWORD address)
  {
    auto oldProtection =
        protectMemory<BYTE[SIZE]>(address, PAGE_EXECUTE_READWRITE);
    for (int i = 0; i < SIZE; i++)
      writeMemory<BYTE>(address + i, 0x90);
    protectMemory<BYTE[SIZE]>(address, oldProtection);
  }

  template <int SIZE>
  void writeNop(HANDLE proc, DWORD address)
  {
    auto oldProtection =
        protectMemory<BYTE[SIZE]>(handle, address, PAGE_EXECUTE_READWRITE);
    for (int i = 0; i < SIZE; i++)
      writeMemory<BYTE>(handle, address + i, 0x90);
    protectMemory<BYTE[SIZE]>(handle, address, oldProtection);
  }

  DWORD callHook(HANDLE proc, DWORD hookAt, DWORD newFunc)
  {
    DWORD newOffset = newFunc - hookAt - 5;

    auto oldProtection = protectMemory<DWORD>(proc, hookAt + 1, ProtectMemoryArg::ExecuteReadWrite);

    DWORD originalOffset = readMemory<DWORD>(proc, hookAt + 1);
    writeMemory<DWORD>(proc, hookAt + 1, newOffset);
    protectMemory<DWORD>(proc, hookAt + 1, oldProtection);

    return originalOffset + hookAt + 5;
  }

  uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t *modName)
  {
    uintptr_t modBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnap != INVALID_HANDLE_VALUE)
    {
      MODULEENTRY32 modEntry;
      modEntry.dwSize = sizeof(modEntry);
      if (Module32First(hSnap, &modEntry))
      {
        do
        {
          if (!_wcsicmp(modEntry.szModule, modName))
          {
            modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
            break;
          }
        } while (Module32Next(hSnap, &modEntry));
      }
    }
    CloseHandle(hSnap);
    return modBaseAddr;
  }

  DWORD hookVF(HANDLE process, DWORD classInst, DWORD funcIndex, DWORD newFunc)
  {
    DWORD VFTable = readMemory<DWORD>(process, classInst);
    DWORD hookAt = VFTable + funcIndex * sizeof(DWORD);

    auto oldProtection = protectMemory<DWORD>(process, hookAt, ProtectMemoryArg::ReadWrite);
    DWORD originalFunc = readMemory<DWORD>(process, hookAt);
    writeMemory<DWORD>(process, hookAt, newFunc);
    protectMemory<DWORD>(process, hookAt, oldProtection);
    return originalFunc;
  }

  DWORD hookIAT(const char *funcName, DWORD newFunc)
  {
    DWORD baseAddr = (DWORD)GetModuleHandle(NULL);
    HANDLE proc = GetModuleHandle(NULL);

    auto dosHeader = pointMemory<IMAGE_DOS_HEADER>(baseAddr);
    if (dosHeader->e_magic != 0x5A4D)
      return 0;
    auto optHeader = pointMemory<IMAGE_OPTIONAL_HEADER>(baseAddr + dosHeader->e_lfanew + 24);
    if (optHeader->Magic != 0x10B)
      return 0;
    auto IAT =
        optHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (IAT.Size == 0 || IAT.VirtualAddress == 0)
      return 0;

    auto impDesc = pointMemory<IMAGE_IMPORT_DESCRIPTOR>(baseAddr + IAT.VirtualAddress);
    while (impDesc->FirstThunk)
    {
      auto thunkData = pointMemory<IMAGE_THUNK_DATA>(baseAddr + impDesc->OriginalFirstThunk);
      int n = 0;
      while (thunkData->u1.Function)
      {
        char *importFuncName = pointMemory<char>(baseAddr + (DWORD)thunkData->u1.AddressOfData + 2);
        if (strcmp(importFuncName, funcName) == 0)
        {
          auto vfTable = pointMemory<DWORD>(baseAddr + impDesc->FirstThunk);
          DWORD original = vfTable[n];
          auto oldProtection =
              protectMemory<DWORD>(proc, (DWORD)&vfTable[n], ProtectMemoryArg::ReadWrite);
          vfTable[n] = newFunc;
          protectMemory<DWORD>(proc, (DWORD)&vfTable[n], oldProtection);
          return original;
        }
        n++;
        thunkData++;
      }
      impDesc++;
    }
  }
}