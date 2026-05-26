#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <cstdio>
#include <thread>
#include <iostream>
#include <string>

const uintptr_t OFFSET_get_Game = 0x4EA5C0;
const uintptr_t OFFSET_get_OperationRequests = 0x4EAE30; // AdminOperations
const uintptr_t OFFSET_get_LocalPlayer = 0x9897B0;
const uintptr_t OFFSET_get_Count = 0x989830;

typedef void* (__fastcall* tGetGame)(void*);
typedef void* (__fastcall* tGetOpRequests)(void*);
typedef void* (__fastcall* tGetLocalPlayer)(void*, void*);
typedef int (__fastcall* tGetCount)(void*, void*);

void ConsoleThread() {
    Sleep(3000);
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONIN$", "r", stdin);
    
    printf("Type info for server info.\n");

    std::string cmd;
    while (true) {
        printf("> ");
        std::getline(std::cin, cmd);
        
        if (cmd == "info") {
            uintptr_t base = (uintptr_t)GetModuleHandle(L"GameAssembly.dll");
            if (!base) { printf("GameAssembly.dll not found\n"); continue; }
            
            tGetGame getGame = (tGetGame)(base + OFFSET_get_Game);
            tGetOpRequests getOp = (tGetOpRequests)(base + OFFSET_get_OperationRequests);
            tGetCount getCount = (tGetCount)(base + OFFSET_get_Count);
            
            void* game = getGame(NULL);
            void* opRequests = getOp(NULL);
            
            printf("Game: 0x%llX\n", (uintptr_t)game);
            printf("OpRequests: 0x%llX\n", (uintptr_t)opRequests);
            
            if (game) {
                void* pc = *(void**)((uintptr_t)game + 0x1B8);
                printf("PlayerContainer: 0x%llX\n", (uintptr_t)pc);
                if (pc) {
                    int count = getCount(pc, NULL);
                    printf("Player count: %d\n", count);
                }
            }
        }
        else if (cmd == "exit") break;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        AllocConsole();
        SetConsoleTitle(L"Debug Info");
        std::thread(ConsoleThread).detach();
    }
    return TRUE;
}
