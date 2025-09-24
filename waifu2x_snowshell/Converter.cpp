#include "Converter.h"
#include "SnowSetting.h"

Converter::Converter() {
	this->Available = false;
	this->ExePath = L"";
	this->WorkingDir = L"";
	this->ModelDir = L"";
	this->CustomOption = L"";
	this->hConvertThread = nullptr;
	this->hConvertProcess = nullptr;
	this->hProgressDlg = nullptr;
}

Converter::Converter(std::wstring exePath) {
	this->ModelDir = L"";
	this->CustomOption = L"";
	this->hConvertThread = nullptr;
	this->hConvertProcess = nullptr;
	this->hProgressDlg = nullptr;

	if (exePath.empty()) {
		this->Available = false;
		this->ExePath = L"";
		this->WorkingDir = L"";
	}
	else {
		this->ExePath = exePath;
		this->WorkingDir = exePath.substr(0, exePath.find_last_of('\\'));
		this->checkAvailable();
	}
}

Converter::~Converter() {
	if (hConvertThread != nullptr)
		TerminateThread(hConvertThread, 1);
	hConvertThread = nullptr;
	if (hConvertProcess != nullptr)
		TerminateProcess(hConvertProcess, 1);
	if (hProgressDlg != nullptr)
		DestroyWindow(hProgressDlg);
	hProgressDlg = nullptr;
}

bool Converter::checkAvailable() {
	this->Available = FileExists(this->ExePath.c_str());
	return this->Available;
}

void Converter::setAvailable(bool available) {
	this->Available = available;
}

void Converter::setExePath(std::wstring exePath) {
	if (exePath.empty())
		this->Available = false;
	this->ExePath = exePath;
}

void Converter::setWorkingDir(std::wstring workingDir) {
	this->WorkingDir = workingDir;
}

void Converter::setModelDir(std::wstring modelDir) {
	this->ModelDir = modelDir;
}

void Converter::setOptionString(std::wstring optionString) {
	this->CustomOption = optionString;
}

bool Converter::getAvailable() {
	return this->Available;
}

std::wstring Converter::getExePath() {
	return this->ExePath;
}

std::wstring Converter::getWorkingDir() {
	return this->WorkingDir;
}

std::wstring Converter::getModelDir() {
	return this->ModelDir;
}

std::wstring Converter::getOptionString() {
	return this->CustomOption;
}

extern HINSTANCE g_hInst;
extern HWND hWnd;

void Converter::addQueue(ConvertOption *convertOption) {
	ConvertQueue.push(*convertOption);
	if (hConvertThread == nullptr) {
		if (hProgressDlg != nullptr)
			DestroyWindow(hProgressDlg);
		hProgressDlg = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, Converter::ProgressDlgProc);
		SendMessage(hProgressDlg, WM_SET_CONVERTER, (WPARAM)this, 0);
		ShowWindow(hProgressDlg, SW_SHOW);
		hConvertThread = CreateThread(nullptr, 0, Converter::ConvertPorc, this, 0, nullptr);
	}
}

void Converter::emptyQueue() {
	while (!ConvertQueue.empty())
		ConvertQueue.pop();
	if (hConvertThread != nullptr)
		TerminateThread(hConvertThread, 1);
	hConvertThread = nullptr;
	if (hConvertProcess != nullptr)
		TerminateProcess(hConvertProcess, 1);
	hConvertProcess = nullptr;
}

