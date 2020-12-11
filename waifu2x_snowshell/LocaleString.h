#pragma once
#include <Windows.h>
#include <string>
#include <sstream>

using namespace std;

#define SETTING_VER_MINIMUM 25

extern int INT_SETTING_VER;

extern wstring STRING_MENU_FILE;
extern wstring STRING_MENU_NOISE;
extern wstring STRING_MENU_SCALE;
extern wstring STRING_MENU_EXPORT;
extern wstring STRING_MENU_CONFIRM;

extern wstring STRING_MENU_FILE_IMAGE_SEL;
extern wstring STRING_MENU_FILE_QUIT;
extern wstring STRING_MENU_FILE_CREDIT;

extern wstring STRING_MENU_NOISE_NONE;
extern wstring STRING_MENU_NOISE_LOW;
extern wstring STRING_MENU_NOISE_MID;
extern wstring STRING_MENU_NOISE_HIGH;
extern wstring STRING_MENU_NOISE_VERY_HIGH;

extern wstring STRING_MENU_SCALE_x1_0;
extern wstring STRING_MENU_SCALE_x1_5;
extern wstring STRING_MENU_SCALE_x1_6;
extern wstring STRING_MENU_SCALE_x2_0;
extern wstring STRING_MENU_SCALE_CUSTOM;

extern wstring STRING_MENU_GPU_CPU;
extern wstring STRING_MENU_GPU_GPU;

extern wstring STRING_MENU_TTA_DISABLED;
extern wstring STRING_MENU_TTA_ENABLED;

extern wstring STRING_MENU_EXPORT_SAME;
extern wstring STRING_MENU_EXPORT_NEW;

extern wstring STRING_MENU_CONFIRM_SHOW;
extern wstring STRING_MENU_CONFIRM_SKIP;
extern wstring STRING_MENU_CONFIRM_DEBUG;


extern int INT_TEXT_TAB;
extern wstring STRING_TEXT_NOISE;
extern wstring STRING_TEXT_NOISE_NONE;
extern wstring STRING_TEXT_NOISE_LOW;
extern wstring STRING_TEXT_NOISE_MID;
extern wstring STRING_TEXT_NOISE_HIGH;
extern wstring STRING_TEXT_NOISE_VERY_HIGH;

extern wstring STRING_TEXT_SCALE;
extern wstring STRING_TEXT_SCALE_x1_0;
extern wstring STRING_TEXT_SCALE_x1_5;
extern wstring STRING_TEXT_SCALE_x1_6;
extern wstring STRING_TEXT_SCALE_x2_0;
extern wstring STRING_TEXT_SCALE_PREFIX;
extern wstring STRING_TEXT_SCALE_POSTFIX;

extern wstring STRING_TEXT_GPU;
extern wstring STRING_TEXT_GPU_CPU;
extern wstring STRING_TEXT_GPU_CPU_TTA;
extern wstring STRING_TEXT_GPU_GPU;
extern wstring STRING_TEXT_GPU_GPU_TTA;
extern wstring STRING_TEXT_GPU_VULKAN;
extern wstring STRING_TEXT_GPU_VULKAN_TTA;

extern wstring STRING_TEXT_EXPORT;
extern wstring STRING_TEXT_EXPORT_SAME;
extern wstring STRING_TEXT_EXPORT_NEW;

extern wstring STRING_TEXT_CONFIRM;
extern wstring STRING_TEXT_CONFIRM_SHOW;
extern wstring STRING_TEXT_CONFIRM_SKIP;
extern wstring STRING_TEXT_CONFIRM_TITLE;
extern wstring STRING_TEXT_CONFIRM_MESSAGE;
extern wstring STRING_TEXT_CONFIRM_CUSTOM_SCALE_TITLE;
extern wstring STRING_TEXT_CONFIRM_CUSTOM_SCALE_MESSAGE;

extern wstring STRING_TEXT_NOCONVERTER_TITLE;
extern wstring STRING_TEXT_NOCONVERTER_MESSAGE;

extern wstring STRING_TEXT_TOO_LONG_PATH_TITLE;
extern wstring STRING_TEXT_TOO_LONG_PATH_MESSAGE;
extern wstring STRING_TEXT_TOO_LONG_PATH_MESSAGE_COUNT;

extern wstring STRING_TEXT_ABORT_CONVERT_TITLE;
extern wstring STRING_TEXT_ABORT_CONVERT_MESSAGE;