#include "Main.h"

HINSTANCE g_hInst;

BOOL is64bit = FALSE;
LPWSTR lpszClassCredit = L"CreditWindow";

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	HWND hWnd;
	MSG msg;
	LPWSTR lpszClass = L"SnowShell";

	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = lpszClass;
	RegisterClass(&wc);

	wc.lpszClassName = lpszClassCredit;
	wc.lpfnWndProc = CreditWndProc;
	RegisterClass(&wc);

	g_hInst = hInstance;

	// OpenCV 3.1 and later version does not supports x86 system.
	IsWow64Process(GetCurrentProcess(), &is64bit);

	// if(!is64bit)
	// MessageBox(NULL, L"SnowShell does not supports x86 system\n\nMore Information: https://github.com/YukihoAA/waifu2x_snowshell/releases", L"Warning", MB_OK | MB_ICONERROR);
	

	if (is64bit) {

		hWnd = CreateWindow(lpszClass, L"waifu2x - SnowShell v1.4", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_BORDER, CW_USEDEFAULT, CW_USEDEFAULT, 530, 370, NULL, NULL, hInstance, NULL);

		ShowWindow(hWnd, nCmdShow);

		while (GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return (int)msg.wParam;
	}
	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static HMENU hMenu;
	static HFONT hFont;
	static HBITMAP hBGBitmap;
	static wstring *UIText[5];
	static wstring *UITitleText[5];
	HBITMAP hBitmap;
	HFONT hOldFont;
	HBITMAP hOldBitmap;
	HBITMAP hOldBGBitmap;
	BITMAP bit;
	PAINTSTRUCT ps;
	HDC hdc, hBitDC, hMemDC;
	RECT rt;
	wstring LangName;
	ConvertOption convertOption;

	switch (uMsg) {
	case WM_CREATE:
		SnowSetting::Init();
		SnowSetting::getTexts(&UITitleText, &UIText);
		hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENU1));
		hFont = CreateFont(35, 0, 0, 0, FW_BOLD, 0, 0, 0, HANGEUL_CHARSET, OUT_OUTLINE_PRECIS, CLIP_STROKE_PRECIS, PROOF_QUALITY, VARIABLE_PITCH | FF_MODERN, L"Malgun Gothic");
		//hFont = CreateFont(35, 0, 0, 0, FW_BOLD, 0, 0, 0, HANGEUL_CHARSET, OUT_OUTLINE_PRECIS, CLIP_STROKE_PRECIS, PROOF_QUALITY, VARIABLE_PITCH | FF_MODERN, L"210 Sonyeoilgi R");
		SetMenu(hWnd, hMenu);
		SnowSetting::loadMenuString(hMenu);
		LangName = SnowSetting::getLangName();
		if (LangName.find(L"Korean") != wstring::npos)
			hBGBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP2));
		else if (LangName.find(L"Japanese") != wstring::npos)
			hBGBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP3));
		else
			hBGBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP1));
		DragAcceptFiles(hWnd, TRUE);
		return TRUE;
	case WM_INITMENU:
		SnowSetting::checkMenuAll(hMenu);
		return TRUE;
	case WM_DROPFILES: {
			DragAcceptFiles(hWnd, FALSE);

			vector<wstring> dragFiles;
			WCHAR* refFiles=NULL;
			UINT refFilesLen=0;
			HDROP hDrop = (HDROP)wParam;

			UINT fileNum = DragQueryFile(hDrop, 0xFFFFFFFF, refFiles, refFilesLen);

			if (fileNum > 0) {
				for (unsigned int i = 0; i < fileNum; i++) {
					int fileLen = DragQueryFile(hDrop, i, NULL, 0) + 1;

					WCHAR* newFileName = new WCHAR[fileLen];
					memset(newFileName, 0, sizeof(WCHAR)*fileLen);

					DragQueryFile(hDrop, i, newFileName, fileLen);

					if (FileExists(newFileName)) {
						wstring newFile = newFileName;
						dragFiles.push_back(newFile);
					}
					delete[] newFileName;
				}
			}

			for (unsigned i = 0; i < dragFiles.size(); i++) {
				Execute(hWnd, &convertOption, dragFiles.at(i).c_str());
			}

			DragAcceptFiles(hWnd, TRUE);
		}
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_MENU_FILE_IMGSEL: {
				OPENFILENAME ofn;
				WCHAR lpwstrFile[MAX_PATH]=L"";
				WCHAR lpstrFilter[MAX_PATH] = L"Supported Image Files\0*.jpg;*.jpeg;*.jpf;*.jpx;*.jp2;*.j2c;*.j2k;*.jpc;*.jps;*.png;*.tiff;*.bmp;*.pbm;*.pgm;*.ppm;*.pnm;*.pfm;*.pam\0All Files\0*.*";

				ZeroMemory(&ofn, sizeof(OPENFILENAME));
				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = hWnd;
				ofn.lpstrFile = lpwstrFile;
				ofn.nMaxFile = MAX_PATH;
				ofn.lpstrFilter = lpstrFilter;

				if (GetOpenFileName(&ofn) != NULL) {
					Execute(hWnd, &convertOption, ofn.lpstrFile);
				}
			}
			return TRUE;
		case ID_MENU_FILE_CREDIT: {
				HWND desktop = GetDesktopWindow();
				GetWindowRect(desktop, &rt);
				//ShowWindow(CreateWindow(lpszClassCredit, L"waifu2x - SnowShell v1.0", WS_POPUP | WS_BORDER, (rt.right - 200) / 2, (rt.bottom - 100) / 2, 300, 150, NULL, NULL, g_hInst, NULL), TRUE);
				if(SnowSetting::getLang()==0)
					MessageBox(hWnd, L"제작자: 유키 (yukiho5048@naver.com)\n\nTwitter: https://twitter.com/YukihoAA\nGitHub: https://github.com/YukihoAA\n", L"정보", MB_OK);
				else
					MessageBox(hWnd, L"Creator: Yuki (yukiho5048@naver.com)\n\nTwitter: https://twitter.com/YukihoAA\nGitHub: https://github.com/YukihoAA\n", L"Credit", MB_OK);
			}
			return TRUE;
		case ID_MENU_FILE_QUIT:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			return TRUE;
		case ID_MENU_NOISE_NONE:
		case ID_MENU_NOISE_LOW:
		case ID_MENU_NOISE_HIGH:
		case ID_MENU_NOISE_VERY_HIGH:
			if (LOWORD(wParam) >= ID_MENU_NOISE_VERY_HIGH) {
				SnowSetting::checkNoise(hMenu, NOISE_VERY_HIGH);
			}
			else
				SnowSetting::checkNoise(hMenu, LOWORD(wParam) - ID_MENU_NOISE_NONE);
			UIText[0]=SnowSetting::getNoiseText();
			InvalidateRect(hWnd, NULL, TRUE);
			return TRUE;
		case ID_MENU_SCALE_x1_0:
		case ID_MENU_SCALE_x1_5:
		case ID_MENU_SCALE_x1_6:
		case ID_MENU_SCALE_x2_0:
			SnowSetting::checkScale(hMenu, LOWORD(wParam) - ID_MENU_SCALE_x1_0);
			UIText[1] = SnowSetting::getScaleText();
			InvalidateRect(hWnd, NULL, TRUE);
			return TRUE;
		case ID_MENU_CPU_MID:
		case ID_MENU_CPU_HIGH:
		case ID_MENU_CPU_FULL:
			SnowSetting::checkCPU(hMenu, LOWORD(wParam) - ID_MENU_CPU_MID);
			UIText[2] = SnowSetting::getCPUText();
			InvalidateRect(hWnd, NULL, TRUE);
			return TRUE;
		case ID_MENU_EXPORT_SAME:
		case ID_MENU_EXPORT_NEW:
			SnowSetting::checkExport(hMenu, LOWORD(wParam) - ID_MENU_EXPORT_SAME);
			UIText[3] = SnowSetting::getExportText();
			InvalidateRect(hWnd, NULL, TRUE);
			return TRUE;
		case ID_MENU_CONFIRM_SHOW:
		case ID_MENU_CONFIRM_HIDE:
			SnowSetting::checkConfirm(hMenu, LOWORD(wParam) - ID_MENU_CONFIRM_SHOW);
			UIText[4] = SnowSetting::getConfirmText();
			InvalidateRect(hWnd, NULL, TRUE);
			return TRUE;
		case ID_MENU_LANG_KO:
		case ID_MENU_LANG_EN:
		case ID_MENU_LANG_JP:
		case ID_MENU_LANG_CN:
			SnowSetting::checkLang(hMenu, LOWORD(wParam) - ID_MENU_LANG_KO);
			SnowSetting::getTexts(&UITitleText, &UIText);
			SetMenu(hWnd, NULL);
			DestroyMenu(hMenu);
			hMenu=LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENU1));
			SnowSetting::loadMenuString(hMenu);
			SetMenu(hWnd, hMenu);
			DeleteObject(hBGBitmap);
			LangName = SnowSetting::getLangName();
			if (LangName.find(L"Korean") != wstring::npos)
				hBGBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP2));
			else if (LangName.find(L"Japanese") != wstring::npos)
				hBGBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP3));
			else
				hBGBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP1));
			InvalidateRect(hWnd, NULL, TRUE);
		}
		return FALSE;
	case WM_PAINT:
		hdc=BeginPaint(hWnd, &ps);
		GetClientRect(hWnd, &rt);

		hMemDC = CreateCompatibleDC(hdc);
		hBitmap= CreateCompatibleBitmap(hdc, rt.right, rt.bottom);
		hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);

		hOldFont = (HFONT)SelectObject(hMemDC, hFont);
		SetTextColor(hMemDC, RGB(0xef, 0xef, 0xef));
		SetBkMode(hMemDC, TRANSPARENT);

		hBitDC = CreateCompatibleDC(hMemDC);
		hOldBGBitmap = (HBITMAP)SelectObject(hBitDC, hBGBitmap);

		GetObject(hBGBitmap, sizeof(BITMAP), &bit);

		BitBlt(hMemDC, 0, 0, rt.right, rt.bottom, hBitDC, 0, 0, SRCCOPY);

		SelectObject(hBitDC, hOldBGBitmap);
		DeleteDC(hBitDC);

		for (int i = 0; i<5; i++)
			prtTextBorder(hMemDC, 7, 133 + 33 * i, UITitleText[i]->c_str(), (int)UITitleText[i]->length(), RGB(0x21, 0x21, 0x21), 2);

		for (int i = 0; i<5; i++)
			prtTextBorder(hMemDC, (int)(INT_TEXT_TAB * 12.5 + 7), 133 + 33 * i, UIText[i]->c_str(), (int) UIText[i]->length(), RGB(0x21, 0x21, 0x21), 2);

		BitBlt(hdc, 0, 0, rt.right, rt.bottom, hMemDC, 0, 0, SRCCOPY);

		SelectObject(hMemDC, hOldFont);
		DeleteDC(hMemDC);

		EndPaint(hWnd, &ps);
		return TRUE;
	case WM_DESTROY:
		SnowSetting::saveSetting();
		DestroyMenu(hMenu);
		DeleteObject(hBGBitmap);
		DeleteObject(hFont);
		PostQuitMessage(0);
		return TRUE;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CreditWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static HBITMAP hBitmap;
	HBITMAP hOldBitmap;
	BITMAP bit;
	PAINTSTRUCT ps;
	HDC hdc, MemDC;
	RECT rt;
	switch (uMsg) {
	case WM_CREATE:
		hBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP1));
		return TRUE;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		MemDC = CreateCompatibleDC(hdc);
		hOldBitmap = (HBITMAP)SelectObject(MemDC, hBitmap);

		GetObject(hBitmap, sizeof(BITMAP), &bit);

		GetClientRect(hWnd, &rt);
		BitBlt(hdc, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, MemDC, 0, 0, SRCCOPY);
		SelectObject(MemDC, hOldBitmap);
		DeleteDC(MemDC);
		EndPaint(hWnd, &ps);
		return TRUE;
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_KEYDOWN:
		SendMessage(hWnd, WM_CLOSE, 0, 0);
		return TRUE;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

