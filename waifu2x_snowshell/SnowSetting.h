#pragma once

#include <Windows.h>
#include <stdio.h>
#include <string>
#include <thread>
#include <sstream>
#include <iostream>

#include "LocaleString.h"
#include "MenuItem.h"

using namespace std;


#define NOISE_NONE 0
#define NOISE_LOW 1
#define NOISE_HIGH 2

#define NOISE_MAX 3


#define SCALE_x1_0 0
#define SCALE_x1_5 1
#define SCALE_x1_6 2
#define SCALE_x2_0 3

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
	static wstring LangPath;
	static wstring LangName[];
	static wstring LangFile[];
	static wstring INIPath;
	static int CoreNum;
	const static int LangNum;

	int Noise;
	int Scale;
	int CPU;
	int Export;
	int Confirm;
	int Lang;

protected:
	SnowSetting();
	~SnowSetting();
	static void loadLocale();

public:
	static wstring NewPath;
	static wstring CurrPath;
	static wstring CONVERTER_x64_EXE;
	static wstring CONVERTER_CAFFE_EXE;

	static SnowSetting *Init();
	static bool loadSetting();
	static bool saveSetting();
	static void loadMenuString(HMENU hMenu);
	static wstring BuildParam(LPCWSTR inputFile);

	static int getNoise();
	static int getScale();
	static int getCPU();
	static int getExport();
	static BOOL getConfirm();
	static int getLang();
	static wstring getLangName();

	static void setNoise(int Noise);
	static void setScale(int Scale);
	static void setCPU(int CPU);
	static void setExport(int Export);
	static void setConfirm(BOOL Confirm);
	static void setLang(int Lang);

	static void checkMenuAll(HMENU hMenu);
	static void checkNoise(HMENU hMenu, int sel = -1);

	static void checkScale(HMENU hMenu, int sel = -1);
	static void checkCPU(HMENU hMenu, int sel = -1);
	static void checkExport(HMENU hMenu, int sel = -1);
	static void checkConfirm(HMENU hMenu, int sel = -1);
	static void checkLang(HMENU hMenu, int sel = -1);

	static void getTexts(wstring*(*UIText)[5]);
	static wstring* getNoiseText();
	static wstring* getScaleText();
	static wstring* getCPUText();
	static wstring* getExportText();
	static wstring* getConfirmText();
};

wstring itos(int n);
BOOL FileExists(LPCWSTR file);
int contain(wstring str, wstring find);