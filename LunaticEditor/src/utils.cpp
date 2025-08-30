// if we're on windows:
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")

void setDarkTitlebar() {
	// Set immersive dark mode for title bar (Windows 10 1809+)
	HWND hwnd = GetActiveWindow();
	if (hwnd) {
		BOOL useDarkMode = TRUE;
		DwmSetWindowAttribute(hwnd, 20, &useDarkMode, sizeof(useDarkMode)); // 20 is DWMWA_USE_IMMERSIVE_DARK_MODE
	}
}
#else
void setDarkTitlebar() {
	// No-op on non-Windows platforms
}
#endif