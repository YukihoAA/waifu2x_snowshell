#include "Main.h"

HINSTANCE g_hInst;
HWND hWnd;

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow) {
	int argc = 0;
	LPWSTR *argv = CommandLineToArgvW(lpCmdLine, &argc);
	MSG msg;
	BOOL bIsWow64 = FALSE;
	LPWSTR lpszClass = L"Snowshell";
	ConvertOption convertOption;

	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = lpszClass;
	RegisterClass(&wc);


	g_hInst = hInstance;

	// OpenCV 3.1 and later version does not supports x86 system.
	IsWow64Process(GetCurrentProcess(), &bIsWow64);

	if (!bIsWow64)
		MessageBox(NULL, L"This program only works on 64bit system", L"Error", MB_OK | MB_ICONERROR);

	hWnd = CreateWindow(lpszClass, L"Snowshell v2.6.1 - Waifu2x Image Upscaler", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_BORDER, CW_USEDEFAULT, CW_USEDEFAULT, 530, 370, NULL, NULL, hInstance, NULL);

	ShowWindow(hWnd, nCmdShow);

	if (wcscmp(lpCmdLine, L"")) {
		for (int i = 0; i < argc; i++) {
			Execute(hWnd, &convertOption, argv[i]);
		}
	}

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static HMENU hMenu;
	static HWND hCombo;
	static HFONT hFont, hSFont;
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
	int i;
	wstring LangName;
	ConvertOption convertOption;
	const int ExtNum = 3;
	const static LPWSTR ExtList[ExtNum] = {L"png", L"jpg", L"webp"};
	const static WCHAR lpstrFilter[MAX_PATH] = L"Supported Image Files\0*.bmp;*.dib;*.jpg;*.jpeg;*.jpe;*.jpf;*.jpx;*.jp2;*.j2c;*.j2k;*.jpc;*.jps;*.png;*.webp;*.pbm;*.pgm;*.ppm;*.pnm;*.pfm;*.pxm;*.pam;*.sr;*.ras;*.tif;*.tiff;*.exr;*.hdr;*.pic\0All Files\0*.*";

	switch (uMsg) {
	case WM_CREATE:
		SnowSetting::Init();

		// Check Converter
		if (SnowSetting::CurrentConverter == nullptr) {
			MessageBox(hWnd, STRING_TEXT_NOCONVERTER_MESSAGE.c_str(), STRING_TEXT_NOCONVERTER_TITLE.c_str(), MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
			SendMessage(hWnd, WM_DESTROY, NULL, NULL);
			return TRUE;
		}

		SnowSetting::getTexts(&UITitleText, &UIText);
		hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENU1));
		hFont = CreateFont(35, 0, 0, 0, FW_BOLD, 0, 0, 0, HANGEUL_CHARSET, OUT_OUTLINE_PRECIS, CLIP_STROKE_PRECIS, PROOF_QUALITY, VARIABLE_PITCH | FF_MODERN, L"Malgun Gothic");
		hSFont = CreateFont(15, 0, 0, 0, FW_BOLD, 0, 0, 0, HANGEUL_CHARSET, OUT_OUTLINE_PRECIS, CLIP_STROKE_PRECIS, PROOF_QUALITY, VARIABLE_PITCH | FF_MODERN, L"Malgun Gothic");
		SetMenu(hWnd, hMenu);
		SnowSetting::loadMenuString(hMenu);
		LangName = SnowSetting::getLangName();
		if (LangName.find(L"Korean") != wstring::npos)
			hBGBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP2));
		else if (LangName.find(L"Japanese") != wstring::npos)
			hBGBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP3));
		else if (LangName.find(L"Chinese") != wstring::npos)
			hBGBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP4));
		else
			hBGBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP1));

		hCombo = CreateWindowW(L"combobox", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 440, 283, 67, 0, hWnd, (HMENU)ID_EXT_COMBO, g_hInst, NULL);
		for (i = 0; i < ExtNum; i++) {
			SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)ExtList[i]);
			if(SnowSetting::getOutputExt()._Equal(ExtList[i]))
				SendMessage(hCombo, CB_SETCURSEL, i, 0);
		}
		SendMessage(hCombo, WM_SETFONT, (WPARAM) hSFont, 0);
		DragAcceptFiles(hWnd, TRUE);


		return TRUE;
	case WM_INITMENU:
		SnowSetting::checkMenuAll(hMenu);
		return TRUE;
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_ESCAPE:
			if (SnowSetting::CurrentConverter != nullptr && MessageBox(hWnd, STRING_TEXT_ABORT_CONVERT_MESSAGE.c_str(), STRING_TEXT_ABORT_CONVERT_TITLE.c_str(), MB_YESNO | MB_ICONEXCLAMATION | MB_SYSTEMMODAL) == IDYES)
				SnowSetting::CurrentConverter->emptyQueue();
			return TRUE;
		}
		return FALSE;
	case WM_DROPFILES: {
		DragAcceptFiles(hWnd, FALSE);

		vector<wstring> dragFiles;
		WCHAR* refFiles = NULL;
		UINT refFilesLen = 0;
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
			LPWSTR lpwstrFile = new WCHAR[MAX_PATH];
			
			lpwstrFile[0] = '\0';
			ZeroMemory(&ofn, sizeof(OPENFILENAME));
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = hWnd;
			ofn.lpstrFile = lpwstrFile;
			ofn.nMaxFile = MAX_PATH - 1;
			ofn.lpstrFilter = lpstrFilter;

			if (GetOpenFileName(&ofn) != NULL) {
				Execute(hWnd, &convertOption, ofn.lpstrFile);
			}

			if (lpwstrFile != nullptr)
			{
				delete[] lpwstrFile;
			}
			return TRUE;
		}
		case ID_MENU_FILE_CREDIT: {
			HWND desktop = GetDesktopWindow();
			GetWindowRect(desktop, &rt);
			if (SnowSetting::getLang() == 0)
				MessageBox(hWnd, L"제작자: 유키\n\nEmail: (yukihoaa@gmail.com)\nGitHub: https://github.com/YukihoAA\n", L"정보", MB_OK);
			else
				MessageBox(hWnd, L"Creator: Yuki\n\nEmail: (yukihoaa@gmail.com)\nGitHub: https://github.com/YukihoAA\n", L"Credit", MB_OK);
		}
								  return TRUE;
		case ID_MENU_FILE_QUIT:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			return TRUE;
		case ID_MENU_NOISE_NONE:
		case ID_MENU_NOISE_LOW:
		case ID_MENU_NOISE_MID:
		case ID_MENU_NOISE_HIGH:
		case ID_MENU_NOISE_VERY_HIGH:
			if (LOWORD(wParam) >= ID_MENU_NOISE_VERY_HIGH) {
				SnowSetting::checkNoise(hMenu, NOISE_VERY_HIGH);
			}
			else
				SnowSetting::checkNoise(hMenu, LOWORD(wParam) - ID_MENU_NOISE_NONE);
			UIText[0] = SnowSetting::getNoiseText();
			InvalidateRect(hWnd, NULL, TRUE);
			return TRUE;
		case ID_MENU_SCALE_CUSTOM:
			if (SnowSetting::getConfirm() == CONFIRM_SHOW && MessageBox(hWnd, STRING_TEXT_CONFIRM_CUSTOM_SCALE_MESSAGE.c_str(), STRING_TEXT_CONFIRM_CUSTOM_SCALE_TITLE.c_str(), MB_YESNO | MB_ICONEXCLAMATION | MB_SYSTEMMODAL) == IDNO)
				return TRUE;
			if (SnowSetting::CurrentConverter == &SnowSetting::CONVERTER_VULKAN)
				DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG3), hWnd, SettingDlgProcVulkan);
			else if (SnowSetting::CurrentConverter == &SnowSetting::CONVERTER_CUGAN)
				DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG3), hWnd, SettingDlgProcCugan);
			else
				DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, SettingDlgProc);
		case ID_MENU_SCALE_x1_0:
		case ID_MENU_SCALE_x1_5:
		case ID_MENU_SCALE_x1_6:
		case ID_MENU_SCALE_x2_0:
			SnowSetting::checkScale(hMenu, LOWORD(wParam) - ID_MENU_SCALE_x1_0);
			UIText[1] = SnowSetting::getScaleText();

			if (SnowSetting::CurrentConverter == &SnowSetting::CONVERTER_CUGAN) {
				SnowSetting::checkNoise(hMenu, SnowSetting::getNoise());
				UIText[0] = SnowSetting::getNoiseText();
			}
			InvalidateRect(hWnd, NULL, TRUE);
			return TRUE;
		case ID_MENU_GPU_CPU:
		case ID_MENU_GPU_GPU:
			SnowSetting::checkGPU(hMenu, LOWORD(wParam) - ID_MENU_GPU_CPU);
			UIText[2] = SnowSetting::getGPUText();
			InvalidateRect(hWnd, NULL, TRUE);
			return TRUE;
		case ID_MENU_TTA_DISABLED:
		case ID_MENU_TTA_ENABLED:
			SnowSetting::checkTTA(hMenu, LOWORD(wParam) - ID_MENU_TTA_DISABLED);
			UIText[2] = SnowSetting::getGPUText();
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
		case ID_MENU_CONFIRM_DEBUG:
			SnowSetting::checkDebug(hMenu, 0);
			return TRUE;
		case ID_MENU_LANG_KO:
		case ID_MENU_LANG_EN:
		case ID_MENU_LANG_JP:
		case ID_MENU_LANG_CN:
		case ID_MENU_LANG_DE:
		case ID_MENU_LANG_SV:
		case ID_MENU_LANG_PT:
		case ID_MENU_LANG_UKR:
			SnowSetting::checkLang(hMenu, LOWORD(wParam) - ID_MENU_LANG_KO);
			SnowSetting::getTexts(&UITitleText, &UIText);
			SetMenu(hWnd, NULL);
			DestroyMenu(hMenu);
			hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENU1));
			SnowSetting::loadMenuString(hMenu);
			SetMenu(hWnd, hMenu);
			DeleteObject(hBGBitmap);
			LangName = SnowSetting::getLangName();
			if (LangName.find(L"Korean") != wstring::npos)
				hBGBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP2));
			else if (LangName.find(L"Japanese") != wstring::npos)
				hBGBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP3));
			else if (LangName.find(L"Chinese") != wstring::npos)
				hBGBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP4));
			else if (LangName.find(L"Portuguese") != wstring::npos)
				hBGBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP5));
			else
				hBGBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP1));
			InvalidateRect(hWnd, NULL, TRUE);
			return TRUE;
		case ID_MENU_CONVERTER_CPP:
		case ID_MENU_CONVERTER_CAFFE:
		case ID_MENU_CONVERTER_VULKAN:
		case ID_MENU_CONVERTER_CUGAN:
		case ID_MENU_CONVERTER_ESRGAN:
			SnowSetting::checkConverterNum(hMenu, LOWORD(wParam) - ID_MENU_CONVERTER_CPP);
			SetMenu(hWnd, NULL);
			DestroyMenu(hMenu);
			hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENU1));
			SnowSetting::loadMenuString(hMenu);
			SetMenu(hWnd, hMenu);
			SnowSetting::checkMenuAll(hMenu);
			SnowSetting::getTexts(&UITitleText, &UIText);
			InvalidateRect(hWnd, NULL, TRUE);
			return TRUE;
		case ID_EXT_COMBO:
			switch (HIWORD(wParam)) {
			case CBN_SELCHANGE:
				i = SendMessage(hCombo, CB_GETCURSEL, 0, 0);
				SnowSetting::setOutputExt(ExtList[i]);
				break;
			}
		}
		return FALSE;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		GetClientRect(hWnd, &rt);

		hMemDC = CreateCompatibleDC(hdc);
		hBitmap = CreateCompatibleBitmap(hdc, rt.right, rt.bottom);
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

		for (int i = 0; i < 5; i++)
			prtTextBorder(hMemDC, 7, 133 + 33 * i, UITitleText[i]->c_str(), (int)UITitleText[i]->length(), RGB(0x21, 0x21, 0x21), 2);

		for (int i = 0; i < 5; i++)
			prtTextBorder(hMemDC, (int)(INT_TEXT_TAB * 12.5 + 7), 133 + 33 * i, UIText[i]->c_str(), (int)UIText[i]->length(), RGB(0x21, 0x21, 0x21), 2);

		SelectObject(hMemDC, hOldFont);
		hOldFont = (HFONT)SelectObject(hMemDC, hSFont);
		SetTextColor(hMemDC, RGB(0xef, 0xef, 0xef));

		if(SnowSetting::CurrentConverter == &SnowSetting::CONVERTER_CPP)
			prtTextBorder(hMemDC, 375, 2, L"waifu2x-converter-cpp", 21, RGB(0x21, 0x21, 0x21), 1);
		else if (SnowSetting::CurrentConverter == &SnowSetting::CONVERTER_CAFFE)
			prtTextBorder(hMemDC, 430, 2, L"waifu2x-caffe", 13, RGB(0x21, 0x21, 0x21), 1);
		else if (SnowSetting::CurrentConverter == &SnowSetting::CONVERTER_VULKAN)
			prtTextBorder(hMemDC, 385, 2, L"waifu2x-ncnn-vulkan", 19, RGB(0x21, 0x21, 0x21), 1);
		else if (SnowSetting::CurrentConverter == &SnowSetting::CONVERTER_CUGAN)
			prtTextBorder(hMemDC, 405, 2, L"realcugan-vulkan", 16, RGB(0x21, 0x21, 0x21), 1);
		else
		prtTextBorder(hMemDC, 400, 2, L"realesrgan-vulkan", 17, RGB(0x21, 0x21, 0x21), 1);

		prtTextBorder(hMemDC, 410, 288, L" =▶", 3, RGB(0x21, 0x21, 0x21), 1);

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

