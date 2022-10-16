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


#define MENU_FILE 0


#define MENU_NOISE 1
#define NOISE_NONE 0
#define NOISE_LOW 1
#define NOISE_MID 2
#define NOISE_HIGH 3
#define NOISE_VERY_HIGH 4

#define NOISE_MAX 4


#define MENU_SCALE 2
#define SCALE_x1_0 0
#define SCALE_x1_5 1
#define SCALE_x1_6 2
#define SCALE_x2_0 3
#define SCALE_CUSTOM 4

#define SCALE_MAX 4


#define MENU_GPU 3
#define GPU_CPU_MODE 0
#define GPU_GPU_MODE 1

#define GPU_MAX 1


#define MENU_TTA 4
#define TTA_DISABLED 0
#define TTA_ENABLED 1

#define TTA_MAX 1


#define MENU_EXPORT 5
#define EXPORT_SAME 0
#define EXPORT_NEW 1

#define EXPORT_MAX 1


#define MENU_CONFIRM 6
#define CONFIRM_SHOW 0
#define CONFIRM_SKIP 1

#define CONFIRM_MAX 1


#define MENU_LANG 7

#define MENU_CONVERTER 8
#define CONVERTER_NUM_CPP 0
#define CONVERTER_NUM_CAFFE 1
#define CONVERTER_NUM_VULKAN 2
#define CONVERTER_NUM_CUGAN 3
#define CONVERTER_NUM_ESRGAN 4

#define CONVERTER_NUM_MAX 4


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
	static bool IsCPU;

	int Noise;
	int Scale;
	int GPU;
	int TTA;
	int Export;
	int Confirm;
	int Lang;
	int Debug;
	int ConverterNum;
	std::wstring ScaleRatio;
	std::wstring OutputExt;

protected:
	SnowSetting();
	~SnowSetting();
	static void loadLocale();

public:
	static LPWSTR VulkanScale[];
	const static int VulkanScaleNum;
	const static int CuganScaleNum;
	static std::wstring OutputDirName;
	static std::wstring CurrPath;

	static Converter_Cpp CONVERTER_CPP;
	static Converter_Caffe CONVERTER_CAFFE;
	static Converter_Vulkan CONVERTER_VULKAN;
	static Converter_Cugan CONVERTER_CUGAN;
	static Converter_Esrgan CONVERTER_ESRGAN;
	static Converter* CurrentConverter;

	static SnowSetting *Init();
	static bool checkCuda();
	static bool checkVulkan();
	static bool getIsCPU();
	static int getCoreNum();
	static bool loadSetting();
	static bool saveSetting();
	static void loadMenuString(HMENU hMenu);

	static int getNoise();
	static int getScale();
	static int getGPU();
	static int getTTA();
	static int getExport();
	static BOOL getConfirm();
	static BOOL getDebug();
	static int getConverterNum();
	static int getLang();
	static std::wstring getLangName();
	static std::wstring getScaleRatio();
	static std::wstring getOutputExt();

	static void setNoise(int Noise);
	static void setScale(int Scale);
	static void setGPU(int GPU);
	static void setTTA(int tta);
	static void setExport(int Export);
	static void setConfirm(BOOL Confirm);
	static void setLang(int Lang);
	static void setDebug(BOOL Debug);
	static void setConverterNum(int ConverterNum);
	static void setScaleRatio(std::wstring scaleRatio);
	static void setOutputExt(std::wstring outputExt);

	static void checkMenuAll(HMENU hMenu);
	static void checkNoise(HMENU hMenu, int sel = -1);
	static void checkScale(HMENU hMenu, int sel = -1);
	static void checkGPU(HMENU hMenu, int sel = -1);
	static void checkTTA(HMENU hMenu, int sel = -1);
	static void checkExport(HMENU hMenu, int sel = -1);
	static void checkConfirm(HMENU hMenu, int sel = -1);
	static void checkLang(HMENU hMenu, int sel = -1);
	static void checkDebug(HMENU hMenu, int sel = -1);
	static void checkConverterNum(HMENU hMenu, int sel = -1);

	static void getTexts(std::wstring*(*UITitleText)[5], std::wstring*(*UIText)[5]);
	static std::wstring* getNoiseText();
	static std::wstring* getScaleText();
	static std::wstring* getGPUText();
	static std::wstring* getExportText();
	static std::wstring* getConfirmText();
};

std::wstring itos(int n);
std::wstring dtos(double n);
BOOL FileExists(LPCWSTR file);
BOOL IsDirectory(LPCWSTR path);
int contain(std::wstring str, std::wstring find);

void PrintDebugMessage(double db, std::wstring title= L"Debug");
void PrintDebugMessage(std::string str);
void PrintDebugMessage(std::wstring str);