#pragma once

#include <Windows.h>
#include <CommCtrl.h>
#include <string>
#include <sstream>
#include <queue>

#include "LocaleString.h"
#include "ConvertOption.h"
#include "resource.h"

#define WM_SET_CONVERTER WM_USER+1000

extern BOOL FileExists(LPCWSTR file);
extern BOOL IsDirectory(LPCWSTR path);

class Converter {
protected:
	bool Available;
	std::wstring ExePath;
	std::wstring WorkingDir;
	std::wstring ModelDir;
	std::wstring CustomOption;
	HANDLE hConvertThread;
	HANDLE hConvertProcess;
	HWND hProgressDlg;

	std::queue<ConvertOption> ConvertQueue;
	static DWORD WINAPI ConvertPorc(PVOID lParam);
	static INT_PTR CALLBACK ProgressDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	Converter();
	Converter(std::wstring exePath);
	~Converter();
	bool checkAvailable();
	void setAvailable(bool available);
	void setExePath(std::wstring exePath);
	void setWorkingDir(std::wstring workingDir);
	void setModelDir(std::wstring modelDir);
	void setOptionString(std::wstring optionString);
	bool getAvailable();
	std::wstring getExePath();
	std::wstring getWorkingDir();
	std::wstring getModelDir();
	std::wstring getOptionString();
	void addQueue(ConvertOption *convertOption);
	void emptyQueue();
	bool convert(std::wstring param, std::wstring exportName, int debug);

	virtual bool execute(ConvertOption *convertOption, bool noLabel = false)=0;
};


class Converter_Cpp : public Converter{
public:
	Converter_Cpp() : Converter() {};
	Converter_Cpp(std::wstring exePath) : Converter(exePath) {};
	virtual bool execute(ConvertOption *convertOption, bool noLabel = false) override;
};

class Converter_Caffe : public Converter {
public:
	Converter_Caffe() : Converter() {};
	Converter_Caffe(std::wstring exePath) : Converter(exePath) {};
	virtual bool execute(ConvertOption *convertOption, bool noLabel = false) override;
};

class Converter_Vulkan : public Converter {
public:
	Converter_Vulkan() : Converter() {};
	Converter_Vulkan(std::wstring exePath) : Converter(exePath) {};
	virtual bool execute(ConvertOption* convertOption, bool noLabel = false) override;
};


class Converter_Cugan : public Converter {
public:
	Converter_Cugan() : Converter() {};
	Converter_Cugan(std::wstring exePath) : Converter(exePath) {};
	virtual bool execute(ConvertOption* convertOption, bool noLabel = false) override;
};


class Converter_Esrgan : public Converter {
public:
	Converter_Esrgan() : Converter() {};
	Converter_Esrgan(std::wstring exePath) : Converter(exePath) {};
	virtual bool execute(ConvertOption* convertOption, bool noLabel = false) override;
};