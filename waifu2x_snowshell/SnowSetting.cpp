#include "Main.h"

#define SETTING_VER_MINIMUM 7

SnowSetting *SnowSetting::Singletone;
wstring SnowSetting::NewPath;
wstring SnowSetting::CurrPath;
wstring SnowSetting::LangPath;
wstring SnowSetting::INIPath;
Converter SnowSetting::CONVERTER_CPP_x86;
Converter SnowSetting::CONVERTER_CPP_x64;
Converter SnowSetting::CONVERTER_CAFFE;
Converter* SnowSetting::CurrentConverter;
int SnowSetting::CoreNum;
bool SnowSetting::IsCudaAvailable;
bool SnowSetting::IsCPU;

const int SnowSetting::LangNum = 4;
wstring SnowSetting::LangName[4] = { L"한국어", L"English", L"日本語", L"中文" };
wstring SnowSetting::LangFile[4] = { L"Korean.ini", L"English.ini", L"Japanese.ini", L"Chinese.ini" };

SnowSetting::SnowSetting()
{
	WCHAR path[MAX_PATH];
	GetModuleFileName(NULL, path, MAX_PATH);
	//AddFontResource(L"font.ttf");
	CurrPath = path;
	CurrPath=CurrPath.substr(0,CurrPath.find_last_of(L'\\'));
	NewPath = L"output";
	INIPath = CurrPath + L"\\config.ini";
	LangPath = CurrPath + L"\\Lang";
	CONVERTER_CPP_x86 = Converter(CurrPath + L"\\waifu2x-converter-x86\\waifu2x-converter_x86.exe", false);
	CONVERTER_CPP_x64 = Converter(CurrPath + L"\\waifu2x-converter\\waifu2x-converter-cpp.exe");
	CONVERTER_CAFFE = Converter(CurrPath + L"\\waifu2x-caffe\\waifu2x-caffe-cui.exe", true, true, true);
	CoreNum = thread::hardware_concurrency();
	IsCudaAvailable = checkCuda();

	Noise = 1;
	Scale = 2;
	CPU = 2;
	Export = 0;
	Confirm = 0;
	Lang = 1;
}

SnowSetting::~SnowSetting()
{
	//RemoveFontResource(L"font.ttf");
	saveSetting();
}

SnowSetting *SnowSetting::Init()
{
	if (Singletone == nullptr) {
		Singletone = new SnowSetting();
		loadSetting();
	}
	return Singletone;
}

