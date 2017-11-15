#ifndef __CONSOLE_H__
#define __CONSOLE_H__
#include <string>

class Console {
private:
	HANDLE handle;
	bool allocated;
private:
	void allocate();
public:
	Console();
	~Console();

	void print(const std::string &string);
	void print(const std::wstring &string);
	COORD getCursorPosition();
	void setCursorPosition(COORD cord);
};

extern Console console;

#endif