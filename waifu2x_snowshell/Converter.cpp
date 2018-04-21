#include "Converter.h"

Converter::Converter() {
	this->Available = false;
	this->ExePath = L"";
	this->WorkingDir = L"";
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
		this->Is64bitOnly = is64bitOnly;
		this->IsCudaOnly = isCudaOnly;
		this->TTA = tta;
		this->IsCPU = false;
		this->hConvertThread = nullptr;
		this->hConvertProcess = nullptr;
		checkAvailable();
	}
}

bool Converter::checkAvailable() {
	if (Is64bitOnly) {
		BOOL bIsWow64;
		IsWow64Process(GetCurrentProcess(), &bIsWow64);
		this->Available &= bIsWow64 == TRUE;
	}
	this->Available = FileExists(ExePath.c_str());
	return this->Available;
}

void Converter::setCPU(bool isCPU) {
	this->IsCPU = isCPU;
}

void Converter::setAvailable(bool available) {
	this->Available = available;
}

void Converter::setExePath(std::wstring exePath) {
	this->ExePath = exePath;
}

void Converter::setWorkingDir(std::wstring workingDir) {
	this->WorkingDir = workingDir;
}

bool Converter::getCPU() {
	return this->IsCPU;
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

bool Converter::execute(ConvertOption *convertOption, bool noLabel) {
	size_t last;
	std::wstring ExportName;
	std::wstringstream ExportNameStream;
	std::wstringstream ParamStream;

	ExportNameStream << convertOption->getInputFilePath().substr(0, convertOption->getInputFilePath().find_last_of(L".")) << L"_waifu2x";

	ParamStream << L"-i \"" << convertOption->getInputFilePath() << L"\" ";

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
		if(!noLabel)
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
		if(!noLabel)
			ExportNameStream << L"_tta_1";
	}

	// set core num
	if (IsCPU && convertOption->getCoreNum() > 0) {
		ParamStream << L"-j " << convertOption->getCoreNum() << L" ";
	}

	ExportName = ExportNameStream.str();


	if (!IsDirectory(convertOption->getInputFilePath().c_str()))
		ExportName += L".png";

	if (convertOption->getOutputFolderName() != L"") {
		last = convertOption->getInputFilePath().find_last_of(L'\\');
		if (last == std::wstring::npos)
			return false;
		CreateDirectory(convertOption->getOutputFolderName().c_str(), NULL);
		ExportName = convertOption->getOutputFolderName() + convertOption->getInputFilePath().substr(last, convertOption->getInputFilePath().find_last_of(L'.'));
	}

	ParamStream << L"-o \"" << ExportName << L"\"";

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
		if(hConvertProcess != nullptr)
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
	if(hConvertThread!=nullptr)
		TerminateThread(hConvertThread, 1);
	hConvertThread = nullptr;
	if (hConvertProcess != nullptr)
		TerminateProcess(hConvertProcess, 1);
	hConvertProcess = nullptr;
}

DWORD WINAPI Converter::ConvertPorc(PVOID lParam) {
	Converter* This = (Converter*)lParam;

	while (!This->ConvertQueue.empty()) {
		This->execute(&This->ConvertQueue.front());
		This->ConvertQueue.pop();
	}
	This->hConvertThread = nullptr;
	ExitThread(0);
}