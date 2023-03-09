﻿#include "Main.h"

SnowSetting *SnowSetting::Singletone;
wstring SnowSetting::OutputDirName;
wstring SnowSetting::CurrPath;
wstring SnowSetting::LangPath;
wstring SnowSetting::INIPath;
Converter_Cpp SnowSetting::CONVERTER_CPP;
Converter_Caffe SnowSetting::CONVERTER_CAFFE;
Converter_Vulkan SnowSetting::CONVERTER_VULKAN;
Converter_Cugan SnowSetting::CONVERTER_CUGAN;
Converter_Esrgan SnowSetting::CONVERTER_ESRGAN;
Converter* SnowSetting::CurrentConverter;
int SnowSetting::CoreNum;
bool SnowSetting::IsCPU;

const int SnowSetting::LangNum = 10;
wstring SnowSetting::LangName[LangNum] = { L"한국어", L"English", L"日本語", L"中文", L"Deutsch", L"Svenska" , L"Portuguese", L"Ukrainian", L"Svenska", L"Dutch" };
wstring SnowSetting::LangFile[LangNum] = { L"Korean.ini", L"English.ini", L"Japanese.ini", L"Chinese.ini", L"German.ini", L"Swedish.ini", L"Portuguese.ini", L"Ukrainian.ini", L"Swedish.ini", L"Dutch.ini" };


const int SnowSetting::VulkanScaleNum = 4;
const int SnowSetting::CuganScaleNum = 1;
LPWSTR SnowSetting::VulkanScale[VulkanScaleNum] = { L"4.0", L"8.0", L"16.0", L"32.0" };

SnowSetting::SnowSetting()
{
	LPWSTR path = new WCHAR[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, path);
	GetModuleFileName(NULL, path, MAX_PATH);
	CurrPath = path;
	CurrPath = CurrPath.substr(0, CurrPath.find_last_of(L'\\'));
	OutputDirName = L"output";
	INIPath = CurrPath + L"\\config.ini";
	LangPath = CurrPath + L"\\Lang";
	CONVERTER_CPP = Converter_Cpp(CurrPath + L"\\waifu2x-converter\\waifu2x-converter-cpp.exe");
	CONVERTER_CAFFE = Converter_Caffe(CurrPath + L"\\waifu2x-caffe\\waifu2x-caffe-cui.exe");
	CONVERTER_VULKAN = Converter_Vulkan(CurrPath + L"\\waifu2x-ncnn-vulkan\\waifu2x-ncnn-vulkan.exe");
	CONVERTER_CUGAN = Converter_Cugan(CurrPath + L"\\realcugan-vulkan\\realcugan-ncnn-vulkan.exe");
	CONVERTER_ESRGAN = Converter_Esrgan(CurrPath + L"\\realesrgan-vulkan\\realesrgan-ncnn-vulkan.exe");
	CoreNum = thread::hardware_concurrency();
	CurrentConverter = nullptr;
	OutputExt = L"png";
	delete[] path;

	Noise = NOISE_MID;
	Scale = SCALE_x1_6;
	GPU = GPU_GPU_MODE;
	TTA = TTA_DISABLED;
	Export = 0;
	Confirm = 0;
	Lang = 1;
	Debug = 0;
	ConverterNum = 0;
}

