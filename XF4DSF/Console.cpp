#include "Includes.h"
#include "Console.h"
#include <string>

Console console;

void Console::allocate() {
	if (!allocated) {
		AllocConsole();
		handle = GetStdHandle(STD_OUTPUT_HANDLE);
		allocated = true;
	}
}

Console::Console() : handle(0), allocated(false) {
}
Console::~Console() {
	if (allocated) {
		FreeConsole();
		allocated = false;
	}
}

void Console::print(const std::string &string) {
	DWORD dwCharsWritten;
	allocate();
	WriteConsoleA(handle, string.c_str(), (DWORD)(string.length()), &dwCharsWritten, NULL);
}

COORD Console::getCursorPosition() {
	CONSOLE_SCREEN_BUFFER_INFO bufferInfo;

	allocate();
	GetConsoleScreenBufferInfo(handle, &bufferInfo);
	return bufferInfo.dwCursorPosition;
}

void Console::setCursorPosition(COORD cord) {
	allocate();
	SetConsoleCursorPosition(handle, cord);
}


