#include <iostream>
#include <string>
#include <fstream>
#include <windows.h>
#include <unordered_map>
#include "clipboard.h"

HHOOK hKeyboardHook; // Global hook handle

bool exitFlag = false;
DWORD mainThreadId;

bool copyFlag = false;
bool pasteFlag = false;
bool copyLock = false;
bool pasteLock = false;
bool charLock = false;

std::unordered_map<DWORD, wchar_t*> clipboard;


// Keyboard Hook Callback Function
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* kbStruct = (KBDLLHOOKSTRUCT*)lParam;
        DWORD vkCode = kbStruct->vkCode;


        if (vkCode == 35 && !copyLock) {
            copyFlag = true;
            copyLock = true;
            pasteLock = true;
            return 1;
        } else if (vkCode == 36 && !pasteLock) {
            pasteFlag = true;
            copyLock = true;
            pasteLock = true;
            return 1;
        } else if (vkCode == 35 || vkCode == 36) {
            return 1;
        } else if (copyFlag && !charLock) {
            charLock = true;
            wchar_t* text = nullptr;
            if (getHighlightedText(text) == 0) {
                clipboard[vkCode] = text;
            }
            copyFlag = false;
            return 1;
        } else if (pasteFlag && !charLock) {
            charLock = true;
            if (clipboard.find(vkCode) != clipboard.end()) {
                wchar_t* text = clipboard[vkCode];
                manualPaste(text);
            }
            pasteFlag = false;
            return 1;
        }

        PostMessage(NULL, WM_USER + 1, wParam, lParam); // Forward the message to the main thread
    } else if (nCode >= 0 && (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)) {
        KBDLLHOOKSTRUCT* kbStruct = (KBDLLHOOKSTRUCT*)lParam;
        DWORD vkCode = kbStruct->vkCode;
        if (vkCode == 35) {
            charLock = false;
            copyFlag = false;
            copyLock = false;
            pasteLock = false;
            return 1;
        } else if (vkCode == 36) {
            charLock = false;
            pasteFlag = false;
            copyLock = false;
            pasteLock = false;
            return 1;
        }
        PostMessage(NULL, WM_USER + 1, wParam, lParam); // Forward the message to the main thread
    }
    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

BOOL WINAPI ConsoleHandler(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_C_EVENT) {
        std::cout << "Ctrl+C pressed. Cleaning up..." << std::endl;
        // Unhook the keyboard hook
        exitFlag = true;
        if (hKeyboardHook) {
            UnhookWindowsHookEx(hKeyboardHook);
            std::cout << "Keyboard hook unhooked!" << std::endl;
        }
        PostThreadMessage(mainThreadId, WM_USER + 2, 0, 0);
        return TRUE;  // Allow the process to exit
    }
    return FALSE;  // Allow other control events to proceed normally
}

void loadClipboard() {
    std::ifstream bin("clipboard.bin", std::ios::binary);
    size_t size;
    if (!bin.is_open()) {
        std::cerr << "Failed to open file for reading." << std::endl;
        return;
    }
    bin.seekg(0, std::ios::end); // Move to the end of the file
    std::streampos length = bin.tellg();
    if (length != 0) {
        bin.seekg(0, std::ios::beg);
        bin.read(reinterpret_cast<char*>(&size), sizeof(size_t));
        if (bin.is_open()) {
            for (size_t i = 0; i < size; i++) {
                DWORD key;
                wchar_t* text;
                size_t pasteSize;
                bin.read(reinterpret_cast<char*>(&key), sizeof(DWORD));
                bin.read(reinterpret_cast<char*>(&pasteSize), sizeof(size_t));
                text = new wchar_t[pasteSize];
                bin.read(reinterpret_cast<char*>(text), pasteSize * sizeof(wchar_t));
                clipboard[key] = text;
            }
            bin.close();
        } else {
            std::cerr << "Failed to open file for reading." << std::endl;
        }
    } else {
        std::cout << "Binary clipboard file is empty, nothing loaded." << std::endl;
        bin.close();
    }
}

int main() {
    mainThreadId = GetCurrentThreadId();

    loadClipboard();
    
    if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
        std::cerr << "Failed to set Ctrl-C handler!" << std::endl;
        return 1;
    }

    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, NULL, 0);
    if (!hKeyboardHook) {
        std::cerr << "Failed to install hook!" << std::endl;
        return 1;
    }

    std::cout << "Global key listener running... CTRL C to exit.\n";

    // Message loop to keep the hook running
    MSG msg;
    while (!exitFlag) {
        if (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    std::wofstream file("clipboard.txt");
    std::ofstream bin("clipboard.bin", std::ios::binary);
    size_t size = clipboard.size();
    if (size != 0) {
        bin.write(reinterpret_cast<const char*>(&size), sizeof(size_t));
        if (file.is_open() && bin.is_open()) {
            for (auto& pair : clipboard) {

                file << pair.first << " " << pair.second << std::endl;

                size_t pasteSize = wcslen(pair.second) + 1;

                bin.write(reinterpret_cast<const char*>(&pair.first), sizeof(DWORD));
                bin.write(reinterpret_cast<const char*>(&pasteSize), sizeof(size_t));
                bin.write(reinterpret_cast<const char*>(pair.second), pasteSize * sizeof(wchar_t));
            }
            file.close();
            bin.close();
        } else {
            std::cerr << "Failed to open file for writing." << std::endl;
            file.close();
            bin.close();
        }
    } else {
        std::cout << "Clipboard is empty, returning without saving." << std::endl;
    }

    // Unhook before exiting
    UnhookWindowsHookEx(hKeyboardHook);
    return 0;

}