bool SnowSetting::checkCuda() {
	bool cuda = false;
	IsCPU = false;
	HANDLE hRead, hWrite;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	WCHAR param[MAX_PATH] = L"";
	const static size_t bufferSize = 128;


	if (!CONVERTER_CPP_x64.getAvailable() && CONVERTER_CAFFE.getAvailable())
		return true;

	CreatePipe(&hRead, &hWrite, NULL, bufferSize);
	SetHandleInformation(hWrite, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

	memset(&si, 0, sizeof(STARTUPINFO));

	si.cb = sizeof(si);
	si.hStdOutput = hWrite;
	si.hStdError = hWrite;
	si.dwFlags = STARTF_USESTDHANDLES;

	if (CONVERTER_CPP_x64.getAvailable())
		lstrcpy(param, CONVERTER_CPP_x64.getExePath().c_str());
	else if (CONVERTER_CPP_x86.getAvailable())
		lstrcpy(param, CONVERTER_CPP_x86.getExePath().c_str());
	else
		return cuda;
	lstrcat(param, L" --list-processor");

	BOOL isExecuted = CreateProcess(NULL, param, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL, CurrPath.c_str(), &si, &pi);

	if (isExecuted)
	{
		CloseHandle(hWrite);

		DWORD len;
		char s[bufferSize] = "";
		string st;

		ReadFile(hRead, s, bufferSize - 1, &len, 0);
		st = s;
		size_t firstLF = st.find_first_of('\n');

		if (firstLF != string::npos)
			st[firstLF] = '\0';

		if (st.find_last_of('=') != string::npos)
			CoreNum = atoi(st.substr(st.find_last_of('=') + 1).c_str());

		if (st.find("CUDA") != string::npos)
			cuda = true;

		if (st.find("FMA") != string::npos)
			IsCPU = true;

		CloseHandle(hRead);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	return cuda;
}

bool SnowSetting::getCudaAvailable() {
	return IsCudaAvailable;
}

bool SnowSetting::getIsCPU() {
	return IsCPU;
}

int SnowSetting::getCoreNum() {
	return CoreNum;
}

void SnowSetting::loadLocale()
{
	if (Singletone == nullptr)
		Init();

	WCHAR buf[200];
	wstring Key, Value, Section;
	wstring LangFileName = LangPath + L"\\" + LangFile[getLang()];

	if (!FileExists(LangFileName.c_str())) {
		CreateDirectory(L"Lang", NULL);
		HRSRC hSrc = NULL;
		if (LangFileName.find(L"Korean") != std::string::npos) hSrc = FindResource(g_hInst, MAKEINTRESOURCE(IDR_LANG_KO), L"LANG");
		else if (LangFileName.find(L"Japanese") != std::string::npos) hSrc = FindResource(g_hInst, MAKEINTRESOURCE(IDR_LANG_JP), L"LANG");
		else if (LangFileName.find(L"Chinese") != std::string::npos) hSrc = FindResource(g_hInst, MAKEINTRESOURCE(IDR_LANG_CN), L"LANG");
		else if (LangFileName.find(L"English") != std::string::npos) hSrc = FindResource(g_hInst, MAKEINTRESOURCE(IDR_LANG_EN), L"LANG");
		else {
			MessageBox(NULL, L"No Lang File", L"Error", MB_ICONWARNING | MB_OK);
			setLang(1);
		}
		if (hSrc != NULL) {
			HGLOBAL hRes = LoadResource(g_hInst, hSrc);
			LPVOID memRes = LockResource(hRes);
			DWORD sizeRes = SizeofResource(g_hInst, hSrc);
			HANDLE hFile = CreateFile(LangFileName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			DWORD dwWritten = 0;

			WriteFile(hFile, memRes, sizeRes, &dwWritten, NULL);
			if (hFile != NULL)
				CloseHandle(hFile);
		}
	}
	Section = L"SnowShell";

	Key = L"INT_SETTING_VER";
	INT_SETTING_VER = GetPrivateProfileIntW(Section.c_str(), Key.c_str(), 0, LangFileName.c_str());

	if (INT_SETTING_VER < SETTING_VER_MINIMUM) {
		DeleteFile(LangFileName.c_str());
		loadSetting();
		return;
	}

	Section = L"Menu";

	Key = L"STRING_MENU_FILE";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"File", buf, 200, LangFileName.c_str());
	STRING_MENU_FILE = buf;

	Key = L"STRING_MENU_NOISE";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Noise Reduce", buf, 200, LangFileName.c_str());
	STRING_MENU_NOISE = buf;

	Key = L"STRING_MENU_SCALE";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Scale", buf, 200, LangFileName.c_str());
	STRING_MENU_SCALE = buf;

	Key = L"STRING_MENU_CPU";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"CPU", buf, 200, LangFileName.c_str());
	STRING_MENU_CPU = buf;

	Key = L"STRING_MENU_GPU";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"GPU", buf, 200, LangFileName.c_str());
	STRING_MENU_GPU = buf;

	Key = L"STRING_MENU_EXPORT";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Export", buf, 200, LangFileName.c_str());
	STRING_MENU_EXPORT = buf;

	Key = L"STRING_MENU_CONFIRM";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Warning", buf, 200, LangFileName.c_str());
	STRING_MENU_CONFIRM = buf;


	Section = L"File";

	Key = L"STRING_MENU_FILE_IMAGE_SEL";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Select Image And Execute", buf, 200, LangFileName.c_str());
	STRING_MENU_FILE_IMAGE_SEL = buf;

	Key = L"STRING_MENU_FILE_CREDIT";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Credit", buf, 200, LangFileName.c_str());
	STRING_MENU_FILE_CREDIT = buf;

	Key = L"STRING_MENU_FILE_QUIT";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Quit", buf, 200, LangFileName.c_str());
	STRING_MENU_FILE_QUIT = buf;


	Section = L"Noise";

	Key = L"STRING_MENU_NOISE_NONE";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"None", buf, 200, LangFileName.c_str());
	STRING_MENU_NOISE_NONE = buf;

	Key = L"STRING_MENU_NOISE_LOW";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Low", buf, 200, LangFileName.c_str());
	STRING_MENU_NOISE_LOW = buf;

	Key = L"STRING_MENU_NOISE_HIGH";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"High", buf, 200, LangFileName.c_str());
	STRING_MENU_NOISE_HIGH = buf;

	Key = L"STRING_MENU_NOISE_VERY_HIGH";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Max", buf, 200, LangFileName.c_str());
	STRING_MENU_NOISE_VERY_HIGH = buf;


	Section = L"Scale";

	Key = L"STRING_MENU_SCALE_x1_0";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"x1.0", buf, 200, LangFileName.c_str());
	STRING_MENU_SCALE_x1_0 = buf;

	Key = L"STRING_MENU_SCALE_x1_5";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"x1.5", buf, 200, LangFileName.c_str());
	STRING_MENU_SCALE_x1_5 = buf;

	Key = L"STRING_MENU_SCALE_x1_6";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"x1.6", buf, 200, LangFileName.c_str());
	STRING_MENU_SCALE_x1_6 = buf;

	Key = L"STRING_MENU_SCALE_x2_0";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"x2.0", buf, 200, LangFileName.c_str());
	STRING_MENU_SCALE_x2_0 = buf;


	Section = L"CPU";

	Key = L"STRING_MENU_CPU_MID";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Half", buf, 200, LangFileName.c_str());
	STRING_MENU_CPU_MID = buf;

	Key = L"STRING_MENU_CPU_HIGH";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Almost All", buf, 200, LangFileName.c_str());
	STRING_MENU_CPU_HIGH = buf;

	Key = L"STRING_MENU_CPU_FULL";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Full Power!!!!", buf, 200, LangFileName.c_str());
	STRING_MENU_CPU_FULL = buf;

	Key = L"STRING_MENU_GPU_OPENCL";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Use OpenCL", buf, 200, LangFileName.c_str());
	STRING_MENU_GPU_OPENCL = buf;

	Key = L"STRING_MENU_GPU_CUDA";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Use CUDA", buf, 200, LangFileName.c_str());
	STRING_MENU_GPU_CUDA = buf;

	Key = L"STRING_MENU_GPU_TTA";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Use CUDA With TTA", buf, 200, LangFileName.c_str());
	STRING_MENU_GPU_TTA = buf;


	Section = L"Export";

	Key = L"STRING_MENU_EXPORT_SAME";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Same Folder of Input File", buf, 200, LangFileName.c_str());
	STRING_MENU_EXPORT_SAME = buf;

	Key = L"STRING_MENU_EXPORT_NEW";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"\"output\" Folder", buf, 200, LangFileName.c_str());
	STRING_MENU_EXPORT_NEW = buf;


	Section = L"Confirm";

	Key = L"STRING_MENU_CONFIRM_SHOW";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Show Warning", buf, 200, LangFileName.c_str());
	STRING_MENU_CONFIRM_SHOW = buf;

	Key = L"STRING_MENU_CONFIRM_SKIP";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Hide Warning", buf, 200, LangFileName.c_str());
	STRING_MENU_CONFIRM_SKIP = buf;


	Section = L"Text";

	Key = L"INT_TEXT_TAB";
	INT_TEXT_TAB = GetPrivateProfileIntW(Section.c_str(), Key.c_str(), 30, LangFileName.c_str());

	Key = L"STRING_TEXT_NOISE";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Anti Noise", buf, 200, LangFileName.c_str());
	STRING_TEXT_NOISE = buf;

	Key = L"STRING_TEXT_NOISE_NONE";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"None", buf, 200, LangFileName.c_str());
	STRING_TEXT_NOISE_NONE = buf;

	Key = L"STRING_TEXT_NOISE_LOW";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Low", buf, 200, LangFileName.c_str());
	STRING_TEXT_NOISE_LOW = buf;

	Key = L"STRING_TEXT_NOISE_HIGH";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"High", buf, 200, LangFileName.c_str());
	STRING_TEXT_NOISE_HIGH = buf;

	Key = L"STRING_TEXT_NOISE_VERY_HIGH";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Max", buf, 200, LangFileName.c_str());
	STRING_TEXT_NOISE_VERY_HIGH = buf;


	Key = L"STRING_TEXT_SCALE";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Zoom Scale", buf, 200, LangFileName.c_str());
	STRING_TEXT_SCALE = buf;

	Key = L"STRING_TEXT_SCALE_x1_0";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"x1.0", buf, 200, LangFileName.c_str());
	STRING_TEXT_SCALE_x1_0 = buf;

	Key = L"STRING_TEXT_SCALE_x1_5";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"x1.5", buf, 200, LangFileName.c_str());
	STRING_TEXT_SCALE_x1_5 = buf;

	Key = L"STRING_TEXT_SCALE_x1_6";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"x1.6", buf, 200, LangFileName.c_str());
	STRING_TEXT_SCALE_x1_6 = buf;

	Key = L"STRING_TEXT_SCALE_x2_0";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"x2.0", buf, 200, LangFileName.c_str());
	STRING_TEXT_SCALE_x2_0 = buf;


	Key = L"STRING_TEXT_CPU";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Thread Num", buf, 200, LangFileName.c_str());
	STRING_TEXT_CPU = buf;

	Key = L"STRING_TEXT_CPU_MID";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Half", buf, 200, LangFileName.c_str());
	STRING_TEXT_CPU_MID = buf;

	Key = L"STRING_TEXT_CPU_HIGH";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Almost All", buf, 200, LangFileName.c_str());
	STRING_TEXT_CPU_HIGH = buf;

	Key = L"STRING_TEXT_CPU_FULL";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"All", buf, 200, LangFileName.c_str());
	STRING_TEXT_CPU_FULL = buf;


	Key = L"STRING_TEXT_GPU";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Converter", buf, 200, LangFileName.c_str());
	STRING_TEXT_GPU = buf;

	Key = L"STRING_TEXT_GPU_OPENCL";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"OpenCL", buf, 200, LangFileName.c_str());
	STRING_TEXT_GPU_OPENCL = buf;

	Key = L"STRING_TEXT_GPU_CUDA";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"CUDA", buf, 200, LangFileName.c_str());
	STRING_TEXT_GPU_CUDA = buf;

	Key = L"STRING_TEXT_GPU_TTA";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"CUDA (with TTA)", buf, 200, LangFileName.c_str());
	STRING_TEXT_GPU_TTA = buf;


	Key = L"STRING_TEXT_EXPORT";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Export Dir", buf, 200, LangFileName.c_str());
	STRING_TEXT_EXPORT = buf;

	Key = L"STRING_TEXT_EXPORT_SAME";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Same Folder", buf, 200, LangFileName.c_str());
	STRING_TEXT_EXPORT_SAME = buf;

	Key = L"STRING_TEXT_EXPORT_NEW";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"\"output\" Folder", buf, 200, LangFileName.c_str());
	STRING_TEXT_EXPORT_NEW = buf;


	Key = L"STRING_TEXT_CONFIRM";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Warning", buf, 200, LangFileName.c_str());
	STRING_TEXT_CONFIRM = buf;

	Key = L"STRING_TEXT_CONFIRM_SHOW";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Show", buf, 200, LangFileName.c_str());
	STRING_TEXT_CONFIRM_SHOW = buf;

	Key = L"STRING_TEXT_CONFIRM_SKIP";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Hide", buf, 200, LangFileName.c_str());
	STRING_TEXT_CONFIRM_SKIP = buf;

	Key = L"STRING_TEXT_CONFIRM_TITLE";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Warning", buf, 200, LangFileName.c_str());
	STRING_TEXT_CONFIRM_TITLE = buf;

	Key = L"STRING_TEXT_CONFIRM_MESSAGE";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Large image will require high spec.\nDo you want to continue?", buf, 200, LangFileName.c_str());
	STRING_TEXT_CONFIRM_MESSAGE = buf;
	size_t nl = STRING_TEXT_CONFIRM_MESSAGE.find(L"\\n");
	while (nl != wstring::npos) {
		STRING_TEXT_CONFIRM_MESSAGE.replace(nl, 2, L"\n");
		nl = STRING_TEXT_CONFIRM_MESSAGE.find(L"\\n");
	}

	Key = L"STRING_TEXT_NOCONVERTER_TITLE";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Error", buf, 200, LangFileName.c_str());
	STRING_TEXT_NOCONVERTER_TITLE = buf;

	Key = L"STRING_TEXT_NOCONVERTER_MESSAGE";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"No converter available.\nPlease check your windows version.", buf, 200, LangFileName.c_str());
	STRING_TEXT_NOCONVERTER_MESSAGE = buf;
	nl = STRING_TEXT_NOCONVERTER_MESSAGE.find(L"\\n");
	while (nl != wstring::npos) {
		STRING_TEXT_NOCONVERTER_MESSAGE.replace(nl, 2, L"\n");
		nl = STRING_TEXT_NOCONVERTER_MESSAGE.find(L"\\n");
	}

}

