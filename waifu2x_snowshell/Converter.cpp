#include "Converter.h"

Converter::Converter() {
	this->Available = false;
	this->ExePath = L"";
	this->WorkingDir = L"";
	this->ModelDir = L"";
	this->CustomOption = L"";
	this->Is64bitOnly = true;
	this->IsCudaOnly = false;
	this->TTA = false;
	this->IsCPU = false;
	this->hConvertThread = nullptr;
	this->hConvertProcess = nullptr;
}

Converter::Converter(std::wstring exePath, bool is64bitOnly, bool isCudaOnly, bool tta) {
	if (exePath.empty())
		this->Available = false;
	else {
		this->ExePath = exePath;
		this->WorkingDir = exePath.substr(0, exePath.find_last_of('\\'));
		this->ModelDir = L"";
		this->CustomOption = L"";
		this->Is64bitOnly = is64bitOnly;
		this->IsCudaOnly = isCudaOnly;
		this->TTA = tta;
		this->IsCPU = false;
		this->hConvertThread = nullptr;
		this->hConvertProcess = nullptr;
		checkAvailable();
	}
}

Converter::~Converter() {
	if (hConvertThread != nullptr)
		TerminateThread(hConvertThread, 1);
	hConvertThread = nullptr;
	if (hConvertProcess != nullptr)
		TerminateProcess(hConvertProcess, 1);
	if (hProgressDlg != nullptr)
		EndDialog(hProgressDlg, 1);
	hProgressDlg = nullptr;
	if (hProgressThread != nullptr)
		TerminateThread(hProgressThread, 1);
	hProgressThread = nullptr;
}

bool Converter::checkAvailable() {
	this->Available = FileExists(ExePath.c_str());
	if (Is64bitOnly) {
		BOOL bIsWow64 = FALSE;
		IsWow64Process(GetCurrentProcess(), &bIsWow64);
		this->Available &= (bool)bIsWow64;
	}
	return this->Available;
}

