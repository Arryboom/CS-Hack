//#include <stdafx.h>

#pragma comment(lib,"OpenGL32.lib")
#pragma comment(lib,"GLu32.lib")

#include <windows.h>
#include <string>
#include <tlhelp32.h>
#include <fstream>
#include <cstdio>
#include <gl\gl.h>
#include <gl\glu.h>

//=================================================================================
// ###################### Hooked OpenGL Functions #################################
//=================================================================================
typedef void (APIENTRY *glBegin_t)(GLenum);
typedef void (APIENTRY *glVertex3fv_t)(const GLfloat *v);
typedef void (APIENTRY *glVertex2f_t)(GLfloat x, GLfloat y);

glBegin_t pglBegin = NULL;
glVertex3fv_t pglVertex3fv = NULL;
glVertex2f_t pglVertex2f = NULL;


bool wallhack = false;
bool antiFlash = false;
bool isFlash = false;
bool antiSmoke = false;
bool isSmoke = false;

GLfloat colors[4];
GLfloat coords[4];

void APIENTRY Hooked_glBegin(GLenum mode) {
  if (GetAsyncKeyState(VK_F1) & 1) wallhack = !wallhack;
  if (GetAsyncKeyState(VK_F2) & 1) antiFlash = !antiFlash;
  if (GetAsyncKeyState(VK_F3) & 1) antiSmoke = !antiSmoke;

  if (wallhack) {
    if (mode == GL_TRIANGLES || mode == GL_TRIANGLE_STRIP || mode == GL_TRIANGLE_FAN) // Human and some objects
      glDepthRange(0, 0.5);
    else
      glDepthRange(0.5, 1);
  }

  if (antiFlash) {
    if (mode == GL_QUADS) { // Flash is a full-screen quads with white color
      glGetFloatv(GL_VIEWPORT, coords);
      glGetFloatv(GL_CURRENT_COLOR, colors);
      isFlash = colors[0] == 1 && colors[1] == 1 && colors[2] == 1;
    }
  }

  if (antiSmoke) {
    if (mode == GL_QUADS) { // Smoke is the gray quad
      glGetFloatv(GL_VIEWPORT, coords);
      glGetFloatv(GL_CURRENT_COLOR, colors);
      isSmoke = colors[0] == colors[1] && colors[1] == colors[2] && colors[0] != 0 && colors[0] != 1;
    }
  }

  if (pglBegin)
    (*pglBegin)(mode);
}

void APIENTRY Hooked_glVertex3fv(GLfloat *v) {
  if (antiSmoke && isSmoke) return;
  (*pglVertex3fv)(v);
}

void APIENTRY Hooked_glVertex2f(GLfloat x, GLfloat y) {
  if (antiFlash && y == coords[3]) {
    glColor4f(1, 1, 1, 0.01f);
  }
  (*pglVertex2f)(x, y);
}

void *inject(BYTE *entry, const BYTE *target, const int len) {
  BYTE *new_entry = (BYTE*)malloc(len + 5); // new memory for save the first statement of the original function
  DWORD dwback;
  VirtualProtect(entry, len, PAGE_READWRITE, &dwback); // make the memory writable
  memcpy(new_entry, entry, len); // save the first statement of the original function, since we'll overrite it later
  new_entry += len;
  new_entry[0] = 0xE9; // jmp
  *(DWORD*)(new_entry + 1) = (DWORD)(entry + len - new_entry) - 5; // jmp back to the original function
  entry[0] = 0xE9; // jmp
  *(DWORD*)(entry + 1) = (DWORD)(target - entry) - 5; // jmp to the target fucntion
  VirtualProtect(entry, len, dwback, &dwback); // restore memory status
  return (new_entry - len); // return new entry
}

void HookGL() {
  HMODULE hOpenGL = GetModuleHandle(L"opengl32.dll");
  pglVertex2f = (glVertex2f_t)inject((LPBYTE)GetProcAddress(hOpenGL, "glVertex2f"), (LPBYTE)&Hooked_glVertex2f, 6);
  pglVertex3fv = (glVertex3fv_t)inject((LPBYTE)GetProcAddress(hOpenGL, "glVertex3fv"), (LPBYTE)&Hooked_glVertex3fv, 6);
  pglBegin = (glBegin_t)inject((LPBYTE)GetProcAddress(hOpenGL, "glBegin"), (LPBYTE)&Hooked_glBegin, 6);
}

//=================================================================================
// ###################### Hooked mp.dll Functions #################################
//=================================================================================
typedef void (APIENTRY *takeDamage_t)(void* player, void* player2, float damage, DWORD d);
typedef void (APIENTRY *playerDeathThink_t)(void);

takeDamage_t takeDamage = NULL;
playerDeathThink_t playerDeathThink = NULL;

DWORD player_address;
HANDLE cs_hProcess;

void infinite_hegrenade(HMODULE hMpDLL) {
  const int mp_weapon_hegrenade = 0x10006FC0; // weapon_hegrenade absolute addr in mp.dll
  const int mp_dec_ecx_hegrenade = 0x1000753A; // exact statement's absolute addr in mp.dll
  // where OS actually load the function
  LPBYTE start_addr = (LPBYTE)GetProcAddress(hMpDLL, "weapon_hegrenade");
  LPBYTE target_addr;
  DWORD dwback;
  if (start_addr) {
    // where OS actually load the exact statement
    // inferred by the offset between absolute addr
    target_addr = start_addr + (mp_dec_ecx_hegrenade - mp_weapon_hegrenade);
    // make the memory writeable
    VirtualProtect(target_addr, 1, PAGE_READWRITE, &dwback);
    // overwrite the original statement with nop
    *target_addr = 0x90;
    // restore memory permission
    VirtualProtect(target_addr, 1, dwback, &dwback);
  } else {
    MessageBox(NULL, TEXT("failed to get weapon_hegrenade start_addr"), NULL, MB_ICONEXCLAMATION | MB_OK);
  }
}