bool SnowSetting::loadSetting()
{
	if (Singletone == nullptr)
		Init();

	WCHAR buf[MAX_PATH];
	wstring Section = L"SnowShell";
	wstring Key, Value;

	Key = L"Noise";
	setNoise(GetPrivateProfileInt(Section.c_str(), Key.c_str(), 1, INIPath.c_str()));

	Key = L"Scale";
	setScale(GetPrivateProfileInt(Section.c_str(), Key.c_str(), 2, INIPath.c_str()));

	Key = L"CPU";
	setCPU(GetPrivateProfileInt(Section.c_str(), Key.c_str(), 2, INIPath.c_str()));

	Key = L"Export";
	setExport(GetPrivateProfileInt(Section.c_str(), Key.c_str(), 0, INIPath.c_str()));

	Key = L"Confirm";
	setConfirm(GetPrivateProfileInt(Section.c_str(), Key.c_str(), 0, INIPath.c_str()));

	Key = L"Lang";
	int langsel = GetPrivateProfileInt(Section.c_str(), Key.c_str(), -1, INIPath.c_str());
	if (langsel == -1) {
		WCHAR buf[40];
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SENGLANGUAGE, buf, 40);
		for (int i = 0; i < LangNum; i++)
			if (LangFile[i].find(buf) >= 0)
			{
				langsel = i;
				break;
			}
		if (langsel == -1)
			langsel = 1;
	}
	setLang(langsel);
	loadLocale();


	Section = L"Model";

	GetPrivateProfileStringW(Section.c_str(), L"waifu2x_caffe", L"", buf, MAX_PATH, INIPath.c_str());
	CONVERTER_CAFFE.setModelDir(buf);

	GetPrivateProfileStringW(Section.c_str(), L"waifu2x_converter_cpp_x64", L"", buf, MAX_PATH, INIPath.c_str());
	CONVERTER_CPP_x64.setModelDir(buf);

	GetPrivateProfileStringW(Section.c_str(), L"waifu2x_converter_cpp_x86", L"", buf, MAX_PATH, INIPath.c_str());
	CONVERTER_CPP_x86.setModelDir(buf);

	return true;
}