DWORD WINAPI Converter::ConvertPorc(PVOID lParam) {
	Converter* This = (Converter*)lParam;
	WCHAR InQueueText[20];

	std::queue<ConvertOption> ErrorQueue;

	for (int i = 0; !This->ConvertQueue.empty(); i++) {
		SendDlgItemMessage(This->hProgressDlg, IDC_PROGRESS1, PBM_SETRANGE32, 0, 10 * (This->ConvertQueue.size() + i + 1));
		SendDlgItemMessage(This->hProgressDlg, IDC_PROGRESS1, PBM_STEPIT, 0, 0);
		wsprintf(InQueueText, L"In queue: %d/%d", i, This->ConvertQueue.size() + i);
		SetDlgItemText(This->hProgressDlg, IDC_TEXT1, InQueueText);
		if(This->execute(&This->ConvertQueue.front())){
			Sleep(200);
		}
		else {
			ErrorQueue.push(This->ConvertQueue.front());
		}
		This->ConvertQueue.pop();
	}

	SetDlgItemText(This->hProgressDlg, IDC_TEXT1, L"Done!");
	SendDlgItemMessage(This->hProgressDlg, IDC_PROGRESS1, PBM_SETPOS, (WPARAM)100, 0);
	Sleep(300);
	SendMessage(This->hProgressDlg, WM_CLOSE, 0, 0);
	This->hConvertThread = nullptr;
	This->hProgressDlg = nullptr;

	if (!ErrorQueue.empty())
	{
		FILE *fp;
		_wfopen_s(&fp, L"error.log", L"wt+,ccs=UTF-16LE");

		if (fp) {
			fwprintf(fp, L"[Converter] \nCurrent: %s\n", This->getExePath().c_str());
			fwprintf(fp, L"Model: %s\n", This->getModelDir().c_str());
			fwprintf(fp, L"Option: %s\n", This->getOptionString().c_str());
			fwprintf(fp, L"WorkDir: %s\n", This->getWorkingDir().c_str());

			fwprintf(fp, L"\n[System] \nCuda: %s\n", SnowSetting::checkCuda() ? L"OK" : L"Fail");
			fwprintf(fp, L"Vulkan: %s\n", SnowSetting::checkVulkan() ? L"OK" : L"Fail");

			fwprintf(fp, L"waifu2x-converter-cpp: %s\n", SnowSetting::CONVERTER_CPP.getAvailable() == true ? L"OK" : L"Fail");
			fwprintf(fp, L"waifu2x-caffe: %s\n", SnowSetting::CONVERTER_CAFFE.getAvailable() == true ? L"OK" : L"Fail");
			fwprintf(fp, L"waifu2x-vulkan: %s\n", SnowSetting::CONVERTER_VULKAN.getAvailable() == true ? L"OK" : L"Fail");
			fwprintf(fp, L"waifu2x-CUGan: %s\n", SnowSetting::CONVERTER_CUGAN.getAvailable() == true ? L"OK" : L"Fail");
			fwprintf(fp, L"waifu2x-ESRGan: %s\n", SnowSetting::CONVERTER_ESRGAN.getAvailable() == true ? L"OK" : L"Fail");
			fwprintf(fp, L"\n\n%s\n\n", SnowSetting::checkProcessor(fp) ? L"" : L"Get processor list: Fail");

			while (!ErrorQueue.empty()) {
				fwprintf(fp, L"Error: %s\n", ErrorQueue.front().getInputFilePath().c_str());
				ErrorQueue.pop();
			}
			fclose(fp);
		}
		MessageBox(hWnd, L"Failed to convert some files.\nCheck \"error.log\"", L"Error", MB_ICONWARNING | MB_OK);
	}
	ExitThread(0);
}

INT_PTR CALLBACK Converter::ProgressDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static HWND hText, hProgress;
	static Converter *converter = nullptr;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		hText = GetDlgItem(hDlg, IDC_TEXT1);
		hProgress = GetDlgItem(hDlg, IDC_PROGRESS1);
		SendMessage(hProgress, PBM_SETRANGE32, 0, 100);
		SendMessage(hProgress, PBM_SETSTEP, (WPARAM)10, NULL);
		SetWindowText(hText, L"In queue: 0");
		return TRUE;
	case WM_SET_CONVERTER:
		converter = (Converter*)wParam;
		return TRUE;
	case WM_COMMAND:
		switch (wParam)
		{
		case IDCANCEL:
			if (converter != nullptr && converter->ConvertQueue.empty() || MessageBox(hWnd, STRING_TEXT_ABORT_CONVERT_MESSAGE.c_str(), STRING_TEXT_ABORT_CONVERT_TITLE.c_str(), MB_YESNO | MB_ICONEXCLAMATION | MB_SYSTEMMODAL) == IDYES) {
				if (converter != nullptr)
					converter->emptyQueue();
				DestroyWindow(hDlg);
				hDlg = nullptr;
			}
			return TRUE;
		}
		return FALSE;
	case WM_CLOSE:
		DestroyWindow(hDlg);
		hDlg = nullptr;
		return TRUE;
	}
	return FALSE;
}