INT_PTR CALLBACK SettingDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static HWND hEdit, hText, hButton;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		hEdit = GetDlgItem(hDlg, IDC_EDIT1);
		hText = GetDlgItem(hDlg, IDC_TEXT1);
		hButton = GetDlgItem(hDlg, IDOK);
		SetWindowText(hEdit, SnowSetting::getScaleRatio().c_str());
		SetWindowText(hText, STRING_TEXT_SCALE.c_str());
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			WCHAR scaleText[6] = L"";
			GetWindowText(hEdit, scaleText, 6);
			SnowSetting::setScaleRatio(scaleText);

			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		return FALSE;
	case WM_CLOSE:
		EndDialog(hDlg, IDCANCEL);
		return TRUE;
	}
	return FALSE;
}

INT_PTR CALLBACK SettingDlgProcVulkan(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static HWND hCombo, hText, hButton;
	int sel;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		hCombo = GetDlgItem(hDlg, IDC_COMBO1);
		hText = GetDlgItem(hDlg, IDC_TEXT1);
		hButton = GetDlgItem(hDlg, IDOK);

		for (int i = 0; i < SnowSetting::VulkanScaleNum; i++) {
			SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)SnowSetting::VulkanScale[i]);
			if (!wcscmp(SnowSetting::getScaleRatio().c_str(), SnowSetting::VulkanScale[i]))
				SendMessage(hCombo, CB_SETCURSEL, i, 0);
		}

		SetWindowText(hText, STRING_TEXT_SCALE.c_str());
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			sel = SendMessage(hCombo, CB_GETCURSEL, 0, 0);

			if (sel != CB_ERR)
				SnowSetting::setScaleRatio(SnowSetting::VulkanScale[sel]);

			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		return FALSE;
	case WM_CLOSE:
		EndDialog(hDlg, IDCANCEL);
		return TRUE;
	}
	return FALSE;
}