bool SnowSetting::saveSetting()
{
	if (Singletone == nullptr)
		Init();

	wstring Section = L"SnowShell";
	wstring Key, Value;

	Key = L"Noise";
	WritePrivateProfileString(Section.c_str(), Key.c_str(), itos(getNoise()).c_str(), INIPath.c_str());

	Key = L"Scale";
	WritePrivateProfileString(Section.c_str(), Key.c_str(), itos(getScale()).c_str(), INIPath.c_str());

	Key = L"CPU";
	WritePrivateProfileString(Section.c_str(), Key.c_str(), itos(getCPU()).c_str(), INIPath.c_str());

	Key = L"Export";
	WritePrivateProfileString(Section.c_str(), Key.c_str(), itos(getExport()).c_str(), INIPath.c_str());

	Key = L"Confirm";
	WritePrivateProfileString(Section.c_str(), Key.c_str(), itos(getConfirm()).c_str(), INIPath.c_str());

	Key = L"Lang";
	WritePrivateProfileString(Section.c_str(), Key.c_str(), itos(getLang()).c_str(), INIPath.c_str());


	Section = L"Model";

	WritePrivateProfileString(Section.c_str(), L"waifu2x_caffe", CONVERTER_CAFFE.getModelDir().c_str(), INIPath.c_str());

	WritePrivateProfileString(Section.c_str(), L"waifu2x_converter_cpp_x64", CONVERTER_CPP_x64.getModelDir().c_str(), INIPath.c_str());

	WritePrivateProfileString(Section.c_str(), L"waifu2x_converter_cpp_x86", CONVERTER_CPP_x86.getModelDir().c_str(), INIPath.c_str());

	return true;
}