void Converter::setCPU(bool isCPU) {
	this->IsCPU = isCPU;
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

bool Converter::getCPU() {
	return this->IsCPU;
}

bool Converter::getTTA() {
	return this->TTA;
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

bool Converter::execute(ConvertOption *convertOption, bool noLabel) {
	size_t last;
	std::wstring ExportName;
	std::wstringstream ExportNameStream;
	std::wstringstream ParamStream;

	ExportNameStream << convertOption->getInputFilePath().substr(0, convertOption->getInputFilePath().find_last_of(L".")) << L"_waifu2x";

	ParamStream << L"-i \"" << convertOption->getInputFilePath() << L"\" ";

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
	if (convertOption->getScaleRatio() != L"1.0") {
		ParamStream << L"--scale_ratio ";
		ParamStream << convertOption->getScaleRatio() << L" ";

		last = convertOption->getScaleRatio().find_last_of(L'.');
		if (!noLabel && last != std::wstring::npos)
			ExportNameStream << L"_scale_x" << convertOption->getScaleRatio().replace(last, last, L"_");
	}

	// set tta mode
	if (convertOption->getTTAEnabled() && this->TTA)
	{
		ParamStream << L"--tta 1 ";
		if (!noLabel)
			ExportNameStream << L"_tta_1";
	}

	// set core num
	if (!this->IsCudaOnly && convertOption->getCoreNum() > 0) {
		ParamStream << L"-j " << convertOption->getCoreNum() << L" ";
	}

	ExportName = ExportNameStream.str();

	// add extension
	if (!IsDirectory(convertOption->getInputFilePath().c_str()))
		ExportName += L".png";

	// create folder for folder conversion
	if (convertOption->getOutputFolderName() != L"") {
		last = convertOption->getInputFilePath().find_last_of(L'\\');
		if (last == std::wstring::npos)
			return false;
		CreateDirectory(convertOption->getOutputFolderName().c_str(), NULL);
		ExportName = convertOption->getOutputFolderName() + convertOption->getInputFilePath().substr(last, convertOption->getInputFilePath().find_last_of(L'.'));
	}

	// set model directory
	if (this->ModelDir != L"")
		ParamStream << L"--model_dir \"" << this->ModelDir << L"\" ";

	// set output name
	ParamStream << L"-o \"" << ExportName << L"\"";

	// Execute
	SHELLEXECUTEINFO si;
	WCHAR param[MAX_PATH] = L"";
	WCHAR lpDir[MAX_PATH] = L"";
	WCHAR lpFile[MAX_PATH] = L"";
	lstrcpyW(param, ParamStream.str().c_str());
	lstrcpyW(lpDir, WorkingDir.c_str());
	lstrcpyW(lpFile, ExePath.c_str());

	memset(&si, 0, sizeof(SHELLEXECUTEINFO));
	si.cbSize = sizeof(SHELLEXECUTEINFO);
	si.nShow = SW_SHOW;
	si.lpVerb = L"open";
	si.lpParameters = param;
	si.hwnd = NULL;
	si.lpDirectory = lpDir;
	si.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
	si.lpFile = lpFile;
	if (ShellExecuteExW(&si) == FALSE)
		return false;
	else {
		hConvertProcess = si.hProcess;
		if (hConvertProcess != nullptr)
			WaitForSingleObject(hConvertProcess, INFINITE);
		if (hConvertProcess != nullptr)
			TerminateProcess(hConvertProcess, 1);
		hConvertProcess = nullptr;
	}
	return true;
}

void Converter::addQueue(ConvertOption *convertOption) {
	ConvertQueue.push(*convertOption);
	if (hConvertThread == nullptr)
		hConvertThread = CreateThread(nullptr, 0, Converter::ConvertPorc, this, 0, nullptr);
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

	if (This->hProgressDlg != nullptr)
		EndDialog(This->hProgressDlg, 1);
	This->hProgressDlg = nullptr;
	if (This->hProgressThread != nullptr)
		TerminateThread(This->hProgressThread, 1);
	This->hProgressThread = nullptr;

	
	This->hProgressThread = CreateThread(nullptr, 0, Converter::ProgressPorc, This, 0, nullptr);

	for (int i = 0; !This->ConvertQueue.empty(); i++) {
		SendDlgItemMessage(This->hProgressDlg, IDC_PROGRESS1, PBM_SETRANGE, NULL, MAKELPARAM(0, This->ConvertQueue.size()+i));
		wsprintf(InQueueText, L"In queue: %d/%d", i, This->ConvertQueue.size() + i);
		SetDlgItemText(This->hProgressDlg, IDC_TEXT1, InQueueText);
		This->execute(&This->ConvertQueue.front());
		This->ConvertQueue.pop();
		SendDlgItemMessage(This->hProgressDlg, IDC_PROGRESS1, PBM_STEPIT, 0, 0);
	}

	SetDlgItemText(This->hProgressDlg, IDC_TEXT1, L"Done!");
	SendDlgItemMessage(This->hProgressDlg, IDC_PROGRESS1, PBM_SETPOS, (WPARAM)100, 0);
	This->hConvertThread = nullptr;
	ExitThread(0);
}

DWORD WINAPI Converter::ProgressPorc(PVOID lParam) {
	MSG msg;
	Converter* This = (Converter*)lParam;
	
	This->hProgressDlg = CreateDialog(NULL, MAKEINTRESOURCE(IDD_DIALOG2), NULL, Converter::ProgressDlgProc);


	while (GetMessage(&msg, This->hProgressDlg, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	This->hProgressDlg = nullptr;
	This->hProgressThread = nullptr;
	ExitThread(0);
}

BOOL CALLBACK Converter::ProgressDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static HWND hEdit, hText, hProgress, hButton;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		hEdit = GetDlgItem(hDlg, IDC_EDIT1);
		hText = GetDlgItem(hDlg, IDC_TEXT1);
		hProgress = GetDlgItem(hDlg, IDC_PROGRESS1);
		hButton = GetDlgItem(hDlg, IDCANCEL);
		SendMessage(hProgress, PBM_SETRANGE, NULL, MAKELPARAM(0, 1));
		SendMessage(hProgress, PBM_SETSTEP, (WPARAM)1, NULL);
		SetWindowText(hText, L"In queue: 0");
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		return FALSE;
	case WM_CLOSE:
		EndDialog(hDlg, IDCANCEL);
		return TRUE;
	}
	return DefWindowProc(hDlg, uMsg, wParam, lParam);
}