BOOL Execute(HWND hWnd, ConvertOption *convertOption, LPCWSTR fileName) {

	if (!FileExists(fileName)) {
		return FALSE;
	}
	// if input is directory
	/*else if (FILE_ATTRIBUTE_DIRECTORY & GetFileAttributes(fileName)) {
		wstring fnStr = fileName;
		size_t last = fnStr.find_last_of(L'\\');
		//CreateDirectory(fnStr.substr(last).c_str(), NULL);

		//TODO: need to build folder name and Find all files to convert
	}*/

	if (SnowSetting::getConfirm() == CONFIRM_SHOW && MessageBox(hWnd, STRING_TEXT_CONFIRM_MESSAGE.c_str(), STRING_TEXT_CONFIRM_TITLE.c_str(), MB_YESNO | MB_ICONEXCLAMATION | MB_SYSTEMMODAL) == IDNO)
		return FALSE;

	convertOption->setInputFilePath(fileName);
	convertOption->setNoiseLevel(SnowSetting::getNoise());
	if(SnowSetting::getExport())
	convertOption->setOutputFolderName(SnowSetting::NewPath);
	switch (SnowSetting::getScale()) {
	case SCALE_x1_0:
		convertOption->setScaleRatio(L"1.0");
		break;
	case SCALE_x1_5:
		convertOption->setScaleRatio(L"1.5");
		break;
	case SCALE_x1_6:
		convertOption->setScaleRatio(L"1.6");
		break;
	case SCALE_x2_0:
		convertOption->setScaleRatio(L"2.0");
		break;
	}
	convertOption->setTTAEnabled(true);


	if (SnowSetting::CONVERTER_CAFFE.getAvailable() && SnowSetting::getCPU() == CPU_FULL && SnowSetting::getCudaAvailable())
		return SnowSetting::CONVERTER_CAFFE.Execute(hWnd, convertOption);
	else if (SnowSetting::CONVERTER_CPP_x64.getAvailable())
		return SnowSetting::CONVERTER_CPP_x64.Execute(hWnd, convertOption);
	else if (SnowSetting::CONVERTER_CPP_x86.getAvailable()) {
		if (SnowSetting::getNoise()>NOISE_HIGH)
			convertOption->setNoiseLevel(NOISE_HIGH);
		return SnowSetting::CONVERTER_CPP_x86.Execute(hWnd, convertOption);
	}
	else
		return FALSE;
	return TRUE;
}


void prtTextBorder(HDC hdc, int x, int y, LPCWSTR str, int c, COLORREF borderColor, int borderSize, COLORREF textColor) {
	COLORREF OldColor=textColor;
	OldColor = GetTextColor(hdc);
	SetTextColor(hdc, borderColor);

	for(int i=-borderSize; i<= borderSize;i++)
		for (int j = -borderSize; j <= borderSize; j++)
			TextOut(hdc, x+i, y+j, str, c);
	SetTextColor(hdc, OldColor);
	TextOut(hdc, x, y, str, c);
}