void SnowSetting::loadMenuString(HMENU hMenu)
{
	ModifyMenu(hMenu, 0, MF_BYPOSITION | MF_STRING, 0, STRING_MENU_FILE.c_str());
	ModifyMenu(hMenu, 1, MF_BYPOSITION | MF_STRING, 1, STRING_MENU_NOISE.c_str());
	ModifyMenu(hMenu, 2, MF_BYPOSITION | MF_STRING, 2, STRING_MENU_SCALE.c_str());
	if (SnowSetting::getIsCPU())
		ModifyMenu(hMenu, 3, MF_BYPOSITION | MF_STRING, 3, STRING_MENU_CPU.c_str());
	else
		ModifyMenu(hMenu, 3, MF_BYPOSITION | MF_STRING, 3, STRING_MENU_GPU.c_str());
	ModifyMenu(hMenu, 4, MF_BYPOSITION | MF_STRING, 4, STRING_MENU_EXPORT.c_str());
	ModifyMenu(hMenu, 5, MF_BYPOSITION | MF_STRING, 5, STRING_MENU_CONFIRM.c_str());

	ModifyMenu(hMenu, ID_MENU_FILE_IMGSEL, MF_BYCOMMAND | MF_STRING, ID_MENU_FILE_IMGSEL, STRING_MENU_FILE_IMAGE_SEL.c_str());
	ModifyMenu(hMenu, ID_MENU_FILE_CREDIT, MF_BYCOMMAND | MF_STRING, ID_MENU_FILE_CREDIT, STRING_MENU_FILE_CREDIT.c_str());
	ModifyMenu(hMenu, ID_MENU_FILE_QUIT, MF_BYCOMMAND | MF_STRING, ID_MENU_FILE_QUIT, STRING_MENU_FILE_QUIT.c_str());

	ModifyMenu(hMenu, ID_MENU_NOISE_NONE, MF_BYCOMMAND | MF_STRING, ID_MENU_NOISE_NONE, STRING_MENU_NOISE_NONE.c_str());
	ModifyMenu(hMenu, ID_MENU_NOISE_LOW, MF_BYCOMMAND | MF_STRING, ID_MENU_NOISE_LOW, STRING_MENU_NOISE_LOW.c_str());
	ModifyMenu(hMenu, ID_MENU_NOISE_HIGH, MF_BYCOMMAND | MF_STRING, ID_MENU_NOISE_HIGH, STRING_MENU_NOISE_HIGH.c_str());
	ModifyMenu(hMenu, ID_MENU_NOISE_VERY_HIGH, MF_BYCOMMAND | MF_STRING, ID_MENU_NOISE_VERY_HIGH, STRING_MENU_NOISE_VERY_HIGH.c_str());

	ModifyMenu(hMenu, ID_MENU_SCALE_x1_0, MF_BYCOMMAND | MF_STRING, ID_MENU_SCALE_x1_0, STRING_MENU_SCALE_x1_0.c_str());
	ModifyMenu(hMenu, ID_MENU_SCALE_x1_5, MF_BYCOMMAND | MF_STRING, ID_MENU_SCALE_x1_5, STRING_MENU_SCALE_x1_5.c_str());
	ModifyMenu(hMenu, ID_MENU_SCALE_x1_6, MF_BYCOMMAND | MF_STRING, ID_MENU_SCALE_x1_6, STRING_MENU_SCALE_x1_6.c_str());
	ModifyMenu(hMenu, ID_MENU_SCALE_x2_0, MF_BYCOMMAND | MF_STRING, ID_MENU_SCALE_x2_0, STRING_MENU_SCALE_x2_0.c_str());

	if (SnowSetting::getIsCPU()) { // FOR CPU
		ModifyMenu(hMenu, ID_MENU_CPU_MID, MF_BYCOMMAND | MF_STRING, ID_MENU_CPU_MID, STRING_MENU_CPU_MID.c_str());
		ModifyMenu(hMenu, ID_MENU_CPU_HIGH, MF_BYCOMMAND | MF_STRING, ID_MENU_CPU_HIGH, STRING_MENU_CPU_HIGH.c_str());
		ModifyMenu(hMenu, ID_MENU_CPU_FULL, MF_BYCOMMAND | MF_STRING, ID_MENU_CPU_FULL, STRING_MENU_CPU_FULL.c_str());
	}
	else if (SnowSetting::getCudaAvailable() && SnowSetting::CONVERTER_CAFFE.getAvailable()) {	// CUDA OPTION
		ModifyMenu(hMenu, ID_MENU_CPU_MID, MF_BYCOMMAND | MF_STRING, ID_MENU_CPU_MID, STRING_MENU_GPU_OPENCL.c_str());
		ModifyMenu(hMenu, ID_MENU_CPU_HIGH, MF_BYCOMMAND | MF_STRING, ID_MENU_CPU_HIGH, STRING_MENU_GPU_CUDA.c_str());
		ModifyMenu(hMenu, ID_MENU_CPU_FULL, MF_BYCOMMAND | MF_STRING, ID_MENU_CPU_FULL, STRING_MENU_GPU_TTA.c_str());
	}
	else { // OPENCL
		ModifyMenu(hMenu, ID_MENU_CPU_MID, MF_BYCOMMAND | MF_STRING, ID_MENU_CPU_MID, STRING_MENU_CPU_MID.c_str());
		ModifyMenu(hMenu, ID_MENU_CPU_HIGH, MF_BYCOMMAND | MF_STRING, ID_MENU_CPU_HIGH, STRING_MENU_CPU_HIGH.c_str());
		ModifyMenu(hMenu, ID_MENU_CPU_FULL, MF_BYCOMMAND | MF_STRING, ID_MENU_CPU_FULL, STRING_MENU_CPU_FULL.c_str());
	}

	ModifyMenu(hMenu, ID_MENU_EXPORT_SAME, MF_BYCOMMAND | MF_STRING, ID_MENU_EXPORT_SAME, STRING_MENU_EXPORT_SAME.c_str());
	ModifyMenu(hMenu, ID_MENU_EXPORT_NEW, MF_BYCOMMAND | MF_STRING, ID_MENU_EXPORT_NEW, STRING_MENU_EXPORT_NEW.c_str());

	ModifyMenu(hMenu, ID_MENU_CONFIRM_SHOW, MF_BYCOMMAND | MF_STRING, ID_MENU_CONFIRM_SHOW, STRING_MENU_CONFIRM_SHOW.c_str());
	ModifyMenu(hMenu, ID_MENU_CONFIRM_HIDE, MF_BYCOMMAND | MF_STRING, ID_MENU_CONFIRM_HIDE, STRING_MENU_CONFIRM_SKIP.c_str());
}

