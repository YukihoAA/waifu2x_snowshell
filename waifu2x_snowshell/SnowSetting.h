#pragma once

#include <Windows.h>
#include <stdio.h>
#include <string>
#include <thread>
#include <sstream>
#include <iostream>

#include "LocaleString.h"
#include "Converter.h"

using namespace std;


#define NOISE_NONE 0
#define NOISE_LOW 1
#define NOISE_HIGH 2
#define NOISE_VERY_HIGH 3

#define NOISE_MAX 3


#define SCALE_x1_0 0
#define SCALE_x1_5 1
#define SCALE_x1_6 2
#define SCALE_x2_0 3
#define SCALE_CUSTOM 4

#define SCALE_MAX 3


#define CPU_MID 0
#define CPU_HIGH 1
#define CPU_FULL 2

#define CPU_MAX 2


#define EXPORT_SAME 0
#define EXPORT_NEW 1

#define EXPORT_MAX 1


#define CONFIRM_SHOW 0
#define CONFIRM_SKIP 1

#define CONFIRM_MAX 1

#undef MAX_PATH
#define MAX_PATH SHRT_MAX

class SnowSetting {
private:
	static SnowSetting *Singletone;
	static std::wstring LangPath;
	static std::wstring LangName[];
	static std::wstring LangFile[];
	static std::wstring INIPath;
	static int CoreNum;
	const static int LangNum;
	static bool IsCudaAvailable;
	static bool IsCPU;

	int Noise;
	int Scale;
	int CPU;
	int Export;
	int Confirm;
	int Lang;
	std::wstring ScaleRatio;

protected:
	SnowSetting();
	~SnowSetting();
	static void loadLocale();

public:
	static std::wstring ExportDirName;
	static std::wstring CurrPath;

	static Converter CONVERTER_CPP_x86;
	static Converter CONVERTER_CPP_x64;
	static Converter CONVERTER_CAFFE;
	static Converter* CurrentConverter;

	static SnowSetting *Init();
	static bool checkCuda();
	static bool getCudaAvailable();
	static bool getIsCPU();
	static int getCoreNum();
	static bool loadSetting();
	static bool saveSetting();
	static void loadMenuString(HMENU hMenu);

	static int getNoise();
	static int getScale();
	static int getCPU();
	static int getExport();
	static BOOL getConfirm();
	static int getLang();
	static std::wstring getLangName();
	static std::wstring getScaleRatio();

	static void setNoise(int Noise);
	static void setScale(int Scale);
	static void setCPU(int CPU);
	static void setExport(int Export);
	static void setConfirm(BOOL Confirm);
	static void setLang(int Lang);
	static void setScaleRatio(std::wstring scaleRatio);

	static void checkMenuAll(HMENU hMenu);
	static void checkNoise(HMENU hMenu, int sel = -1);

	static void checkScale(HMENU hMenu, int sel = -1);
	static void checkCPU(HMENU hMenu, int sel = -1);
	static void checkExport(HMENU hMenu, int sel = -1);
	static void checkConfirm(HMENU hMenu, int sel = -1);
	static void checkLang(HMENU hMenu, int sel = -1);

	static void getTexts(std::wstring*(*UITitleText)[5], std::wstring*(*UIText)[5]);
	static std::wstring* getNoiseText();
	static std::wstring* getScaleText();
	static std::wstring* getCPUText();
	static std::wstring* getGPUText();
	static std::wstring* getExportText();
	static std::wstring* getConfirmText();
};

std::wstring itos(int n);
BOOL FileExists(LPCWSTR file);
BOOL IsDirectory(LPCWSTR path);
int contain(std::wstring str, std::wstring find);

void PrintDebugMessage(double db, std::wstring title= L"Debug");
void PrintDebugMessage(std::string str);
void PrintDebugMessage(std::wstring str);