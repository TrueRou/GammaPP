#include <iostream>
#include <windows.h>

float currentGammaValue = 0;
int currentDisplay = 0;
int availableDisplays = 1;

static int HasAttachedScreen(int monitorIndex) {
	DISPLAY_DEVICE display{};
	display.cb = sizeof(DISPLAY_DEVICE);
	int ok = EnumDisplayDevicesW(NULL, monitorIndex, &display, 0);
	return ok != 0 && display.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;
}

static BOOL SetGammaRamp(int monitorIndex, WORD gamma[]) {
	HDC hdc = GetDC(0);
	if (HasAttachedScreen(monitorIndex)) {
		DISPLAY_DEVICE display{};
		display.cb = sizeof(DISPLAY_DEVICE);
		EnumDisplayDevicesW(NULL, monitorIndex, &display, 0);
		hdc = CreateDC(NULL, display.DeviceName, NULL, 0);
		if (hdc == NULL) return false;
		WORD ramp[256 * 3]{};
		for (int i = 0; i < 256; i++) {
			ramp[i] = gamma[i];
			ramp[i + 256] = gamma[i];
			ramp[i + 512] = gamma[i];
		}
		BOOL result = SetDeviceGammaRamp(hdc, ramp);
		ReleaseDC(0, hdc);
		return result;
	}
	return false;
}

static BOOL GetGammaRamp(int monitorIndex, WORD* ramp) {
	HDC hdc = GetDC(0);
	if (HasAttachedScreen(monitorIndex)) {
		DISPLAY_DEVICE display{};
		display.cb = sizeof(DISPLAY_DEVICE);
		EnumDisplayDevicesW(NULL, monitorIndex, &display, 0);
		hdc = CreateDC(NULL, display.DeviceName, NULL, 0);
		if (hdc == NULL) return false;
		BOOL result = GetDeviceGammaRamp(hdc, ramp);
		ReleaseDC(0, hdc);
		return result;
	}
	return false;
}

static BOOL SetGamma(int monitorIndex, float offset) {
	WORD* words = new(WORD[256 * 3]);
	GetGammaRamp(monitorIndex, words);
	for (int i = 0; i < 256 * 3; i++) {
		int value = powf(i / 256.0, powf(4, offset)) * 65535 + 0.5;
		words[i] = min(65535, max(0, value));
	}
	return SetGammaRamp(monitorIndex, words);
}

static void RestoreGamma() {
	int cursor = 0;
	while (HasAttachedScreen(cursor++)) {
		availableDisplays = cursor;
		SetGamma(cursor - 1, 0);
	}
}

static BOOL WINAPI ConsoleHandler(DWORD event){
	if (event == CTRL_C_EVENT || event == CTRL_BREAK_EVENT || event == CTRL_CLOSE_EVENT)
		RestoreGamma();
	return TRUE;
}

static void RefreshScreen() {
	system("cls");
	printf("Gamma++ (ver 1.0.0)\n");
	printf("Made by TuRou, inspired by wasupandceacar\n");
	printf("=======================================================\n");
	printf("Guide: \n");
	printf("Press Ctrl + [ to decrease gamma (min value: -1)\n");
	printf("Press Ctrl + ] to increase gamma (max value: +1)\n");
	printf("Press Ctrl + \\ to reset gamma\n");
	printf("Press Alt + \\ to switch between displays\n");
	printf("=======================================================\n");
	printf("Current Display: %d\n", currentDisplay);
	printf("Current Gamma: %.2f\n", currentGammaValue);
}

static void RegisterEvents() {
	RegisterHotKey(NULL, 1, MOD_CONTROL, 219); // Control + [
	RegisterHotKey(NULL, 2, MOD_CONTROL, 221); // Control + ]
	RegisterHotKey(NULL, 3, MOD_CONTROL, 220); // Control + Slash
	RegisterHotKey(NULL, 4, MOD_ALT, 220); // Control + Slash
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE);
}

static void BeginEventLoop() {
	MSG msg = { 0 };
	while (GetMessage(&msg, NULL, 0, 0)) {
		if (msg.message == WM_HOTKEY) {
			if (msg.wParam == 1) {
				currentGammaValue = max(-1, currentGammaValue - 0.05);
			}
			if (msg.wParam == 2) {
				currentGammaValue = min(1, currentGammaValue + 0.05);
			}
			if (msg.wParam == 3) {
				currentGammaValue = 0;
			}
			if (msg.wParam == 4) {
				RestoreGamma();
				currentDisplay++;
				if (currentDisplay >= availableDisplays) currentDisplay = 0;
			}
			SetGamma(currentDisplay, -currentGammaValue);
			RefreshScreen();
		}
	}
}

int main()
{
	RefreshScreen();
	RestoreGamma();
	RegisterEvents();
	BeginEventLoop();
}