bool Converter::convert(std::wstring param, std::wstring exportName, int debug) {
	SHELLEXECUTEINFO shellExecuteInfo;
	LPWSTR lpParam = new WCHAR[MAX_PATH];
	LPWSTR lpDir = new WCHAR[MAX_PATH];
	LPWSTR lpFile = new WCHAR[MAX_PATH];
	bool ret=true;

	lstrcpyW(lpParam, param.c_str());
	lstrcpyW(lpDir, WorkingDir.c_str());
	lstrcpyW(lpFile, ExePath.c_str());

	memset(&shellExecuteInfo, 0, sizeof(SHELLEXECUTEINFO));
	shellExecuteInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	if (debug == 0)
		shellExecuteInfo.nShow = SW_HIDE;
	else
		shellExecuteInfo.nShow = SW_SHOW;
	shellExecuteInfo.lpVerb = L"open";
	shellExecuteInfo.lpParameters = lpParam;
	shellExecuteInfo.hwnd = NULL;
	shellExecuteInfo.lpDirectory = lpDir;
	shellExecuteInfo.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
	shellExecuteInfo.lpFile = lpFile;

	if (!ShellExecuteExW(&shellExecuteInfo))
		ret=false;
	else {
		hConvertProcess = shellExecuteInfo.hProcess;
		if (hConvertProcess != nullptr)
			WaitForSingleObject(hConvertProcess, INFINITE);
		if (hConvertProcess != nullptr)
			TerminateProcess(hConvertProcess, 1);
		hConvertProcess = nullptr;
		if (shellExecuteInfo.hProcess != nullptr)
			CloseHandle(shellExecuteInfo.hProcess);
		if (!FileExists(exportName.c_str()))
			ret=false;
	}

	delete[] lpParam;
	delete[] lpDir;
	delete[] lpFile;

	return ret;
}