INT_PTR CALLBACK SettingDlgProcCugan(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static HWND hCombo, hText, hButton;
	int sel;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		hCombo = GetDlgItem(hDlg, IDC_COMBO1);
		hText = GetDlgItem(hDlg, IDC_TEXT1);
		hButton = GetDlgItem(hDlg, IDOK);

		for (int i = 0; i < SnowSetting::CuganScaleNum; i++) {
			SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)SnowSetting::VulkanScale[i]);
			if (!wcscmp(SnowSetting::getScaleRatio().c_str(), SnowSetting::VulkanScale[i]))
				SendMessage(hCombo, CB_SETCURSEL, i, 0);
		}

		SetWindowText(hText, STRING_TEXT_SCALE.c_str());
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			sel = SendMessage(hCombo, CB_GETCURSEL, 0, 0);

			if(sel != CB_ERR)
				SnowSetting::setScaleRatio(SnowSetting::VulkanScale[sel]);

			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		return FALSE;
	case WM_CLOSE:
		EndDialog(hDlg, IDCANCEL);
		return TRUE;
	}
	return FALSE;
}

BOOL Execute(HWND hWnd, ConvertOption *convertOption, LPCWSTR fileName, bool noLabel) {
	int FileNameLength;
	int MaxInputLength;
	if (!FileExists(fileName)) {
		return FALSE;
	}

	// Limit Input File Name Length
	FileNameLength = (int) wcslen(fileName);
	MaxInputLength = SnowSetting::getExport() ? 255 - (int) SnowSetting::OutputDirName.length() : 220;
	if (FileNameLength >= MaxInputLength) {
		wstring Message = STRING_TEXT_TOO_LONG_PATH_MESSAGE + L" (" + itos(FileNameLength) + STRING_TEXT_TOO_LONG_PATH_MESSAGE_COUNT + L"/" + itos(MaxInputLength) + STRING_TEXT_TOO_LONG_PATH_MESSAGE_COUNT + L")\n" + fileName;
		MessageBoxW(hWnd, Message.c_str(), STRING_TEXT_TOO_LONG_PATH_TITLE.c_str(), MB_ICONERROR | MB_SYSTEMMODAL | MB_OK);
		return FALSE;
	}

	if (SnowSetting::getConfirm() == CONFIRM_SHOW && convertOption->getOutputFolderName() == L"" && MessageBox(hWnd, STRING_TEXT_CONFIRM_MESSAGE.c_str(), STRING_TEXT_CONFIRM_TITLE.c_str(), MB_YESNO | MB_ICONEXCLAMATION | MB_SYSTEMMODAL) == IDNO)
		return FALSE;

	convertOption->setDebugMode(SnowSetting::getDebug());

	convertOption->setInputFilePath(fileName);
	convertOption->setNoiseLevel(SnowSetting::getNoise() - 1);
	if (SnowSetting::getExport() && convertOption->getOutputFolderName() == L"") {
		wstring inputPath = fileName;
		convertOption->setOutputFolderName(inputPath.substr(0, inputPath.find_last_of(L"\\")) + L"\\" + SnowSetting::OutputDirName);
	}
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
	case SCALE_CUSTOM:
		convertOption->setScaleRatio(SnowSetting::getScaleRatio());
		break;
	}

	convertOption->setTTAEnabled(SnowSetting::getTTA());
	convertOption->setForceCPU(SnowSetting::getGPU() == GPU_CPU_MODE || SnowSetting::CurrentConverter == &SnowSetting::CONVERTER_CAFFE && !SnowSetting::checkCuda());
	convertOption->setOutputFileExtension(SnowSetting::getOutputExt());

	DWORD FileAttribute = GetFileAttributes(fileName);

	// if input is directory
	if (FILE_ATTRIBUTE_DIRECTORY & FileAttribute) {
		std::wstringstream FolderNameStream;
		FolderNameStream << fileName << L"_waifu2x";

		if (!noLabel) {
			// set noise_level
			if (convertOption->getNoiseLevel() != ConvertOption::CO_NOISE_NONE) {
				FolderNameStream << L"_noise" << convertOption->getNoiseLevel();
			}

			// set scale_ratio
			if (convertOption->getScaleRatio() != L"1.0") {

				std::wstring ScaleRatio = convertOption->getScaleRatio();
				size_t last = ScaleRatio.find_last_of(L'.');
				if (last != std::wstring::npos)
					ScaleRatio[last] = L'_';

				FolderNameStream << L"_scale_x" << ScaleRatio;
			}

			// set tta
			if (convertOption->getTTAEnabled())
				FolderNameStream << L"_tta_1";
		}
		convertOption->setOutputFolderName(FolderNameStream.str());
		CreateDirectory(convertOption->getOutputFolderName().c_str(), NULL);

		//TODO: Find all files to convert
		queue<ConvertOption> FolderSearchQueue;

		FolderSearchQueue.push(*convertOption);

		while (!FolderSearchQueue.empty()) {
			WIN32_FIND_DATA FileFindData;
			int FoundNameLength;
			HANDLE hFind = INVALID_HANDLE_VALUE;
			ConvertOption FolderConvertOption = FolderSearchQueue.front();
			wstring filePath = FolderConvertOption.getInputFilePath();

			hFind = FindFirstFileW((filePath + L"\\*.*").c_str(), &FileFindData);

			if (INVALID_HANDLE_VALUE == hFind) {
				FindClose(hFind);
				return TRUE;
			}
			do {
				if (FileFindData.cFileName[0] == L'.')
					continue;
				wstring FoundFilePath = filePath + L"\\" + FileFindData.cFileName;
				// Limit Input File Name Length
				FoundNameLength = (int) FoundFilePath.length();
				if (FoundNameLength >= 220) {
					wstring Message = STRING_TEXT_TOO_LONG_PATH_MESSAGE + L" (" + itos(FoundNameLength) + STRING_TEXT_TOO_LONG_PATH_MESSAGE_COUNT + L"/" + itos(220) + STRING_TEXT_TOO_LONG_PATH_MESSAGE_COUNT + L")\n" + FoundFilePath;
					MessageBoxW(hWnd, Message.c_str(), STRING_TEXT_TOO_LONG_PATH_TITLE.c_str(), MB_ICONERROR | MB_SYSTEMMODAL | MB_OK);
					continue;
				}
				if (FILE_ATTRIBUTE_DIRECTORY & FileFindData.dwFileAttributes)
				{
					ConvertOption NewFolderConvertOption = FolderConvertOption;
					NewFolderConvertOption.setInputFilePath(FoundFilePath.c_str());
					NewFolderConvertOption.setOutputFolderName(NewFolderConvertOption.getOutputFolderName() + L"\\" + FileFindData.cFileName);
					NewFolderConvertOption.setNoLabel(true);
					FolderSearchQueue.push(NewFolderConvertOption);
					continue;
				}
				else if (FILE_ATTRIBUTE_ARCHIVE & FileFindData.dwFileAttributes) {
					FolderConvertOption.setInputFilePath(FoundFilePath.c_str());
					FolderConvertOption.setNoLabel(true);
					SnowSetting::CurrentConverter->addQueue(&FolderConvertOption);
				}
				else
					continue;

			} while (FindNextFile(hFind, &FileFindData) != NULL);

			FindClose(hFind);
			FolderSearchQueue.pop();
		}

	}

	if (FILE_ATTRIBUTE_ARCHIVE & FileAttribute)
		SnowSetting::CurrentConverter->addQueue(convertOption);

	return TRUE;
}

void prtTextBorder(HDC hdc, int x, int y, LPCWSTR str, int c, COLORREF borderColor, int borderSize, COLORREF textColor) {
	COLORREF OldColor = textColor;
	OldColor = GetTextColor(hdc);
	SetTextColor(hdc, borderColor);

	for (int i = -borderSize; i <= borderSize; i++)
		for (int j = -borderSize; j <= borderSize; j++)
			TextOut(hdc, x + i, y + j, str, c);
	SetTextColor(hdc, OldColor);
	TextOut(hdc, x, y, str, c);
}