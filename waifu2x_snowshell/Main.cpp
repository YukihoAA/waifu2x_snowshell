#include "Main.h"

HINSTANCE g_hInst;

BOOL is64bit = FALSE;
LPWSTR lpszClassCredit = L"CreditWindow";

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	HWND hWnd;
	MSG msg;
	BOOL bIsWow64;
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

	// OpenCV 3.2 does not supports x86 system.
	if (IsWow64Process(GetCurrentProcess(), &bIsWow64))
		is64bit = TRUE;
	else {
		MessageBox(NULL, L"SnowShell does not supports x86 system\n\nMore Information: https://github.com/YukihoAA/waifu2x_snowshell/releases", L"Warning", MB_OK | MB_ICONERROR);
	}

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
	HBITMAP hBitmap;
	HFONT hOldFont;
	HBITMAP hOldBitmap;
	HBITMAP hOldBGBitmap;
	BITMAP bit;
	PAINTSTRUCT ps;
	HDC hdc, hBitDC, hMemDC;
	RECT rt;
	wstring LangName;

	switch (uMsg) {
	case WM_CREATE:
		SnowSetting::Init();
		SnowSetting::getTexts(&UIText);
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
				Execute(hWnd, dragFiles.at(i).c_str());
			}

			DragAcceptFiles(hWnd, TRUE);
		}
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case MENU_FILE_IMAGE_SEL: {
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
					Execute(hWnd, ofn.lpstrFile);
				}
			}
			return TRUE;
		case MENU_FILE_CREDIT: {
				HWND desktop = GetDesktopWindow();
				GetWindowRect(desktop, &rt);
				//ShowWindow(CreateWindow(lpszClassCredit, L"waifu2x - SnowShell v1.0", WS_POPUP | WS_BORDER, (rt.right - 200) / 2, (rt.bottom - 100) / 2, 300, 150, NULL, NULL, g_hInst, NULL), TRUE);
				if(SnowSetting::getLang()==0)
					MessageBox(hWnd, L"제작자: 유키 (yukiho5048@naver.com)\n\nTwitter: https://twitter.com/YukihoAA\nGitHub: https://github.com/YukihoAA\n", L"정보", MB_OK);
				else
					MessageBox(hWnd, L"Creator: Yuki (yukiho5048@naver.com)\n\nTwitter: https://twitter.com/YukihoAA\nGitHub: https://github.com/YukihoAA\n", L"Credit", MB_OK);
			}
			return TRUE;
		case MENU_FILE_QUIT:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			return TRUE;
		case MENU_NOISE_NONE:
		case MENU_NOISE_LOW:
		case MENU_NOISE_HIGH:
		case MENU_NOISE_MAX:
			if (LOWORD(wParam) == MENU_NOISE_MAX) {
				SnowSetting::checkNoise(hMenu, NOISE_MAX);
			}
			else
				SnowSetting::checkNoise(hMenu, LOWORD(wParam) - MENU_NOISE_NONE);
			UIText[0]=SnowSetting::getNoiseText();
			InvalidateRect(hWnd, NULL, TRUE);
			return TRUE;
		case MENU_SCALE_x1_0:
		case MENU_SCALE_x1_5:
		case MENU_SCALE_x1_6:
		case MENU_SCALE_x2_0:
			SnowSetting::checkScale(hMenu, LOWORD(wParam) - MENU_SCALE_x1_0);
			UIText[1] = SnowSetting::getScaleText();
			InvalidateRect(hWnd, NULL, TRUE);
			return TRUE;
		case MENU_CPU_MID:
		case MENU_CPU_HIGH:
		case MENU_CPU_FULL:
			SnowSetting::checkCPU(hMenu, LOWORD(wParam) - MENU_CPU_MID);
			UIText[2] = SnowSetting::getCPUText();
			InvalidateRect(hWnd, NULL, TRUE);
			return TRUE;
		case MENU_EXPORT_SAME:
		case MENU_EXPORT_NEW:
			SnowSetting::checkExport(hMenu, LOWORD(wParam) - MENU_EXPORT_SAME);
			UIText[3] = SnowSetting::getExportText();
			InvalidateRect(hWnd, NULL, TRUE);
			return TRUE;
		case MENU_CONFIRM_SHOW:
		case MENU_CONFIRM_SKIP:
			SnowSetting::checkConfirm(hMenu, LOWORD(wParam) - MENU_CONFIRM_SHOW);
			UIText[4] = SnowSetting::getConfirmText();
			InvalidateRect(hWnd, NULL, TRUE);
			return TRUE;
		case MENU_LANG_KO:
		case MENU_LANG_EN:
		case MENU_LANG_JP:
		case MENU_LANG_CN:
			SnowSetting::checkLang(hMenu, LOWORD(wParam) - MENU_LANG_KO);
			SnowSetting::getTexts(&UIText);
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
			prtTextBorder(hMemDC, 4, 133 + 33 * i, UIText[i]->c_str(), (int) UIText[i]->length(), RGB(0x21, 0x21, 0x21), 2);

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

