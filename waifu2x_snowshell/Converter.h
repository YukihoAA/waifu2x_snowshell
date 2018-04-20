#pragma once

#include <Windows.h>
#include <string>
#include <sstream>
#include <queue>

#include "ConvertOption.h"

extern BOOL FileExists(LPCWSTR file);
extern BOOL IsDirectory(LPCWSTR path);

class Converter {
private:
	bool Available;
	bool IsCPU;
	bool Is64bitOnly;
	bool IsCudaOnly;
	bool TTA;
	std::wstring ExePath;
	std::wstring WorkingDir;
	HANDLE hConvertThread;
	HANDLE hConvertProcess;

protected:
	std::queue<ConvertOption> ConvertQueue;
	static DWORD WINAPI ConvertPorc(PVOID lParam);

public:
	Converter();
	Converter(std::wstring exePath, bool is64bitOnly = true, bool isCudaOnly = false, bool tta = false);
	bool checkAvailable();
	void setCPU(bool isCPU);
	void setAvailable(bool available);
	void setExePath(std::wstring exePath);
	void setWorkingDir(std::wstring workingDir);
	bool getCPU();
	bool getAvailable();
	std::wstring getExePath();
	std::wstring getWorkingDir();
	bool execute(ConvertOption *convertOption, bool noLabel = false);
	void addQueue(ConvertOption *convertOption);
	void emptyQueue();

};