bool Converter_Cpp::execute(ConvertOption *convertOption, bool noLabel) {
	size_t last;
	std::wstring ExportName;
	std::wstring InputName = convertOption->getInputFilePath();
	std::wstringstream ExportNameStream;
	std::wstringstream ParamStream;

	last = InputName.find_last_of(L'\\');
	if (last == std::wstring::npos)
		return false;

	ParamStream << L"-i \"" << InputName << L"\" ";

	ExportNameStream << InputName.substr(0, InputName.find_last_of(L".")) << L"_waifu2x";

	// add custom option (user can use -- / --ignore_rest flag to ignore rest of parameter)
	if (this->CustomOption != L"")
		ParamStream << this->CustomOption << L" ";

	// set convert mode
	if (convertOption->getNoiseLevel() == ConvertOption::CO_NOISE_NONE) {
		ParamStream << L"-m scale ";
	}
	else if (convertOption->getScaleRatio() == L"1.0")
		ParamStream << L"-m noise ";
	else
		ParamStream << L"-m noise-scale ";

	// set noise_level
	if (convertOption->getNoiseLevel() != ConvertOption::CO_NOISE_NONE) {
		ParamStream << L"--noise-level " << convertOption->getNoiseLevel() << L" ";
		if (!noLabel)
			ExportNameStream << L"_noise" << convertOption->getNoiseLevel();
	}

	// set scale_ratio
	if (convertOption->getScaleRatio() != L"1.0" || convertOption->getNoiseLevel() == ConvertOption::CO_NOISE_NONE) {
		ParamStream << L"--scale-ratio ";
		ParamStream << convertOption->getScaleRatio() << L" ";

		std::wstring ScaleRatio = convertOption->getScaleRatio();
		size_t last = ScaleRatio.find_last_of(L'.');
		if (last != std::wstring::npos)
			ScaleRatio[last] = L'_';
		if (!noLabel)
			ExportNameStream << L"_scale_x" << ScaleRatio;
	}

	// set tta mode
	if (convertOption->getTTAEnabled())
	{
		ParamStream << L"--tta 1 ";
		if (!noLabel)
			ExportNameStream << L"_tta_1";
	}

	/*
	// set core num
	if (convertOption->getCoreNum() > 0) {
		ParamStream << L"-j " << convertOption->getCoreNum() << L" ";
	}*/

	// force cpu
	if (convertOption->getForceCPU()) {
		ParamStream << L"--disable-gpu ";
	}

	ExportName = ExportNameStream.str();

	// add extension
	if (!IsDirectory(InputName.c_str()))
		ExportName += L"." + convertOption->getOutputFileExtension();

	// create folder for folder conversion
	if (convertOption->getOutputFolderName() != L"") {
		CreateDirectory(convertOption->getOutputFolderName().c_str(), NULL);
		ExportName = convertOption->getOutputFolderName() + InputName.substr(last, InputName.find_last_of(L'.')) + L'.' + convertOption->getOutputFileExtension();
	}

	// set model directory
	if (this->ModelDir != L"")
		ParamStream << L"--model-dir \"" << this->ModelDir << L"\" ";

	// set output name
	ParamStream << L"-o \"" << ExportName << L"\"";

	// Execute
	return convert(ParamStream.str(), ExportName, convertOption->getDebugMode());
}


bool Converter_Caffe::execute(ConvertOption *convertOption, bool noLabel) {
	size_t last;
	std::wstring ExportName;
	std::wstring InputName = convertOption->getInputFilePath();
	std::wstringstream ExportNameStream;
	std::wstringstream ParamStream;

	last = InputName.find_last_of(L'\\');
	if (last == std::wstring::npos)
		return false;

	ParamStream << L"-i \"" << InputName << L"\" ";

	ExportNameStream << InputName.substr(0, InputName.find_last_of(L".")) << L"_waifu2x";

	// add custom option (user can use -- / --ignore_rest flag to ignore rest of parameter)
	if (this->CustomOption != L"")
		ParamStream << this->CustomOption << L" ";

	// set convert mode
	if (convertOption->getNoiseLevel() == ConvertOption::CO_NOISE_NONE) {
		ParamStream << L"-m scale ";
	}
	else if (convertOption->getScaleRatio() == L"1.0")
		ParamStream << L"-m noise ";
	else
		ParamStream << L"-m noise_scale ";

	// set noise_level
	if (convertOption->getNoiseLevel() != ConvertOption::CO_NOISE_NONE) {
		ParamStream << L"--noise_level " << convertOption->getNoiseLevel() << L" ";
		if (!noLabel)
			ExportNameStream << L"_noise" << convertOption->getNoiseLevel();
	}

	// set scale_ratio
	if (convertOption->getScaleRatio() != L"1.0" || convertOption->getNoiseLevel() == ConvertOption::CO_NOISE_NONE) {
		ParamStream << L"--scale_ratio ";
		ParamStream << convertOption->getScaleRatio() << L" ";

		std::wstring ScaleRatio = convertOption->getScaleRatio();
		size_t last = ScaleRatio.find_last_of(L'.');
		if (last != std::wstring::npos)
			ScaleRatio[last] = L'_';
		if (!noLabel)
			ExportNameStream << L"_scale_x" << ScaleRatio;
	}

	// set tta mode
	if (convertOption->getTTAEnabled())
	{
		ParamStream << L"--tta 1 ";
		if (!noLabel)
			ExportNameStream << L"_tta_1";
	}

	/*
	// set core num
	if (convertOption->getCoreNum() > 0) {
		ParamStream << L"-j " << convertOption->getCoreNum() << L" ";
	}*/

	// force cpu
	if (convertOption->getForceCPU()) {
		ParamStream << L"-p cpu ";
	}

	ExportName = ExportNameStream.str();

	// add extension
	if (!IsDirectory(InputName.c_str()))
		ExportName += L"." + convertOption->getOutputFileExtension();

	// create folder for folder conversion
	if (convertOption->getOutputFolderName() != L"") {
		CreateDirectory(convertOption->getOutputFolderName().c_str(), NULL);
		ExportName = convertOption->getOutputFolderName() + InputName.substr(last, InputName.find_last_of(L'.')) + L'.' + convertOption->getOutputFileExtension();
	}

	// set model directory
	if (this->ModelDir != L"")
		ParamStream << L"--model_dir \"" << this->ModelDir << L"\" ";

	// set output name
	ParamStream << L"-o \"" << ExportName << L"\"";

	// Execute
	return convert(ParamStream.str(), ExportName, convertOption->getDebugMode());
}