BOOL Execute(HWND hWnd, LPCWSTR fileName) {

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

	if (SnowSetting::getConfirm() == CONFIRM_SHOW) {
		int x, y;
		GetImageSize(fileName, &x, &y);
		if(x*y>100000 && MessageBox(hWnd, STRING_TEXT_CONFIRM_MESSAGE.c_str(), STRING_TEXT_CONFIRM_TITLE.c_str(), MB_YESNO | MB_ICONEXCLAMATION | MB_TOPMOST) == IDNO)
		return FALSE;
	}

	SHELLEXECUTEINFO si;
	WCHAR param[MAX_PATH]=L"";
	WCHAR lpDir[MAX_PATH] = L"";
	lstrcpyW(param, SnowSetting::BuildParam(fileName).c_str());
	lstrcpyW(lpDir, (SnowSetting::CurrPath + L"\\waifu2x-converter").c_str());

	memset(&si, 0, sizeof(SHELLEXECUTEINFO));
	si.cbSize=sizeof(SHELLEXECUTEINFO);
	si.nShow = SW_SHOW;
	si.lpVerb = L"open";
	si.lpParameters = param;
	si.hwnd = hWnd;
	si.lpDirectory = lpDir;
	si.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;

	if (is64bit && FileExists(SnowSetting::CONVERTER_CAFFE_EXE.c_str()) && SnowSetting::getCPU() == CPU_FULL) {
		si.lpFile = SnowSetting::CONVERTER_CAFFE_EXE.c_str();
		ShellExecuteEx(&si);
		WaitForSingleObject(si.hProcess, INFINITE);
	
	}
	else if (is64bit && FileExists(SnowSetting::CONVERTER_x64_EXE.c_str())) {
		si.lpFile = SnowSetting::CONVERTER_x64_EXE.c_str();
		ShellExecuteEx(&si);
		WaitForSingleObject(si.hProcess, INFINITE);
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

// orig code from: https://stackoverflow.com/questions/10111784/get-image-resolution-from-image-file#comment12969712_10111784
bool GetImageSize(LPCWSTR fn, int *x, int *y)
{
	FILE *f;
	_wfopen_s(&f, fn, L"rb");
	*x = 0, *y = 0;

	if (f == 0) return false;

	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (len<24) {
		fclose(f);
		return false;
	}

	// Strategy:
	// reading GIF dimensions requires the first 10 bytes of the file
	// reading PNG dimensions requires the first 24 bytes of the file
	// reading JPEG dimensions requires scanning through jpeg chunks
	// In all formats, the file is at least 24 bytes big, so we'll read that always
	unsigned char buf[24]; fread(buf, 1, 24, f);


	// For JPEGs, we need to read the first 12 bytes of each chunk.
	// We'll read those 12 bytes at buf+2...buf+14, i.e. overwriting the existing buf.
	if ((buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF && buf[3] == 0xE0 && buf[6] == 'J' && buf[7] == 'F' && buf[8] == 'I' && buf[9] == 'F') ||
		(buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF && buf[3] == 0xE1 && buf[6] == 'E' && buf[7] == 'x' && buf[8] == 'i' && buf[9] == 'f'))
	{
		long pos = 2;
		while (buf[2] == 0xFF)
		{
			if (buf[3] == 0xC0 || buf[3] == 0xC1 || buf[3] == 0xC2 || buf[3] == 0xC3 || buf[3] == 0xC9 || buf[3] == 0xCA || buf[3] == 0xCB)
				break;

			pos += 2 + (buf[4] << 8) + buf[5];
			if (pos + 12>len) break;
			fseek(f, pos, SEEK_SET);
			fread(buf + 2, 1, 12, f);
		}
	}


	fclose(f);

	// JPEG: (first two bytes of buf are first two bytes of the jpeg file; rest of buf is the DCT frame
	if (buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF)
	{
		*y = (buf[7] << 8) + buf[8];
		*x = (buf[9] << 8) + buf[10];

		return true;
	}

	// GIF: first three bytes say "GIF", next three give version number. Then dimensions
	if (buf[0] == 'G' && buf[1] == 'I' && buf[2] == 'F')
	{
		*x = buf[6] + (buf[7] << 8);
		*y = buf[8] + (buf[9] << 8);
		return true;
	}

	// PNG: the first frame is by definition an IHDR frame, which gives dimensions
	if (buf[0] == 0x89 && buf[1] == 'P' && buf[2] == 'N' && buf[3] == 'G' && buf[4] == 0x0D && buf[5] == 0x0A && buf[6] == 0x1A && buf[7] == 0x0A && buf[12] == 'I' && buf[13] == 'H' && buf[14] == 'D' && buf[15] == 'R')
	{
		*x = (buf[16] << 24) + (buf[17] << 16) + (buf[18] << 8) + (buf[19] << 0);
		*y = (buf[20] << 24) + (buf[21] << 16) + (buf[22] << 8) + (buf[23] << 0);

		return true;
	}

	return false;
}