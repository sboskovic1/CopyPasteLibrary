#ifndef CLIPBOARD_H
#define CLIPBOARD_H

int getClipboardText(wchar_t*& text);

int setClipboardText(wchar_t*& text);

int simCopy();

int simPaste();

int getHighlightedText(wchar_t*& text);

int manualPaste(wchar_t*& text);


#endif