int SnowSetting::getNoise()
{
	if (Singletone == nullptr)
		Init();

	return Singletone->Noise;
}

int SnowSetting::getScale()
{
	if (Singletone == nullptr)
		Init();

	return Singletone->Scale;
}

int SnowSetting::getCPU()
{
	if (Singletone == nullptr)
		Init();

	return Singletone->CPU;
}

int SnowSetting::getExport()
{
	if (Singletone == nullptr)
		Init();

	return Singletone->Export;
}

BOOL SnowSetting::getConfirm()
{
	if (Singletone == nullptr)
		Init();

	return Singletone->Confirm;
}

int SnowSetting::getLang()
{
	if (Singletone == nullptr)
		Init();

	return Singletone->Lang;
}

wstring SnowSetting::getLangName()
{
	return LangFile[getLang()];
}

void SnowSetting::setNoise(int Noise)
{
	if (Singletone == nullptr)
		Init();

	if (Noise > NOISE_MAX || Noise < 0)
		Noise = 0;

	Singletone->Noise = Noise;
}

void SnowSetting::setScale(int Scale)
{
	if (Singletone == nullptr)
		Init();

	if (Scale > SCALE_MAX || Scale < 0)
		Scale = 0;

	Singletone->Scale = Scale;
}

void SnowSetting::setCPU(int CPU)
{
	if (Singletone == nullptr)
		Init();

	if (CPU > CPU_MAX || CPU < 0)
		CPU = 0;

	Singletone->CPU = CPU;
}