void unbroken_armor(HMODULE hMpDLL) {
  // same stuff as in infinite_hegrenade
  const int mp_player = 0x1008DF80;
  const int mp_hurt_armor = 0x1008F2ED;
  int i;
  int len = 4;
  LPBYTE start_addr = (LPBYTE)GetProcAddress(hMpDLL, "player");
  LPBYTE target_addr;
  DWORD dwback;
  if (start_addr) {
    target_addr = start_addr + (mp_hurt_armor - mp_player);
    VirtualProtect(target_addr, len, PAGE_READWRITE, &dwback);
    // overwrite, but this time is 4-byte-long
    for (i = 0; i < len; ++i) {
      *(target_addr + i) = 0x90;
    }
    VirtualProtect(target_addr, len, dwback, &dwback);
  } else {
    MessageBox(NULL, TEXT("failed to get player start_addr"), NULL, MB_ICONEXCLAMATION | MB_OK);
  }
}

bool is_player(void* player) {
  DWORD pev = (DWORD)player;
  return player_address == pev;
}

void Hooked_takeDamage(void* player, void* player2, float damage, int d) {
  // protect registers
  __asm {
    pushad
  }
  // turn up damage
  if (is_player(player)) {
    damage = 512.0;
  }
  // jump back
  // shoudln't use call, since the anonymous process in DLL
  // doesn't obey call-ret routine(push ebp & mov esp, ebp, etc;)
  __asm {
    popad
    mov esp, ebp
    pop ebp
    jmp takeDamage
  }
}

void unbroken_man(HMODULE hMpDLL) {
  // same stuff as usual
  const int mp_player = 0x1008DF80;
  const int mp_damage_entry = 0x1008EFE0;
  LPBYTE start_addr = (LPBYTE)GetProcAddress(hMpDLL, "player");
  LPBYTE entry_addr = start_addr + (mp_damage_entry - mp_player);
  // get the jump-back-address after injection
  takeDamage = (takeDamage_t)inject(entry_addr, (LPBYTE)&Hooked_takeDamage, 8);
}

void APIENTRY Hooked_PlayerDeathThink(void) {
  // Infact it is not playerDeathThink, just an anonymous process in 0x10097350
  __asm {
    pushad
  }
  float hp = 1.0f;
  float god = 0.0f;
  WriteProcessMemory(cs_hProcess, (DWORD*)(player_address + 0x160), (LPVOID)&hp, 4, NULL);
  WriteProcessMemory(cs_hProcess, (DWORD*)(player_address + 0x16C), (LPVOID)&god, 4, NULL);
  __asm {
    popad
    mov esp, ebp
    pop ebp
    jmp playerDeathThink
  }
}

void god_like(HMODULE hMpDLL) {
  const int mp_weapon_shield = 0x10093170;
  const int mp_change_god = 0x1009742C;
  // this function will be called by every player and bot after a new game
  //  0x10097426 is the op changing the takeDamage， 0x1009742C is the next command with 3 + 5 byte
  LPBYTE start_addr = (LPBYTE)GetProcAddress(hMpDLL, "weapon_shield");
  LPBYTE entry_addr = start_addr + (mp_change_god - mp_weapon_shield);
  if (start_addr == NULL) {
    MessageBox(NULL, TEXT("weapon_shield is NULL!"), NULL, MB_ICONEXCLAMATION | MB_OK);
  }
  playerDeathThink = (playerDeathThink_t)inject(entry_addr, (LPBYTE)&Hooked_PlayerDeathThink, 8);
}

void getPlayerAddress() {
  // player_address == [[[cstrike.exe+11069BC]+7c]+4]
  // player_hp = [player_address + 0x160]
  // player_god = [player_address + 0x16C]
  HWND cs_hwnd = FindWindow(NULL, L"Counter-Strike");
  DWORD cs_dwPID;
  GetWindowThreadProcessId(cs_hwnd, &cs_dwPID);
  cs_hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, cs_dwPID);
  DWORD ptr = (0x11069BC + 0x1400000);
  DWORD ptr1, ptr2;
  ReadProcessMemory(cs_hProcess, (DWORD*)ptr, &ptr1, 4, NULL);
  ptr1 += 0x7c;
  ReadProcessMemory(cs_hProcess, (DWORD*)ptr1, &ptr2, 4, NULL);
  ptr2 += 0x4;
  ReadProcessMemory(cs_hProcess, (DWORD*)ptr2, &player_address, 4, NULL);
}

void HookMP() {
  HMODULE hMpDLL = GetModuleHandle(L"mp.dll");
  if (hMpDLL) {
    infinite_hegrenade(hMpDLL);
    unbroken_armor(hMpDLL);
    unbroken_man(hMpDLL);
    getPlayerAddress();
    god_like(hMpDLL);
  } else {
    MessageBox(NULL, TEXT("hMpDLL is NULL!"), NULL, MB_ICONEXCLAMATION | MB_OK);
  }
}

//=================================================================================
// ###################### hack in #################################
//=================================================================================
DWORD WINAPI dwMainThread(LPVOID) {
  HookGL();
  HookMP();
  return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD dwReason, LPVOID lpReserved) {
  if (dwReason == DLL_PROCESS_ATTACH)
    CreateThread(0, 0, dwMainThread, 0, 0, 0);

  return TRUE;
}

