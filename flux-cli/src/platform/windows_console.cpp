#ifdef _WIN32

#include "windows_console.h"
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <iostream>

namespace FluxCLI::Platform {

void enableUTF8Console() {
    // Set console input/output code page to UTF-8
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    
    // Set C++ streams to UTF-8 mode
    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stderr), _O_U8TEXT);
    _setmode(_fileno(stdin), _O_U8TEXT);
    
    // Enable ANSI escape sequence support (Windows 10 version 1607 and above)
    enableANSIColors();
}

void enableANSIColors() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        return;
    }
    
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) {
        return;
    }
    
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}

int getConsoleWidth() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    
    if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
    
    return 80; // Default width
}

} // namespace FluxCLI::Platform

#endif // _WIN32

