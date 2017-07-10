#pragma once
#include <Windows.h>
#include <string.h>
#include <vector>

#include "resource.h"
#include "MenuItem.h"
#include "SnowSetting.h"

using namespace std;

#undef MAX_PATH
#define MAX_PATH SHRT_MAX

extern HINSTANCE g_hInst;
extern BOOL is64bit;

BOOL Execute(HWND hWnd, LPCWSTR fileName);
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK CreditWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void prtTextBorder(HDC hdc, int x, int y, LPCWSTR str, int c, COLORREF borderColor, int borderSize, COLORREF textColor = NULL);
bool GetImageSize(LPCWSTR fn, int *x, int *y);