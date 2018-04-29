#pragma once

#include <Windows.h>
#include <CommCtrl.h>
#include <string>
#include <sstream>
#include <queue>

#include "ConvertOption.h"
#include "resource.h"

#define WM_SET_CONVERTER WM_USER+1000

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
	std::wstring ModelDir;
	std::wstring CustomOption;
	HANDLE hConvertThread;
	HANDLE hConvertProcess;
	HWND hProgressDlg;

protected:
	std::queue<ConvertOption> ConvertQueue;
	static DWORD WINAPI ConvertPorc(PVOID lParam);
	static BOOL CALLBACK ProgressDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	Converter();
	Converter(std::wstring exePath, bool is64bitOnly = true, bool isCudaOnly = false, bool tta = false);
	~Converter();
	bool checkAvailable();
	void setCPU(bool isCPU);
	void setAvailable(bool available);
	void setExePath(std::wstring exePath);
	void setWorkingDir(std::wstring workingDir);
	void setModelDir(std::wstring modelDir);
	void setOptionString(std::wstring optionString);
	bool getCPU();
	bool getTTA();
	bool getAvailable();
	std::wstring getExePath();
	std::wstring getWorkingDir();
	std::wstring getModelDir();
	std::wstring getOptionString();
	bool execute(ConvertOption *convertOption, bool noLabel = false);
	void addQueue(ConvertOption *convertOption);
	void emptyQueue();

};