bool Converter_Vulkan::execute(ConvertOption* convertOption, bool noLabel) {
	size_t last;
	std::wstring ExportName;
	std::wstring InputName = convertOption->getInputFilePath();
	std::wstringstream ExportNameStream;
	std::wstringstream ParamStream;

	last = InputName.find_last_of(L'\\');
	if (last == std::wstring::npos)
		return false;

	ParamStream << L"-i \"" << InputName << L"\" ";

	ExportNameStream << InputName.substr(0, InputName.find_last_of(L".")) << L"_waifu2x";

	// add custom option (user can use -- / --ignore_rest flag to ignore rest of parameter)
	if (this->CustomOption != L"")
		ParamStream << this->CustomOption << L" ";

	// set noise_level
	ParamStream << L"-n " << convertOption->getNoiseLevel() << L" ";
	if (!noLabel)
		ExportNameStream << L"_noise" << convertOption->getNoiseLevel();

	// set scale_ratio
	ParamStream << L"-s ";
	ParamStream << convertOption->getScaleRatio() << L" ";

	std::wstring ScaleRatio = convertOption->getScaleRatio();

	if (!noLabel && (convertOption->getScaleRatio() != L"1" || convertOption->getNoiseLevel() == ConvertOption::CO_NOISE_NONE))
		ExportNameStream << L"_scale_x" << ScaleRatio;

	// set tta mode
	if (convertOption->getTTAEnabled())
	{
		ParamStream << L"-x ";
		if (!noLabel)
			ExportNameStream << L"_tta_1";
	}

	ExportName = ExportNameStream.str();

	// add extension
	if (!IsDirectory(InputName.c_str()))
		ExportName += L"." + convertOption->getOutputFileExtension();

	// create folder for folder conversion
	if (convertOption->getOutputFolderName() != L"") {
		CreateDirectory(convertOption->getOutputFolderName().c_str(), NULL);
		ExportName = convertOption->getOutputFolderName() + InputName.substr(last, InputName.find_last_of(L'.')) + L'.' + convertOption->getOutputFileExtension();
	}

	// set model directory
	if (this->ModelDir != L"")
		ParamStream << L"-m \"" << this->ModelDir << L"\" ";

	// set output name
	ParamStream << L"-o \"" << ExportName << L"\"";

	// Execute
	return convert(ParamStream.str(), ExportName, convertOption->getDebugMode());
}