SnowSetting::~SnowSetting()
{
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

bool SnowSetting::checkProcessor(FILE* fp) {
	HANDLE hRead, hWrite;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	LPWSTR param;
	const static size_t bufferSize = 1024;


	if (!CONVERTER_CPP.getAvailable()){
		
		return false;	// cannot read processor list
	}

	CreatePipe(&hRead, &hWrite, NULL, bufferSize);
	SetHandleInformation(hWrite, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

	memset(&si, 0, sizeof(STARTUPINFO));

	si.cb = sizeof(si);
	si.hStdOutput = hWrite;
	si.hStdError = hWrite;
	si.dwFlags = STARTF_USESTDHANDLES;

	param = new WCHAR[MAX_PATH];

	lstrcpy(param, CONVERTER_CPP.getExePath().c_str());
	lstrcat(param, L" --list-processor");

	BOOL isExecuted = CreateProcess(NULL, param, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, NULL, CurrPath.c_str(), &si, &pi);

	CloseHandle(hWrite);

	if (isExecuted)
	{
		DWORD len;
		char s[bufferSize] = "";
		string st;
		wstring ws;

		fwprintf(fp, L"\n[proc]\n");

		while (ReadFile(hRead, s, bufferSize - 1, &len, 0) != 0 || len < 0) {
			st = s;
			ws.assign(st.begin(), st.end());
			fwprintf(fp, ws.c_str());

		}
		fwprintf(fp, L"\n\n");
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}

	CloseHandle(hRead);

	delete[] param;

	return true;
}

bool SnowSetting::checkCuda() {
	static int result = 0;
	
	if (result != 0)
		return result == 1;

	HMODULE cu = LoadLibraryW(L"nvcuda.dll");
	if (!cu) {
		result = -1;
		return false;
	}
	FreeLibrary(cu);
	result = 1;
	return true;
}
bool SnowSetting::checkVulkan() {
	static int result = 0;

	if (result != 0)
		return result == 1;

	HMODULE vk = LoadLibraryA("vulkan-1.dll");
	if (!vk) {
		result = -1;
		return false;
	}
	FreeLibrary(vk);
	result = 1;
	return true;
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
		else if (LangFileName.find(L"German") != std::string::npos) hSrc = FindResource(g_hInst, MAKEINTRESOURCE(IDR_LANG_DE), L"LANG");
		else if (LangFileName.find(L"Swedish") != std::string::npos) hSrc = FindResource(g_hInst, MAKEINTRESOURCE(IDR_LANG_SV), L"LANG");
		else if (LangFileName.find(L"Portuguese") != std::string::npos) hSrc = FindResource(g_hInst, MAKEINTRESOURCE(IDR_LANG_PT), L"LANG");
		else if (LangFileName.find(L"Ukrainian") != std::string::npos) hSrc = FindResource(g_hInst, MAKEINTRESOURCE(IDR_LANG_UKR), L"LANG");
		else if (LangFileName.find(L"Swedish") != std::string::npos) hSrc = FindResource(g_hInst, MAKEINTRESOURCE(IDR_LANG_SW), L"LANG");
		else if (LangFileName.find(L"Dutch") != std::string::npos) hSrc = FindResource(g_hInst, MAKEINTRESOURCE(IDR_LANG_NL), L"LANG");
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

	Section = L"Snowshell";

	Key = L"INT_SETTING_VER";
	INT_SETTING_VER = GetPrivateProfileIntW(Section.c_str(), Key.c_str(), 0, LangFileName.c_str());

	if (INT_SETTING_VER < SETTING_VER_MINIMUM) {
		for (wstring iLangFileName : LangFile) {
			DeleteFile((LangPath + L"\\" + iLangFileName).c_str());
		}
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

	Key = L"STRING_MENU_NOISE_MID";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Medium", buf, 200, LangFileName.c_str());
	STRING_MENU_NOISE_MID = buf;

	Key = L"STRING_MENU_NOISE_HIGH";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"High", buf, 200, LangFileName.c_str());
	STRING_MENU_NOISE_HIGH = buf;

	Key = L"STRING_MENU_NOISE_VERY_HIGH";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Max", buf, 200, LangFileName.c_str());
	STRING_MENU_NOISE_VERY_HIGH = buf;


	Section = L"Scale";

	Key = L"STRING_MENU_SCALE_CUSTOM";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Custom", buf, 200, LangFileName.c_str());
	STRING_MENU_SCALE_CUSTOM = buf;


	Section = L"GPU";

	Key = L"STRING_MENU_GPU_CPU";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Force CPU Only", buf, 200, LangFileName.c_str());
	STRING_MENU_GPU_CPU = buf;

	Key = L"STRING_MENU_GPU_GPU";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Prefer GPU", buf, 200, LangFileName.c_str());
	STRING_MENU_GPU_GPU = buf;


	Section = L"TTA";

	Key = L"STRING_MENU_TTA_DISABLED";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Disabled", buf, 200, LangFileName.c_str());
	STRING_MENU_TTA_DISABLED = buf;

	Key = L"STRING_MENU_TTA_ENABLED";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Enabled", buf, 200, LangFileName.c_str());
	STRING_MENU_TTA_ENABLED = buf;


	Section = L"Export";

	Key = L"STRING_MENU_EXPORT_SAME";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Same Folder of Input File", buf, 200, LangFileName.c_str());
	STRING_MENU_EXPORT_SAME = buf;

	Key = L"STRING_MENU_EXPORT_NEW";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"output Folder", buf, 200, LangFileName.c_str());
	STRING_MENU_EXPORT_NEW = buf;


	Section = L"Confirm";

	Key = L"STRING_MENU_CONFIRM_SHOW";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Show Warning", buf, 200, LangFileName.c_str());
	STRING_MENU_CONFIRM_SHOW = buf;

	Key = L"STRING_MENU_CONFIRM_SKIP";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Hide Warning", buf, 200, LangFileName.c_str());
	STRING_MENU_CONFIRM_SKIP = buf;

	Key = L"STRING_MENU_CONFIRM_DEBUG";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Show Converting Log", buf, 200, LangFileName.c_str());
	STRING_MENU_CONFIRM_DEBUG = buf;


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

	Key = L"STRING_TEXT_NOISE_MID";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Medium", buf, 200, LangFileName.c_str());
	STRING_TEXT_NOISE_MID = buf;

	Key = L"STRING_TEXT_NOISE_HIGH";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"High", buf, 200, LangFileName.c_str());
	STRING_TEXT_NOISE_HIGH = buf;

	Key = L"STRING_TEXT_NOISE_VERY_HIGH";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Max", buf, 200, LangFileName.c_str());
	STRING_TEXT_NOISE_VERY_HIGH = buf;


	Key = L"STRING_TEXT_SCALE";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Zoom Scale", buf, 200, LangFileName.c_str());
	STRING_TEXT_SCALE = buf;


	Key = L"STRING_TEXT_GPU";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Converter", buf, 200, LangFileName.c_str());
	STRING_TEXT_GPU = buf;

	Key = L"STRING_TEXT_EXPORT";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Export Dir", buf, 200, LangFileName.c_str());
	STRING_TEXT_EXPORT = buf;

	Key = L"STRING_TEXT_EXPORT_SAME";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Same Folder", buf, 200, LangFileName.c_str());
	STRING_TEXT_EXPORT_SAME = buf;

	Key = L"STRING_TEXT_EXPORT_NEW";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"output Folder", buf, 200, LangFileName.c_str());
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

	Key = L"STRING_TEXT_CONFIRM_CUSTOM_SCALE_TITLE";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Warning", buf, 200, LangFileName.c_str());
	STRING_TEXT_CONFIRM_CUSTOM_SCALE_TITLE = buf;

	Key = L"STRING_TEXT_CONFIRM_CUSTOM_SCALE_MESSAGE";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Program will crashes if your memory space is not enough.\nDo you want to continue?", buf, 200, LangFileName.c_str());
	STRING_TEXT_CONFIRM_CUSTOM_SCALE_MESSAGE = buf;
	nl = STRING_TEXT_CONFIRM_CUSTOM_SCALE_MESSAGE.find(L"\\n");
	while (nl != wstring::npos) {
		STRING_TEXT_CONFIRM_CUSTOM_SCALE_MESSAGE.replace(nl, 2, L"\n");
		nl = STRING_TEXT_CONFIRM_CUSTOM_SCALE_MESSAGE.find(L"\\n");
	}

	Key = L"STRING_TEXT_TOO_LONG_PATH_TITLE";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Error", buf, 200, LangFileName.c_str());
	STRING_TEXT_TOO_LONG_PATH_TITLE = buf;

	Key = L"STRING_TEXT_TOO_LONG_PATH_MESSAGE";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"File path is too long.", buf, 200, LangFileName.c_str());
	STRING_TEXT_TOO_LONG_PATH_MESSAGE = buf;

	Key = L"STRING_TEXT_TOO_LONG_PATH_MESSAGE_COUNT";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"", buf, 200, LangFileName.c_str());
	STRING_TEXT_TOO_LONG_PATH_MESSAGE_COUNT = buf;

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

	Key = L"STRING_TEXT_ABORT_CONVERT_TITLE";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Confirm", buf, 200, LangFileName.c_str());
	STRING_TEXT_ABORT_CONVERT_TITLE = buf;

	Key = L"STRING_TEXT_ABORT_CONVERT_MESSAGE";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"Do you want to stop the converting process?", buf, 200, LangFileName.c_str());
	STRING_TEXT_ABORT_CONVERT_MESSAGE = buf;

}

