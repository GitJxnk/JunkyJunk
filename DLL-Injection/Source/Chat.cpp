#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <MinHook.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <thread>

struct MethodInfo {};

const uintptr_t OFFSET_SendChatMessage = 0x6FBC40;
const uintptr_t OFFSET_il2cpp_string_new_len = 0x2922A0;

typedef void (__fastcall* tSendChatMessage)(void*, void*, void*);
typedef void* (__fastcall* tStringNewLen)(const char*, uint32_t);

tSendChatMessage origSendChatMessage = nullptr;
tStringNewLen il2cpp_string_new_len = nullptr;

wchar_t* GetStringText(void* str) {
    return (wchar_t*)((uintptr_t)str + 0x14);
}

void SpamChat(void* _this, const char* msg, int count, int delay) {
    for (int i = 0; i < count; i++) {
        void* newMsg = il2cpp_string_new_len(msg, (uint32_t)strlen(msg));
        origSendChatMessage(_this, newMsg, NULL);
        if (delay > 0) Sleep(delay);
    }
}

void SpamWide(void* _this, const wchar_t* msg, int count, int delay) {
    char narrow[512] = { 0 };
    WideCharToMultiByte(CP_UTF8, 0, msg, -1, narrow, 512, NULL, NULL);
    SpamChat(_this, narrow, count, delay);
}

// Chat hook
void __fastcall HookedSendChatMessage(void* _this, void* chatMsg, void* methodInfo) {
    wchar_t* text = GetStringText(chatMsg);
    char buffer[512] = { 0 };

    // Spam 10 message
    if (wcsncmp(text, L"/spam ", 6) == 0) {
        wchar_t* rest = text + 6;
        int count = _wtoi(rest);
        if (count > 0 && count <= 50) {
            wchar_t* msgStart = rest;
            while (*msgStart >= L'0' && *msgStart <= L'9') msgStart++;
            if (*msgStart == L' ') msgStart++;
            if (*msgStart) { SpamWide(_this, msgStart, count, 50); return; }
        }
    }

    // rainbow spam
    if (wcsncmp(text, L"/rspam ", 7) == 0) {
        wchar_t* rest = text + 7;
        int count = _wtoi(rest);
        if (count > 0 && count <= 20) {
            wchar_t* msgStart = rest;
            while (*msgStart >= L'0' && *msgStart <= L'9') msgStart++;
            if (*msgStart == L' ') msgStart++;
            if (*msgStart) {
                const char* colors[] = {"#FF0000", "#FF7F00", "#FFFF00", "#00FF00", "#0000FF", "#4B0082", "#9400D3"};
                for (int i = 0; i < count; i++) {
                    sprintf(buffer, "<color=%s>%ls</color>", colors[i % 7], msgStart);
                    void* newMsg = il2cpp_string_new_len(buffer, (uint32_t)strlen(buffer));
                    origSendChatMessage(_this, newMsg, NULL);
                    Sleep(50);
                }
                return;
            }
        }
    }

    // Test message
    if (wcsncmp(text, L"/test ", 6) == 0) {
        wchar_t* msg = text + 6;
        sprintf(buffer, "<color=#FFFF00>[Test]: %ls</color>", msg);
        void* newMsg = il2cpp_string_new_len(buffer, (uint32_t)strlen(buffer));
        origSendChatMessage(_this, newMsg, methodInfo);
        return;
    }

    origSendChatMessage(_this, chatMsg, methodInfo);
}

void InitializeHooks() {
    uintptr_t base = (uintptr_t)GetModuleHandle(L"GameAssembly.dll");
    if (!base) return;

    MH_Initialize();
    il2cpp_string_new_len = (tStringNewLen)(base + OFFSET_il2cpp_string_new_len);
    MH_CreateHook((LPVOID)(base + OFFSET_SendChatMessage), HookedSendChatMessage, (LPVOID*)&origSendChatMessage);
    MH_EnableHook(MH_ALL_HOOKS);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        AllocConsole();
        SetConsoleTitle(L"Bla bla bla");
        std::thread([]() { Sleep(3000); InitializeHooks(); }).detach();
    }
    return TRUE;
}