bool Converter_Cugan::execute(ConvertOption* convertOption, bool noLabel) {
	size_t last;
	std::wstring ExportName;
	std::wstring InputName = convertOption->getInputFilePath();
	std::wstringstream ExportNameStream;
	std::wstringstream ParamStream;

	last = InputName.find_last_of(L'\\');
	if (last == std::wstring::npos)
		return false;

	ParamStream << L"-i \"" << InputName << L"\" ";

	ExportNameStream << InputName.substr(0, InputName.find_last_of(L".")) << L"_cugan";

	// add custom option (user can use -- / --ignore_rest flag to ignore rest of parameter)
	if (this->CustomOption != L"")
		ParamStream << this->CustomOption << L" ";

	// set noise_level
	ParamStream << L"-n " << convertOption->getNoiseLevel() << L" ";
	if (!noLabel)
		ExportNameStream << L"_noise" << convertOption->getNoiseLevel();

	// set scale_ratio
	ParamStream << L"-s ";
	ParamStream << convertOption->getScaleRatio() << L" ";

	std::wstring ScaleRatio = convertOption->getScaleRatio();

	if (!noLabel && (convertOption->getScaleRatio() != L"1" || convertOption->getNoiseLevel() == ConvertOption::CO_NOISE_NONE))
		ExportNameStream << L"_scale_x" << ScaleRatio;

	// set tta mode
	if (convertOption->getTTAEnabled())
	{
		ParamStream << L"-x ";
		if (!noLabel)
			ExportNameStream << L"_tta_1";
	}

	ExportName = ExportNameStream.str();

	// add extension
	if (!IsDirectory(InputName.c_str()))
		ExportName += L"." + convertOption->getOutputFileExtension();

	// create folder for folder conversion
	if (convertOption->getOutputFolderName() != L"") {
		CreateDirectory(convertOption->getOutputFolderName().c_str(), NULL);
		ExportName = convertOption->getOutputFolderName() + InputName.substr(last, InputName.find_last_of(L'.')) + L'.' + convertOption->getOutputFileExtension();
	}

	// set model directory
	if (this->ModelDir != L"")
		ParamStream << L"-m \"" << this->ModelDir << L"\" ";

	// set output name
	ParamStream << L"-o \"" << ExportName << L"\"";

	// Execute
	return convert(ParamStream.str(), ExportName, convertOption->getDebugMode());
}


bool Converter_Esrgan::execute(ConvertOption* convertOption, bool noLabel) {
	size_t last;
	std::wstring ExportName;
	std::wstring InputName = convertOption->getInputFilePath();
	std::wstringstream ExportNameStream;
	std::wstringstream ParamStream;

	last = InputName.find_last_of(L'\\');
	if (last == std::wstring::npos)
		return false;

	ParamStream << L"-i \"" << InputName << L"\" ";

	ExportNameStream << InputName.substr(0, InputName.find_last_of(L".")) << L"_esrgan";

	// add custom option (user can use -- / --ignore_rest flag to ignore rest of parameter)
	if (this->CustomOption != L"")
		ParamStream << this->CustomOption << L" ";

	// set scale_ratio
	ParamStream << L"-s ";
	ParamStream << convertOption->getScaleRatio() << L" ";

	std::wstring ScaleRatio = convertOption->getScaleRatio();

	if (!noLabel && (convertOption->getScaleRatio() != L"1" || convertOption->getNoiseLevel() == ConvertOption::CO_NOISE_NONE))
		ExportNameStream << L"_scale_x" << ScaleRatio;

	// set tta mode
	if (convertOption->getTTAEnabled())
	{
		ParamStream << L"-x ";
		if (!noLabel)
			ExportNameStream << L"_tta_1";
	}

	ExportName = ExportNameStream.str();

	// add extension
	if (!IsDirectory(InputName.c_str()))
		ExportName += L"." + convertOption->getOutputFileExtension();

	// create folder for folder conversion
	if (convertOption->getOutputFolderName() != L"") {
		CreateDirectory(convertOption->getOutputFolderName().c_str(), NULL);
		ExportName = convertOption->getOutputFolderName() + InputName.substr(last, InputName.find_last_of(L'.')) + L'.' + convertOption->getOutputFileExtension();
	}

	// set model directory
	if (this->ModelDir != L"")
		ParamStream << L"-m \"" << this->ModelDir << L"\" ";

	// set output name
	ParamStream << L"-o \"" << ExportName << L"\"";

	// Execute
	return convert(ParamStream.str(), ExportName, convertOption->getDebugMode());
}