bool SnowSetting::loadSetting()
{
	if (Singletone == nullptr)
		Init();

	LPWSTR buf = new WCHAR[MAX_PATH];
	wstring Section = L"Snowshell";
	wstring Key, Value;

	Key = L"ConverterNum";
	setConverterNum(GetPrivateProfileInt(Section.c_str(), Key.c_str(), -1, INIPath.c_str()));

	Key = L"Noise";
	setNoise(GetPrivateProfileInt(Section.c_str(), Key.c_str(), NOISE_MID, INIPath.c_str()));

	Key = L"Scale";
	setScale(GetPrivateProfileInt(Section.c_str(), Key.c_str(), SCALE_x1_6, INIPath.c_str()));

	Key = L"GPU";
	setGPU(GetPrivateProfileInt(Section.c_str(), Key.c_str(), GPU_GPU_MODE, INIPath.c_str()));

	Key = L"TTA";
	setTTA(GetPrivateProfileInt(Section.c_str(), Key.c_str(), TTA_DISABLED, INIPath.c_str()));

	Key = L"Export";
	setExport(GetPrivateProfileInt(Section.c_str(), Key.c_str(), EXPORT_SAME, INIPath.c_str()));

	Key = L"Extension";
	GetPrivateProfileStringW(Section.c_str(), Key.c_str(), L"png", buf, MAX_PATH, INIPath.c_str());
	setOutputExt(buf);

	Key = L"Confirm";
	setConfirm(GetPrivateProfileInt(Section.c_str(), Key.c_str(), CONFIRM_SHOW, INIPath.c_str()));

	Key = L"Lang";
	int langsel = GetPrivateProfileInt(Section.c_str(), Key.c_str(), -1, INIPath.c_str());
	if (langsel == -1) {
		WCHAR lbuf[40];
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SENGLANGUAGE, lbuf, 40);
		for (int i = 0; i < LangNum; i++)
			if (LangFile[i].find(lbuf) >= 0)
			{
				langsel = i;
				break;
			}
		if (langsel == -1)
			langsel = 1;
	}
	setLang(langsel);

	Key = L"Debug";
	setDebug(GetPrivateProfileInt(Section.c_str(), Key.c_str(), 0, INIPath.c_str()));

	loadLocale();

	GetPrivateProfileStringW(Section.c_str(), L"ScaleRatio", L"1.6", buf, MAX_PATH, INIPath.c_str());
	SnowSetting::setScaleRatio(buf);
	if (SnowSetting::getScaleRatio() == L"")
		SnowSetting::setScaleRatio(L"1.6");

	GetPrivateProfileStringW(Section.c_str(), L"OutputDirName", L"output", buf, MAX_PATH, INIPath.c_str());
	OutputDirName = buf;


	Section = L"Converter";

	GetPrivateProfileStringW(Section.c_str(), L"waifu2x-caffe", CONVERTER_CAFFE.getExePath().c_str(), buf, MAX_PATH, INIPath.c_str());
	CONVERTER_CAFFE.setExePath(buf);
	CONVERTER_CAFFE.checkAvailable();

	GetPrivateProfileStringW(Section.c_str(), L"waifu2x-converter-cpp", CONVERTER_CPP.getExePath().c_str(), buf, MAX_PATH, INIPath.c_str());
	CONVERTER_CPP.setExePath(buf);
	CONVERTER_CPP.checkAvailable();

	GetPrivateProfileStringW(Section.c_str(), L"waifu2x-ncnn-vulkan", CONVERTER_VULKAN.getExePath().c_str(), buf, MAX_PATH, INIPath.c_str());
	CONVERTER_VULKAN.setExePath(buf);
	CONVERTER_VULKAN.checkAvailable();

	GetPrivateProfileStringW(Section.c_str(), L"realcugan-vulkan", CONVERTER_CUGAN.getExePath().c_str(), buf, MAX_PATH, INIPath.c_str());
	CONVERTER_CUGAN.setExePath(buf);
	CONVERTER_CUGAN.checkAvailable();

	GetPrivateProfileStringW(Section.c_str(), L"realesrgan-vulkan", CONVERTER_ESRGAN.getExePath().c_str(), buf, MAX_PATH, INIPath.c_str());
	CONVERTER_ESRGAN.setExePath(buf);
	CONVERTER_ESRGAN.checkAvailable();


	Section = L"Model";

	GetPrivateProfileStringW(Section.c_str(), L"waifu2x-caffe", L"", buf, MAX_PATH, INIPath.c_str());
	CONVERTER_CAFFE.setModelDir(buf);

	GetPrivateProfileStringW(Section.c_str(), L"waifu2x-converter-cpp", L"", buf, MAX_PATH, INIPath.c_str());
	CONVERTER_CPP.setModelDir(buf);

	GetPrivateProfileStringW(Section.c_str(), L"waifu2x-ncnn-vulkan", L"", buf, MAX_PATH, INIPath.c_str());
	CONVERTER_VULKAN.setModelDir(buf);

	GetPrivateProfileStringW(Section.c_str(), L"realcugan-vulkan", L"", buf, MAX_PATH, INIPath.c_str());
	CONVERTER_CUGAN.setModelDir(buf);

	GetPrivateProfileStringW(Section.c_str(), L"realesrgan-vulkan", L"", buf, MAX_PATH, INIPath.c_str());
	CONVERTER_ESRGAN.setModelDir(buf);

	Section = L"CustomOption";

	GetPrivateProfileStringW(Section.c_str(), L"waifu2x-caffe", L"", buf, MAX_PATH, INIPath.c_str());
	CONVERTER_CAFFE.setOptionString(buf);

	GetPrivateProfileStringW(Section.c_str(), L"waifu2x-converter-cpp", L"", buf, MAX_PATH, INIPath.c_str());
	CONVERTER_CPP.setOptionString(buf);

	GetPrivateProfileStringW(Section.c_str(), L"waifu2x-ncnn-vulkan", L"", buf, MAX_PATH, INIPath.c_str());
	CONVERTER_VULKAN.setOptionString(buf);

	GetPrivateProfileStringW(Section.c_str(), L"realcugan-vulkan", L"", buf, MAX_PATH, INIPath.c_str());
	CONVERTER_CUGAN.setOptionString(buf);

	GetPrivateProfileStringW(Section.c_str(), L"realesrgan-vulkan", L"-n realesrgan-x4plus-anime", buf, MAX_PATH, INIPath.c_str());
	CONVERTER_ESRGAN.setOptionString(buf);

	// Set Converter
	if (CurrentConverter == nullptr) {
		if (checkVulkan() && CONVERTER_VULKAN.getAvailable()) {
			setConverterNum(CONVERTER_NUM_VULKAN);
			setScale(getScale());
		}
		else if (checkVulkan() && CONVERTER_CUGAN.getAvailable()) {
			setConverterNum(CONVERTER_NUM_CUGAN);
			setScale(getScale());
		}
		else if (checkCuda() && CONVERTER_CAFFE.getAvailable()) {
			setConverterNum(CONVERTER_NUM_CAFFE);
		}
		else if (checkVulkan() && CONVERTER_ESRGAN.getAvailable()) {
			setConverterNum(CONVERTER_NUM_ESRGAN);
			setScale(SCALE_CUSTOM);
		}
		else if (CONVERTER_CPP.getAvailable()) {
			setConverterNum(CONVERTER_NUM_CPP);
		}
	}

	delete[] buf;

	return true;
}

