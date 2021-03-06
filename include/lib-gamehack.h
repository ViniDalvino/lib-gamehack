#pragma once
#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>
#include <string>
// Putting that incase DEFINE_ENUM_FLAG_OPERATORS is deprecated
#ifndef DEFINE_ENUM_FLAG_OPERATORS
#define DEFINE_ENUM_FLAG_OPERATORS(ENUMTYPE)                                                              \
  extern "C++"                                                                                            \
  {                                                                                                       \
    inline ENUMTYPE operator|(ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) | ((int)b)); }           \
    inline ENUMTYPE &operator|=(ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) |= ((int)b)); } \
    inline ENUMTYPE operator&(ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) & ((int)b)); }           \
    inline ENUMTYPE &operator&=(ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) &= ((int)b)); } \
    inline ENUMTYPE operator~(ENUMTYPE a) { return ENUMTYPE(~((int)a)); }                                 \
    inline ENUMTYPE operator^(ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) ^ ((int)b)); }           \
    inline ENUMTYPE &operator^=(ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) ^= ((int)b)); } \
  }
#endif

/**
* Read a variable at address in which you can use as a variable. I recommend to only use this in DLLs
* 
* @example
* DWORD value = READ_ADDRESS(0xDEADBEEF); // read a DWORD from adr
* READ_ADDRESS(0xDEADBEEF) = 1234;        // write 1234 to DWORD adr
*/
#define READ_ADDRESS(addr, type) *((type *)addr)
namespace libGameHack
{

  enum class ProtectMemoryArg : DWORD
  {
    NoAcess = 0x01,
    ReadOnly = 0x02,
    ReadWrite = 0x04,
    WriteCopy = 0x08,
    Execute = 0x10,
    ExecuteRead = 0x20,
    ExecuteReadWrite = 0x40,
    Guard = 0x100,
    NoCache = 0x200,
    WriteCombine = 0x400
  };

  enum class DesiredAcess : DWORD
  {
    /**
     * The returned handle can be used with 
     * VirtualAllocEx(), VirtualFreeEx(), and VirtualProtectEx() to allocate, 
     * free, and protect chunks of memory, respectively.
    */
    VmOperation = PROCESS_VM_OPERATION,
    /**
     * The returned handle can be used with 
     * ReadProcessMemory().
    */
    VmRead = PROCESS_VM_READ,
    /**
     * The returned handle can be used with 
     * WriteProcessMemory(), but it must also have PROCESS_VM_OPERATION rights. 
     * You can set both flags by passing PROCESS_VM_OPERATION | PROCESS_VM_WRITE 
     * as the DesiredAccess parameter.
    */
    VmWrite = PROCESS_VM_WRITE,
    /**
     * The returned handle can be used with 
     * CreateRemoteThread().
    */
    CreateThread = PROCESS_CREATE_THREAD,
    /**
     * The returned handle can be used to do anything. 
     * Avoid using this flag, as it can only be used by processes with debug 
     * privileges enabled and has compatibility issues with older versions of 
     * Windows.
    */
    AllAcess = PROCESS_ALL_ACCESS,
  };

  /**
   * Search for the process id from a certain exe name. This function will return NULL if it can't find the pid
   * @example get_pid_from_bin_name(L"game.exe") 
   * @param exeName the name of the exe.
  */
  DWORD fetch_pid_from_bin_name(std::wstring exeName);

  /**
  * Fetch the pid of a process from it's window. The function will return NULL if it can't find the pid
  * @param window_name The name of the window. 
  * @example fetch_pid_from_window_name(L"METAL GEAR SOLID 5: THE PHANTOM PAIN")
  */
  DWORD fetch_pid_from_window_name(std::wstring window_name);

  /**
   * Fetch the handle of a proces. This function will return NULL if it fail to fetch the process memory
  */
  HANDLE fetch_proces_handle(DWORD pid, DesiredAcess desiredAcess = (DesiredAcess)(static_cast<DWORD>(DesiredAcess::VmRead) | static_cast<DWORD>(DesiredAcess::VmWrite) | static_cast<DWORD>(DesiredAcess::VmOperation)), BOOL inheritHandle = FALSE);

  template <typename T>
  T readMemory(HANDLE process, LPVOID address);

  template <typename T>
  T readMemory(HANDLE process, DWORD address);

  /** Used to read memory when the cheat is a dll */
  template <typename T>
  T readMemory(LPVOID adr);

  template <typename T>
  T readMemory(DWORD adr);

  template <typename T>
  ProtectMemoryArg protectMemory(HANDLE proc, LPVOID adr, ProtectMemoryArg prot);
  template <typename T>
  ProtectMemoryArg protectMemory(HANDLE proc, DWORD adr, ProtectMemoryArg prot);

  /**
   * Dynamically rebase addresses at runtime -- Bypass aslr in production
   * @param process The handle of the process of the game
   * @param address Base address of the game 
  */
  DWORD rebase(HANDLE process, DWORD address);

  void showHowToDisableAslr();

  DWORD GetProcessThreadID(HANDLE Process);

  /**
  * Used to write memory when the cheat is a dll
  */
  template <typename T>
  void writeMemory(LPVOID adr, T val);

  template <typename T>
  void writeMemory(HANDLE proc, LPVOID adr, T val);

  /**
  * Used to point memory when the cheat is a dll
  */
  template <typename T>
  T *pointMemory(LPVOID adr);

  template <typename T>
  T *pointMemory(DWORD adr);


  /**
   * Write Nop to a range of data
   */
  template <int SIZE>
  void writeNop(DWORD address);

  /**
   * Write Nop to a range of data
   */
  template <int SIZE>
  void writeNop(HANDLE proc, DWORD address);

  /**
   * Hook the call o a function
   * @param proc The process that the function to hook is in
   * @param hookAt The address of the function to hook
   * @param newFunc The function that is going to be replaced. Example: (DWORD)&someNewFunction
   * @return The function that you hooked
   */
  DWORD callHook(HANDLE proc, DWORD hookAt, DWORD newFunc);

  uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t *modName);

  /**
   * Hook an virtual function. 
  */
  DWORD hookVF(HANDLE process, DWORD classInst, DWORD funcIndex, DWORD newFunc);

  /**
   * Hook an imported function. This function only work with dll mods
   * @param funcName The function name of the function to hook
   * @param newFunc An pointer to the function that is going to hook the function
   * @return The old function
   * @example
   * VOID WINAPI newSleepFunction(DWORD ms)
   * {
   *   // do thread-sensitive things
   *     originalSleep(ms);
   * }
   * 
   * typedef VOID (WINAPI _origSleep)(DWORD ms);
   * _origSleep* originalSleep = (_origSleep*)hookIAT("Sleep", (DWORD)&newSleepFunction);
  */
  DWORD hookIAT(const char *funcName, DWORD newFunc);
} // namespace libGameHack