void SnowSetting::setExport(int Export)
{
	if (Singletone == nullptr)
		Init();

	if (Export > EXPORT_MAX || Export < 0)
		Export = 0;

	Singletone->Export = Export;
}

void SnowSetting::setConfirm(BOOL Confirm)
{
	if (Singletone == nullptr)
		Init();

	Singletone->Confirm = Confirm;
}

void SnowSetting::setLang(int Lang)
{
	if (Singletone == nullptr)
		Init();

	if (Lang >= LangNum || Lang < 0)
		Lang = 0;

	Singletone->Lang = Lang;
	loadLocale();
}

void SnowSetting::checkMenuAll(HMENU hMenu)
{
	checkNoise(hMenu);
	checkScale(hMenu);
	checkCPU(hMenu);
	checkExport(hMenu);
	checkConfirm(hMenu);
	checkLang(hMenu);
}

void SnowSetting::checkNoise(HMENU hMenu, int sel)
{
	HMENU hSubMenu = GetSubMenu(hMenu, 1);

	if (sel != -1)
		setNoise(sel);

	for (int i = 0; i <= NOISE_MAX; i++)
		CheckMenuItem(hSubMenu, i, MF_BYPOSITION | MF_UNCHECKED);
	CheckMenuItem(hSubMenu, getNoise(), MF_BYPOSITION | MF_CHECKED);
}

void SnowSetting::checkScale(HMENU hMenu, int sel)
{
	HMENU hSubMenu = GetSubMenu(hMenu, 2);

	if (sel != -1)
		setScale(sel);

	for (int i = 0; i <= SCALE_MAX; i++)
		CheckMenuItem(hSubMenu, i, MF_BYPOSITION | MF_UNCHECKED);
	CheckMenuItem(hSubMenu, getScale(), MF_BYPOSITION | MF_CHECKED);
}

void SnowSetting::checkCPU(HMENU hMenu, int sel)
{
	HMENU hSubMenu = GetSubMenu(hMenu, 3);

	if (sel != -1)
		setCPU(sel);

	for (int i = 0; i <= CPU_MAX; i++)
		CheckMenuItem(hSubMenu, i, MF_BYPOSITION | MF_UNCHECKED);
	CheckMenuItem(hSubMenu, getCPU(), MF_BYPOSITION | MF_CHECKED);
}

void SnowSetting::checkExport(HMENU hMenu, int sel)
{
	HMENU hSubMenu = GetSubMenu(hMenu, 4);

	if (sel != -1)
		setExport(sel);

	for (int i = 0; i <= EXPORT_MAX; i++)
		CheckMenuItem(hSubMenu, i, MF_BYPOSITION | MF_UNCHECKED);
	CheckMenuItem(hSubMenu, getExport(), MF_BYPOSITION | MF_CHECKED);
}

void SnowSetting::checkConfirm(HMENU hMenu, int sel)
{
	HMENU hSubMenu = GetSubMenu(hMenu, 5);

	if (sel != -1)
		setConfirm(sel);

	for (int i = 0; i <= CONFIRM_MAX; i++)
		CheckMenuItem(hSubMenu, i, MF_BYPOSITION | MF_UNCHECKED);
	CheckMenuItem(hSubMenu, getConfirm(), MF_BYPOSITION | MF_CHECKED);
}