bool SnowSetting::saveSetting()
{
	if (Singletone == nullptr)
		Init();

	wstring Section = L"Snowshell";
	wstring Key, Value;

	Key = L"ConverterNum";
	WritePrivateProfileString(Section.c_str(), Key.c_str(), itos(getConverterNum()).c_str(), INIPath.c_str());

	Key = L"Noise";
	WritePrivateProfileString(Section.c_str(), Key.c_str(), itos(getNoise()).c_str(), INIPath.c_str());

	Key = L"Scale";
	WritePrivateProfileString(Section.c_str(), Key.c_str(), itos(getScale()).c_str(), INIPath.c_str());

	Key = L"GPU";
	WritePrivateProfileString(Section.c_str(), Key.c_str(), itos(getGPU()).c_str(), INIPath.c_str());

	Key = L"TTA";
	WritePrivateProfileString(Section.c_str(), Key.c_str(), itos(getTTA()).c_str(), INIPath.c_str());

	Key = L"Export";
	WritePrivateProfileString(Section.c_str(), Key.c_str(), itos(getExport()).c_str(), INIPath.c_str());

	Key = L"Extension";
	WritePrivateProfileString(Section.c_str(), Key.c_str(), getOutputExt().c_str(), INIPath.c_str());

	Key = L"Confirm";
	WritePrivateProfileString(Section.c_str(), Key.c_str(), itos(getConfirm()).c_str(), INIPath.c_str());

	Key = L"Lang";
	WritePrivateProfileString(Section.c_str(), Key.c_str(), itos(getLang()).c_str(), INIPath.c_str());

	Key = L"Debug";
	WritePrivateProfileString(Section.c_str(), Key.c_str(), itos(getDebug()).c_str(), INIPath.c_str());

	Key = L"ScaleRatio";
	WritePrivateProfileString(Section.c_str(), Key.c_str(), SnowSetting::getScaleRatio().c_str(), INIPath.c_str());

	Key = L"OutputDirName";
	WritePrivateProfileString(Section.c_str(), Key.c_str(), OutputDirName.c_str(), INIPath.c_str());


	Section = L"Converter";

	WritePrivateProfileString(Section.c_str(), L"waifu2x-caffe", CONVERTER_CAFFE.getExePath().c_str(), INIPath.c_str());

	WritePrivateProfileString(Section.c_str(), L"waifu2x-converter-cpp", CONVERTER_CPP.getExePath().c_str(), INIPath.c_str());

	WritePrivateProfileString(Section.c_str(), L"waifu2x-ncnn-vulkan", CONVERTER_VULKAN.getExePath().c_str(), INIPath.c_str());

	WritePrivateProfileString(Section.c_str(), L"realcugan-vulkan", CONVERTER_CUGAN.getExePath().c_str(), INIPath.c_str());

	WritePrivateProfileString(Section.c_str(), L"realesrgan-vulkan", CONVERTER_ESRGAN.getExePath().c_str(), INIPath.c_str());


	Section = L"Model";

	WritePrivateProfileString(Section.c_str(), L"waifu2x-caffe", CONVERTER_CAFFE.getModelDir().c_str(), INIPath.c_str());

	WritePrivateProfileString(Section.c_str(), L"waifu2x-converter-cpp", CONVERTER_CPP.getModelDir().c_str(), INIPath.c_str());

	WritePrivateProfileString(Section.c_str(), L"waifu2x-ncnn-vulkan", CONVERTER_VULKAN.getModelDir().c_str(), INIPath.c_str());

	WritePrivateProfileString(Section.c_str(), L"realcugan-vulkan", CONVERTER_CUGAN.getModelDir().c_str(), INIPath.c_str());

	WritePrivateProfileString(Section.c_str(), L"realesrgan-vulkan", CONVERTER_ESRGAN.getModelDir().c_str(), INIPath.c_str());


	Section = L"CustomOption";

	WritePrivateProfileString(Section.c_str(), L"waifu2x-caffe", CONVERTER_CAFFE.getOptionString().c_str(), INIPath.c_str());

	WritePrivateProfileString(Section.c_str(), L"waifu2x-converter-cpp", CONVERTER_CPP.getOptionString().c_str(), INIPath.c_str());

	WritePrivateProfileString(Section.c_str(), L"waifu2x-ncnn-vulkan", CONVERTER_VULKAN.getOptionString().c_str(), INIPath.c_str());

	WritePrivateProfileString(Section.c_str(), L"realcugan-vulkan", CONVERTER_CUGAN.getOptionString().c_str(), INIPath.c_str());

	WritePrivateProfileString(Section.c_str(), L"realesrgan-vulkan", CONVERTER_ESRGAN.getOptionString().c_str(), INIPath.c_str());

	return true;
}

