#include <string>
#include <iostream>
#include <unordered_map>
#include <windows.h>
#include "clipboard.h"

int getClipboardText(wchar_t*& text) {
    // Open the clipboard
    if (!OpenClipboard(nullptr)) {
        std::cerr << "Failed to open the clipboard." << std::endl;
        return 1;
    }

    // Get the clipboard data (assuming it's in text format)
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData == nullptr) {
        std::cerr << "Failed to get clipboard data." << std::endl;
        CloseClipboard();
        return 1;
    }
    wchar_t* clipboardText = static_cast<wchar_t*>(GlobalLock(hData));

    if (clipboardText == nullptr) {
        std::cerr << "Failed to lock clipboard data." << std::endl;
        CloseClipboard();
        return 1;
    }


    size_t textLength = wcslen(clipboardText) + 1;
    try {
        text = new wchar_t[textLength];
        wcscpy(text, clipboardText);
    } catch (std::exception& e) {
        std::cout << "Failed to allocate memory for text: " << e.what() << std::endl;
        GlobalUnlock(hData);
        CloseClipboard();
        return 1;
    }
    GlobalUnlock(hData);
    CloseClipboard();

    return 0;
}

int getHighlightedText(wchar_t*& text) {
    wchar_t* temp = nullptr;
    if (getClipboardText(temp) == 1) {
        return 1;
    }
    simCopy();
    Sleep(10);
    if (getClipboardText(text) == 1) {
        return 1;
    }
    setClipboardText(temp);
    return 0;

}

int simCopy() {
    INPUT inputs[4] = {0};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 0x43;
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = 0x43;
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_CONTROL;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    if (SendInput(4, inputs, sizeof(INPUT)) == 0) {
        std::cerr << "Failed to simulate copy." << std::endl;
        return 1;
    }
    return 0;
}

int simPaste() {
    INPUT inputs[4] = {0};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 0x56;
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = 0x56;
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_CONTROL;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    if (SendInput(4, inputs, sizeof(INPUT)) == 0) {
        std::cerr << "Failed to simulate paste." << std::endl;
        return 1;
    }
    return 0;
}

int setClipboardText(wchar_t*& text) {
    // Open the clipboard
    if (!OpenClipboard(nullptr)) {
        std::cerr << "Failed to open the clipboard." << std::endl;
        return 1;
    }

    EmptyClipboard();

    size_t textLength = wcslen(text) + 1;
    HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE, textLength * sizeof(wchar_t));
    if (!hData) {
        std::cerr << "Failed to allocate memory for clipboard data." << std::endl;
        CloseClipboard();
        return 1;
    }

    wchar_t* buf = static_cast<wchar_t*>(GlobalLock(hData));
    if (!buf) {
        std::cerr << "Failed to lock memory for clipboard data." << std::endl;
        GlobalFree(hData);
        CloseClipboard();
        return 1;
    }

    memcpy(buf, text, textLength * sizeof(wchar_t));
    GlobalUnlock(hData);

    if (!SetClipboardData(CF_UNICODETEXT, hData)) {
        std::cerr << "Failed to set clipboard data." << std::endl;
        GlobalFree(hData);
        CloseClipboard();
        return 1;
    }
    CloseClipboard();
    return 0;
}

int manualPaste(wchar_t*& text) {
    wchar_t* temp = nullptr;
    if (getClipboardText(temp) == 1) {
        return 1;
    }
    if (setClipboardText(text) == 1) {
        return 1;
    }
    simPaste();
    Sleep(10);
    if (setClipboardText(temp) == 1) {
        return 1;
    }
    return 0;
}