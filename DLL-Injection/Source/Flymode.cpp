#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <MinHook.h>
#include <cstdio>
#include <thread>
#include <cmath>
#include <string>
#include <iostream>

struct Vector3 { float x, y, z; Vector3() : x(0), y(0), z(0) {} Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {} };
struct MethodInfo {};

const uintptr_t OFFSET_ApplyGravity = 0x3BF6D0;
const uintptr_t OFFSET_GetVelocity = 0x424940;

bool g_FlyEnabled = false;
float g_FlySpeed = 20.0f;

typedef Vector3(__fastcall* tApplyGravity)(void*, Vector3, Vector3, void*, void*);
typedef Vector3(__fastcall* tGetVelocity)(void*, Vector3, Vector3, bool, Vector3, void*);

tApplyGravity origApplyGravity = nullptr;
tGetVelocity origGetVelocity = nullptr;

Vector3 __fastcall HookedApplyGravity(void* _this, Vector3 velocity, Vector3 velocityPrevFrame, void* interactableLocal, void* methodInfo) {
    if (g_FlyEnabled) return Vector3(0.0f, 0.0f, 0.0f);
    return origApplyGravity(_this, velocity, velocityPrevFrame, interactableLocal, methodInfo);
}

Vector3 __fastcall HookedGetVelocity(void* _this, Vector3 velocity, Vector3 movableVelocity, bool inputJump, Vector3 inputDirection, void* methodInfo) {
    if (g_FlyEnabled) {
        Vector3 moveDir(0.0f, 0.0f, 0.0f);
        if (GetAsyncKeyState('W') & 0x8000) moveDir.z += 1;
        if (GetAsyncKeyState('S') & 0x8000) moveDir.z -= 1;
        if (GetAsyncKeyState('A') & 0x8000) moveDir.x -= 1;
        if (GetAsyncKeyState('D') & 0x8000) moveDir.x += 1;
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) moveDir.y += 1;
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000) moveDir.y -= 1;
        float mag = sqrtf(moveDir.x * moveDir.x + moveDir.y * moveDir.y + moveDir.z * moveDir.z);
        if (mag > 0) {
            moveDir.x = (moveDir.x / mag) * g_FlySpeed;
            moveDir.y = (moveDir.y / mag) * g_FlySpeed;
            moveDir.z = (moveDir.z / mag) * g_FlySpeed;
        }
        return moveDir;
    }
    return origGetVelocity(_this, velocity, movableVelocity, inputJump, inputDirection, methodInfo);
}

void InitializeHooks() {
    uintptr_t base = (uintptr_t)GetModuleHandle(L"GameAssembly.dll");
    if (!base) return;
    MH_Initialize();
    MH_CreateHook((LPVOID)(base + OFFSET_ApplyGravity), HookedApplyGravity, (LPVOID*)&origApplyGravity);
    MH_CreateHook((LPVOID)(base + OFFSET_GetVelocity), HookedGetVelocity, (LPVOID*)&origGetVelocity);
    MH_EnableHook(MH_ALL_HOOKS);
}

void ConsoleThread() {
    Sleep(1000);
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONIN$", "r", stdin);
    printf("Fly Type fly to toggle, fly 50 to set speed.\n");
    printf("Fly wasd = move, space = up, ctrl = down.\n");

    std::string cmd;
    while (true) {
        printf("> ");
        std::getline(std::cin, cmd);
        if (cmd == "fly") {
            g_FlyEnabled = !g_FlyEnabled;
            printf("Fly: %s (%.1f)\n", g_FlyEnabled ? "ON" : "OFF", g_FlySpeed);
        }
        else if (cmd.find("fly ") == 0) {
            g_FlySpeed = (float)atof(cmd.substr(4).c_str());
            g_FlyEnabled = true;
            printf("Fly speed: %.1f\n", g_FlySpeed);
        }
        else if (cmd == "exit") break;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        AllocConsole();
        SetConsoleTitle(L"Fly");
        std::thread(ConsoleThread).detach();
        std::thread([]() { Sleep(3000); InitializeHooks(); }).detach();
    }
    return TRUE;
}