void SnowSetting::checkLang(HMENU hMenu, int sel)
{
	HMENU hSubMenu = GetSubMenu(hMenu, 6);

	if (sel != -1)
		setLang(sel);

	for (int i = 0; i < LangNum; i++)
		CheckMenuItem(hSubMenu, i, MF_BYPOSITION | MF_UNCHECKED);
	CheckMenuItem(hSubMenu, getLang(), MF_BYPOSITION | MF_CHECKED);
}

void SnowSetting::getTexts(wstring*(*UITitleText)[5], wstring*(*UIText)[5]) {
	(*UITitleText)[0] = &STRING_TEXT_NOISE;
	(*UITitleText)[1] = &STRING_TEXT_SCALE;
	if(!IsCudaAvailable || !SnowSetting::CONVERTER_CAFFE.getAvailable())
		(*UITitleText)[2] = &STRING_TEXT_CPU;
	else
		(*UITitleText)[2] = &STRING_TEXT_GPU;
	(*UITitleText)[3] = &STRING_TEXT_EXPORT;
	(*UITitleText)[4] = &STRING_TEXT_CONFIRM;

	(*UIText)[0] = getNoiseText();
	(*UIText)[1] = getScaleText();
	if (!IsCudaAvailable || !SnowSetting::CONVERTER_CAFFE.getAvailable())
		(*UIText)[2] = getCPUText();
	else
		(*UIText)[2] = getGPUText();
	(*UIText)[3] = getExportText();
	(*UIText)[4] = getConfirmText();
}

wstring * SnowSetting::getNoiseText()
{
	switch (getNoise()) {
	case NOISE_NONE:
		return &STRING_TEXT_NOISE_NONE;
	case NOISE_LOW:
		return &STRING_TEXT_NOISE_LOW;
	case NOISE_HIGH:
		return &STRING_TEXT_NOISE_HIGH;
	case NOISE_VERY_HIGH:
		return &STRING_TEXT_NOISE_VERY_HIGH;
	}
	return nullptr;
}

wstring * SnowSetting::getScaleText()
{
	switch (getScale()) {
	case SCALE_x1_0:
		return &STRING_TEXT_SCALE_x1_0;
	case SCALE_x1_5:
		return &STRING_TEXT_SCALE_x1_5;
	case SCALE_x1_6:
		return &STRING_TEXT_SCALE_x1_6;
	case SCALE_x2_0:
		return &STRING_TEXT_SCALE_x2_0;
	}
	return nullptr;
}

wstring * SnowSetting::getCPUText()
{
	switch (getCPU()) {
	case CPU_MID:
		return &STRING_TEXT_CPU_MID;
	case CPU_HIGH:
		return &STRING_TEXT_CPU_HIGH;
	case CPU_FULL:
		return &STRING_TEXT_CPU_FULL;
	}
	return nullptr;
}

wstring * SnowSetting::getGPUText()
{
	switch (getCPU()) {
	case CPU_MID:
		return &STRING_TEXT_GPU_OPENCL;
	case CPU_HIGH:
		return &STRING_TEXT_GPU_CUDA;
	case CPU_FULL:
		return &STRING_TEXT_GPU_TTA;
	}
	return nullptr;
}

wstring * SnowSetting::getExportText()
{
	switch (getExport()) {
	case EXPORT_SAME:
		return &STRING_TEXT_EXPORT_SAME;
	case EXPORT_NEW:
		return &STRING_TEXT_EXPORT_NEW;
	}
	return nullptr;
}

wstring * SnowSetting::getConfirmText()
{
	if (getConfirm() == CONFIRM_SHOW)
		return &STRING_TEXT_CONFIRM_SHOW;
	else
		return &STRING_TEXT_CONFIRM_SKIP;
}

wstring itos(int n) {
	wstringstream ss;

	ss << n;

	return ss.str();
}

BOOL FileExists(LPCWSTR file) {
	WIN32_FIND_DATA FindFileData;
	HANDLE handle = FindFirstFile(file, &FindFileData);
	BOOL found = handle != INVALID_HANDLE_VALUE;
	if (found) {
		FindClose(handle);
	}
	return found;
}

BOOL IsDirectory(LPCWSTR path) {
	return FILE_ATTRIBUTE_DIRECTORY & GetFileAttributes(path);
}

int contain(wstring str, wstring find) {
	int count = 0;
	size_t index = 0;
	while ((index = str.find(find, index + 1)) != std::string::npos) {
		count++;
	}
	return count;
}

void PrintDebugMessage(double db, wstring title) {
	wstringstream ss;
	ss << db;
	MessageBox(NULL, ss.str().c_str(), title.c_str(), MB_OK);
}
void PrintDebugMessage(string str) {
	MessageBoxA(NULL, str.c_str(), "Debug", MB_OK);
}
void PrintDebugMessage(wstring str) {
	MessageBox(NULL, str.c_str(), L"Debug", MB_OK);
}