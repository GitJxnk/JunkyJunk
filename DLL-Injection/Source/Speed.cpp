#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <MinHook.h>
#include <cstdio>
#include <thread>
#include <string>
#include <iostream>

struct Vector3 { float x, y, z; };
struct MethodInfo {};

const uintptr_t OFFSET_GetSpeed = 0x4257A0; // AvatarMotor

bool g_SpeedHackEnabled = false;
float g_SpeedMultiplier = 2.0f;

typedef float (__fastcall* tGetSpeed)(void*, float, Vector3, void*);
tGetSpeed origGetSpeed = nullptr;

float __fastcall HookedGetSpeed(void* _this, float currentSpeed, Vector3 inputDirection, void* methodInfo) {
    float s = origGetSpeed(_this, currentSpeed, inputDirection, methodInfo);
    return g_SpeedHackEnabled ? s * g_SpeedMultiplier : s;
}

void InitializeHooks() {
    uintptr_t base = (uintptr_t)GetModuleHandle(L"GameAssembly.dll");
    if (!base) return;
    MH_Initialize();
    MH_CreateHook((LPVOID)(base + OFFSET_GetSpeed), HookedGetSpeed, (LPVOID*)&origGetSpeed);
    MH_EnableHook(MH_ALL_HOOKS);
}

void ConsoleThread() {
    Sleep(1000);
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONIN$", "r", stdin);
    printf("Type speed to toggle, speed 5 to set multiplier.\n");

    std::string cmd;
    while (true) {
        printf("> ");
        std::getline(std::cin, cmd);
        if (cmd == "speed") {
            g_SpeedHackEnabled = !g_SpeedHackEnabled;
            printf("Speed: %s (x%.1f)\n", g_SpeedHackEnabled ? "ON" : "OFF", g_SpeedMultiplier);
        }
        else if (cmd.find("speed ") == 0) {
            g_SpeedMultiplier = (float)atof(cmd.substr(6).c_str());
            g_SpeedHackEnabled = true;
            printf("Speed set to x%.1f\n", g_SpeedMultiplier);
        }
        else if (cmd == "exit") break;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        AllocConsole();
        SetConsoleTitle(L"speed");
        std::thread(ConsoleThread).detach();
        std::thread([]() { Sleep(3000); InitializeHooks(); }).detach();
    }
    return TRUE;
}