void SnowSetting::loadMenuString(HMENU hMenu)
{
	ModifyMenu(hMenu, MENU_FILE, MF_BYPOSITION | MF_STRING, MENU_FILE, STRING_MENU_FILE.c_str());
	ModifyMenu(hMenu, MENU_NOISE, MF_BYPOSITION | MF_STRING, MENU_NOISE, STRING_MENU_NOISE.c_str());
	ModifyMenu(hMenu, MENU_SCALE, MF_BYPOSITION | MF_STRING, MENU_SCALE, STRING_MENU_SCALE.c_str());
	ModifyMenu(hMenu, MENU_EXPORT, MF_BYPOSITION | MF_STRING, MENU_EXPORT, STRING_MENU_EXPORT.c_str());
	ModifyMenu(hMenu, MENU_CONFIRM, MF_BYPOSITION | MF_STRING, MENU_CONFIRM, STRING_MENU_CONFIRM.c_str());

	ModifyMenu(hMenu, ID_MENU_FILE_IMGSEL, MF_BYCOMMAND | MF_STRING, ID_MENU_FILE_IMGSEL, STRING_MENU_FILE_IMAGE_SEL.c_str());
	ModifyMenu(hMenu, ID_MENU_FILE_CREDIT, MF_BYCOMMAND | MF_STRING, ID_MENU_FILE_CREDIT, STRING_MENU_FILE_CREDIT.c_str());
	ModifyMenu(hMenu, ID_MENU_FILE_QUIT, MF_BYCOMMAND | MF_STRING, ID_MENU_FILE_QUIT, STRING_MENU_FILE_QUIT.c_str());

	ModifyMenu(hMenu, ID_MENU_NOISE_NONE, MF_BYCOMMAND | MF_STRING, ID_MENU_NOISE_NONE, STRING_MENU_NOISE_NONE.c_str());
	ModifyMenu(hMenu, ID_MENU_NOISE_LOW, MF_BYCOMMAND | MF_STRING, ID_MENU_NOISE_LOW, STRING_MENU_NOISE_LOW.c_str());
	ModifyMenu(hMenu, ID_MENU_NOISE_MID, MF_BYCOMMAND | MF_STRING, ID_MENU_NOISE_MID, STRING_MENU_NOISE_MID.c_str());
	ModifyMenu(hMenu, ID_MENU_NOISE_HIGH, MF_BYCOMMAND | MF_STRING, ID_MENU_NOISE_HIGH, STRING_MENU_NOISE_HIGH.c_str());
	ModifyMenu(hMenu, ID_MENU_NOISE_VERY_HIGH, MF_BYCOMMAND | MF_STRING, ID_MENU_NOISE_VERY_HIGH, STRING_MENU_NOISE_VERY_HIGH.c_str());

	ModifyMenu(hMenu, ID_MENU_SCALE_CUSTOM, MF_BYCOMMAND | MF_STRING, ID_MENU_SCALE_CUSTOM, STRING_MENU_SCALE_CUSTOM.c_str());

	ModifyMenu(hMenu, ID_MENU_GPU_CPU, MF_BYCOMMAND | MF_STRING, ID_MENU_GPU_CPU, STRING_MENU_GPU_CPU.c_str());
	ModifyMenu(hMenu, ID_MENU_GPU_GPU, MF_BYCOMMAND | MF_STRING, ID_MENU_GPU_GPU, STRING_MENU_GPU_GPU.c_str());

	ModifyMenu(hMenu, ID_MENU_TTA_DISABLED, MF_BYCOMMAND | MF_STRING, ID_MENU_TTA_DISABLED, STRING_MENU_TTA_DISABLED.c_str());
	ModifyMenu(hMenu, ID_MENU_TTA_ENABLED, MF_BYCOMMAND | MF_STRING, ID_MENU_TTA_ENABLED, STRING_MENU_TTA_ENABLED.c_str());

	ModifyMenu(hMenu, ID_MENU_EXPORT_SAME, MF_BYCOMMAND | MF_STRING, ID_MENU_EXPORT_SAME, STRING_MENU_EXPORT_SAME.c_str());
	ModifyMenu(hMenu, ID_MENU_EXPORT_NEW, MF_BYCOMMAND | MF_STRING, ID_MENU_EXPORT_NEW, STRING_MENU_EXPORT_NEW.c_str());

	ModifyMenu(hMenu, ID_MENU_CONFIRM_SHOW, MF_BYCOMMAND | MF_STRING, ID_MENU_CONFIRM_SHOW, STRING_MENU_CONFIRM_SHOW.c_str());
	ModifyMenu(hMenu, ID_MENU_CONFIRM_HIDE, MF_BYCOMMAND | MF_STRING, ID_MENU_CONFIRM_HIDE, STRING_MENU_CONFIRM_SKIP.c_str());
	ModifyMenu(hMenu, ID_MENU_CONFIRM_DEBUG, MF_BYCOMMAND | MF_STRING, ID_MENU_CONFIRM_DEBUG, STRING_MENU_CONFIRM_DEBUG.c_str());
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

int SnowSetting::getGPU()
{
	if (Singletone == nullptr)
		Init();

	return Singletone->GPU;
}

int SnowSetting::getTTA()
{
	if (Singletone == nullptr)
		Init();

	return Singletone->TTA;
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

BOOL SnowSetting::getDebug()
{
	if (Singletone == nullptr)
		Init();

	return Singletone->Debug;
}

int SnowSetting::getConverterNum()
{
	if (Singletone == nullptr)
		Init();

	return Singletone->ConverterNum;
}

wstring SnowSetting::getLangName()
{
	return LangFile[getLang()];
}

std::wstring SnowSetting::getScaleRatio() {
	if (Singletone == nullptr)
		Init();

	return Singletone->ScaleRatio;
}

std::wstring SnowSetting::getOutputExt(){
	if (Singletone == nullptr)
		Init();

	return Singletone->OutputExt;
}

void SnowSetting::setNoise(int Noise)
{
	if (Singletone == nullptr)
		Init();

	if (Noise > NOISE_MAX || Noise < NOISE_NONE)
		Noise = NOISE_NONE;

	Singletone->Noise = Noise;
}

void SnowSetting::setScale(int Scale)
{
	if (Singletone == nullptr)
		Init();

	if (Scale > SCALE_MAX || Scale < 0)
		Scale = SCALE_x2_0;
	else if (CurrentConverter == &CONVERTER_ESRGAN) {
		Scale = SCALE_CUSTOM;	
	}
	else if (CurrentConverter == &CONVERTER_VULKAN && Scale != SCALE_x1_0 && Scale != SCALE_x2_0 && Scale != SCALE_CUSTOM)
		Scale = SCALE_x2_0;
	else if (CurrentConverter == &CONVERTER_CUGAN&& Scale != SCALE_x2_0 && Scale != SCALE_CUSTOM)
		Scale = SCALE_x2_0;

	Singletone->Scale = Scale;
}

void SnowSetting::setGPU(int GPU)
{
	if (Singletone == nullptr)
		Init();

	if (GPU > GPU_MAX || GPU < 0)
		GPU = 0;

	Singletone->GPU = GPU;
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

void SnowSetting::setDebug(BOOL Debug)
{
	if (Singletone == nullptr)
		Init();

	Singletone->Debug = Debug;
}

void SnowSetting::setConverterNum(int ConverterNum)
{
	if (Singletone == nullptr)
		Init();
	
	switch (ConverterNum) {
	case CONVERTER_NUM_CPP:
		CONVERTER_CPP.checkAvailable();
		if (CONVERTER_CPP.getAvailable()) {
			Singletone->CurrentConverter = &CONVERTER_CPP;
			Singletone->ConverterNum = ConverterNum;
		}
		break;
	case CONVERTER_NUM_CAFFE:
		CONVERTER_CAFFE.checkAvailable();
		if (CONVERTER_CAFFE.getAvailable()) {
			Singletone->CurrentConverter = &CONVERTER_CAFFE;
			Singletone->ConverterNum = ConverterNum;
		}
		break;
	case CONVERTER_NUM_VULKAN:
		CONVERTER_VULKAN.checkAvailable();
		if (CONVERTER_VULKAN.getAvailable()) {
			Singletone->CurrentConverter = &CONVERTER_VULKAN;
			Singletone->ConverterNum = ConverterNum;
			for (int i = 0; i < VulkanScaleNum; i++) {
				if (!wcscmp(VulkanScale[i], Singletone->ScaleRatio.c_str()))
					return;
			}
			Singletone->ScaleRatio = L"4.0";
		}
		break;
	case CONVERTER_NUM_CUGAN:
		CONVERTER_CUGAN.checkAvailable();
		if (CONVERTER_CUGAN.getAvailable()) {
			Singletone->CurrentConverter = &CONVERTER_CUGAN;
			Singletone->ConverterNum = ConverterNum;
			for (int i = 0; i < CuganScaleNum; i++) {
				if (!wcscmp(VulkanScale[i], Singletone->ScaleRatio.c_str()))
					return;
			}
			Singletone->ScaleRatio = L"4.0";
		}
		break;
	case CONVERTER_NUM_ESRGAN:
		CONVERTER_ESRGAN.checkAvailable();
		if (CONVERTER_ESRGAN.getAvailable()) {
			Singletone->CurrentConverter = &CONVERTER_ESRGAN;
			Singletone->ConverterNum = ConverterNum;
			Singletone->ScaleRatio = L"4.0";
		}
		break;
	}
}

void SnowSetting::setScaleRatio(std::wstring scaleRatio) {
	if (Singletone == nullptr)
		Init();

	Singletone->ScaleRatio = scaleRatio;
}

void SnowSetting::setOutputExt(std::wstring outputExt) {
	if (Singletone == nullptr)
		Init();

	Singletone->OutputExt = outputExt;
}

void SnowSetting::setTTA(int tta)
{
	if (Singletone == nullptr)
		Init();

	Singletone->TTA = tta;
}

void SnowSetting::checkMenuAll(HMENU hMenu)
{
	checkNoise(hMenu);
	checkScale(hMenu);
	checkGPU(hMenu);
	checkTTA(hMenu);
	checkExport(hMenu);
	checkConfirm(hMenu);
	checkLang(hMenu);
	checkDebug(hMenu);
	checkConverterNum(hMenu);
}

void SnowSetting::checkNoise(HMENU hMenu, int sel)
{
	HMENU hSubMenu = GetSubMenu(hMenu, MENU_NOISE);

	if (sel != -1)
		setNoise(sel);

	if (CurrentConverter == &CONVERTER_ESRGAN) {
		EnableMenuItem(hMenu, MENU_NOISE, MF_BYPOSITION | MF_GRAYED);
		return;
	}
	else if (CurrentConverter == &CONVERTER_CUGAN && getScale() == SCALE_CUSTOM) {
		EnableMenuItem(hSubMenu, NOISE_MID, MF_BYPOSITION | MF_GRAYED);
		EnableMenuItem(hSubMenu, NOISE_HIGH, MF_BYPOSITION | MF_GRAYED);

		if (getNoise() == NOISE_MID || getNoise() == NOISE_HIGH)
			setNoise(NOISE_LOW);
	}
	else {
		EnableMenuItem(hMenu, MENU_NOISE, MF_BYPOSITION | MF_ENABLED);
		EnableMenuItem(hSubMenu, NOISE_MID, MF_BYPOSITION | MF_ENABLED);
		EnableMenuItem(hSubMenu, NOISE_HIGH, MF_BYPOSITION | MF_ENABLED);
	}

	for (int i = 0; i <= NOISE_MAX; i++)
		CheckMenuItem(hSubMenu, i, MF_BYPOSITION | MF_UNCHECKED);
	CheckMenuItem(hSubMenu, getNoise(), MF_BYPOSITION | MF_CHECKED);
}

void SnowSetting::checkScale(HMENU hMenu, int sel)
{
	HMENU hSubMenu = GetSubMenu(hMenu, MENU_SCALE);

	if (sel != -1)
		setScale(sel);

	if (CurrentConverter == &CONVERTER_ESRGAN) {
		EnableMenuItem(hMenu, MENU_SCALE, MF_BYPOSITION | MF_GRAYED);
		setScale(SCALE_CUSTOM);
		return;
	}
	else {
		EnableMenuItem(hMenu, MENU_SCALE, MF_BYPOSITION | MF_ENABLED);
	}

	for (int i = 0; i <= SCALE_MAX; i++)
		CheckMenuItem(hSubMenu, i, MF_BYPOSITION | MF_UNCHECKED);

	if (CurrentConverter == &CONVERTER_VULKAN) {
		EnableMenuItem(hSubMenu, SCALE_x1_5, MF_BYPOSITION | MF_GRAYED);
		EnableMenuItem(hSubMenu, SCALE_x1_6, MF_BYPOSITION | MF_GRAYED);

		if (getScale() == SCALE_x1_5 || getScale() == SCALE_x1_6)
			setScale(SCALE_x2_0);
	}
	else if (CurrentConverter == &CONVERTER_CUGAN) {
		EnableMenuItem(hSubMenu, SCALE_x1_0, MF_BYPOSITION | MF_GRAYED);
		EnableMenuItem(hSubMenu, SCALE_x1_5, MF_BYPOSITION | MF_GRAYED);
		EnableMenuItem(hSubMenu, SCALE_x1_6, MF_BYPOSITION | MF_GRAYED);

		if (getScale() != SCALE_x2_0 && getScale() != SCALE_CUSTOM)
			setScale(SCALE_x2_0);
	}
	else {
		EnableMenuItem(hSubMenu, SCALE_x1_0, MF_BYPOSITION | MF_ENABLED);
		EnableMenuItem(hSubMenu, SCALE_x1_5, MF_BYPOSITION | MF_ENABLED);
		EnableMenuItem(hSubMenu, SCALE_x1_6, MF_BYPOSITION | MF_ENABLED);
	}

	CheckMenuItem(hSubMenu, getScale(), MF_BYPOSITION | MF_CHECKED);
}

void SnowSetting::checkGPU(HMENU hMenu, int sel)
{
	HMENU hSubMenu = GetSubMenu(hMenu, MENU_GPU);

	if (sel != -1)
		setGPU(sel);

	if (CurrentConverter == &CONVERTER_VULKAN || CurrentConverter == &CONVERTER_CUGAN || CurrentConverter == &CONVERTER_ESRGAN) {
		EnableMenuItem(hMenu, MENU_GPU, MF_BYPOSITION | MF_GRAYED);
		return;
	}
	else {
		EnableMenuItem(hMenu, MENU_GPU, MF_BYPOSITION | MF_ENABLED);
	}

	for (int i = 0; i <= GPU_MAX; i++)
		CheckMenuItem(hSubMenu, i, MF_BYPOSITION | MF_UNCHECKED);
	CheckMenuItem(hSubMenu, getGPU(), MF_BYPOSITION | MF_CHECKED);
}

void SnowSetting::checkTTA(HMENU hMenu, int sel)
{
	HMENU hSubMenu = GetSubMenu(hMenu, MENU_TTA);

	if (sel != -1)
		setTTA(sel);

	for (int i = 0; i <= TTA_MAX; i++)
		CheckMenuItem(hSubMenu, i, MF_BYPOSITION | MF_UNCHECKED);
	CheckMenuItem(hSubMenu, getTTA(), MF_BYPOSITION | MF_CHECKED);
}

void SnowSetting::checkExport(HMENU hMenu, int sel)
{
	HMENU hSubMenu = GetSubMenu(hMenu, MENU_EXPORT);

	if (sel != -1)
		setExport(sel);

	for (int i = 0; i <= EXPORT_MAX; i++)
		CheckMenuItem(hSubMenu, i, MF_BYPOSITION | MF_UNCHECKED);
	CheckMenuItem(hSubMenu, getExport(), MF_BYPOSITION | MF_CHECKED);
}

void SnowSetting::checkConfirm(HMENU hMenu, int sel)
{
	HMENU hSubMenu = GetSubMenu(hMenu, MENU_CONFIRM);

	if (sel != -1)
		setConfirm(sel);

	for (int i = 0; i <= CONFIRM_MAX; i++)
		CheckMenuItem(hSubMenu, i, MF_BYPOSITION | MF_UNCHECKED);
	CheckMenuItem(hSubMenu, getConfirm(), MF_BYPOSITION | MF_CHECKED);
}

void SnowSetting::checkLang(HMENU hMenu, int sel)
{
	HMENU hSubMenu = GetSubMenu(hMenu, MENU_LANG);

	if (sel != -1)
		setLang(sel);

	for (int i = 0; i < LangNum; i++)
		CheckMenuItem(hSubMenu, i, MF_BYPOSITION | MF_UNCHECKED);
	CheckMenuItem(hSubMenu, getLang(), MF_BYPOSITION | MF_CHECKED);
}

void SnowSetting::checkDebug(HMENU hMenu, int sel)
{
	HMENU hSubMenu = GetSubMenu(hMenu, MENU_CONFIRM);

	if (sel != -1)
		setDebug(!getDebug());

	if(getDebug())
		CheckMenuItem(hSubMenu, 3, MF_BYPOSITION | MF_CHECKED);
	else
		CheckMenuItem(hSubMenu, 3, MF_BYPOSITION | MF_UNCHECKED);
}

void SnowSetting::checkConverterNum(HMENU hMenu, int sel)
{
	HMENU hSubMenu = GetSubMenu(hMenu, MENU_CONVERTER);

	if (sel != -1)
		setConverterNum(sel);

	for (int i = 0; i <= CONVERTER_NUM_MAX; i++)
		CheckMenuItem(hSubMenu, i, MF_BYPOSITION | MF_UNCHECKED);
	CheckMenuItem(hSubMenu, getConverterNum(), MF_BYPOSITION | MF_CHECKED);
}

void SnowSetting::getTexts(wstring* (*UITitleText)[5], wstring* (*UIText)[5]) {
	if (CurrentConverter == &CONVERTER_ESRGAN) {
		(*UITitleText)[0] = &STRING_EMPTY;
		(*UIText)[0] = &STRING_EMPTY;
	}
	else {
		(*UITitleText)[0] = &STRING_TEXT_NOISE;
		(*UIText)[0] = getNoiseText();
	}

	(*UITitleText)[1] = &STRING_TEXT_SCALE;
	(*UIText)[1] = getScaleText();

	(*UITitleText)[2] = &STRING_TEXT_GPU;
	(*UIText)[2] = getGPUText();

	(*UITitleText)[3] = &STRING_TEXT_EXPORT;
	(*UIText)[3] = getExportText();

	(*UITitleText)[4] = &STRING_TEXT_CONFIRM;
	(*UIText)[4] = getConfirmText();

}

wstring * SnowSetting::getNoiseText()
{
	switch (getNoise()) {
	case NOISE_NONE:
		return &STRING_TEXT_NOISE_NONE;
	case NOISE_LOW:
		return &STRING_TEXT_NOISE_LOW;
	case NOISE_MID:
		return &STRING_TEXT_NOISE_MID;
	case NOISE_HIGH:
		return &STRING_TEXT_NOISE_HIGH;
	case NOISE_VERY_HIGH:
		return &STRING_TEXT_NOISE_VERY_HIGH;
	}
	return nullptr;
}

wstring * SnowSetting::getScaleText()
{
	static std::wstring scaleRatioString = L"x1.0";
	switch (getScale()) {
	case SCALE_x1_0:
		return &STRING_TEXT_SCALE_x1_0;
	case SCALE_x1_5:
		return &STRING_TEXT_SCALE_x1_5;
	case SCALE_x1_6:
		return &STRING_TEXT_SCALE_x1_6;
	case SCALE_x2_0:
		return &STRING_TEXT_SCALE_x2_0;
	case SCALE_CUSTOM:
		scaleRatioString = L"x" + SnowSetting::getScaleRatio();
	}
	return &scaleRatioString;
}

wstring * SnowSetting::getGPUText()
{
	if (CurrentConverter == &CONVERTER_VULKAN) {
		if (getTTA() == TTA_DISABLED) {
			return &STRING_TEXT_GPU_VULKAN;
		}
		else {
			return &STRING_TEXT_GPU_VULKAN_TTA;
		}
	}
	else if (CurrentConverter == &CONVERTER_CUGAN) {
		if (getTTA() == TTA_DISABLED) {
			return &STRING_TEXT_GPU_CUGAN;
		}
		else {
			return &STRING_TEXT_GPU_CUGAN_TTA;
		}
	}
	else if (CurrentConverter == &CONVERTER_ESRGAN) {
		if (getTTA() == TTA_DISABLED) {
			return &STRING_TEXT_GPU_ESRGAN;
		}
		else {
			return &STRING_TEXT_GPU_ESRGAN_TTA;
		}
	}
	else if (getGPU() == GPU_CPU_MODE) {
		if (getTTA() == TTA_DISABLED) {
			return &STRING_TEXT_GPU_CPU;
		} else {
			return &STRING_TEXT_GPU_CPU_TTA;
		}

	} else {
		if (getTTA() == TTA_DISABLED) {
			return &STRING_TEXT_GPU_GPU;
		}
		else {
			return &STRING_TEXT_GPU_GPU_TTA;
		}
	}	return nullptr;
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

wstring dtos(double n) {
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