#include "Converter.h"

Converter::Converter() {
	this->Available = false;
	this->ExePath = L"";
	this->WorkingDir = L"";
	this->Is64bitOnly = true;
	this->IsCudaOnly = false;
	this->TTA = false;
	this->IsCPU = false;
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

bool Converter::Execute(HWND hWnd, ConvertOption *convertOption) {
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
		ExportNameStream << L"_noise" << convertOption->getNoiseLevel();
	}

	// set scale_ratio
	if (convertOption->getScaleRatio() != L"1.0") {
		ParamStream << L"--scale_ratio ";
		ParamStream << convertOption->getScaleRatio() << L" ";

		last = convertOption->getScaleRatio().find_last_of(L'.');
		if (last != std::wstring::npos)
			ExportNameStream << L"_scale_x" << convertOption->getScaleRatio().replace(last, last, L"_");
	}

	// set tta mode
	if (convertOption->getTTAEnabled() && this->TTA)
	{
		ParamStream << L"--tta 1 ";
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
		last = ExportName.find_last_of(L'\\');
		if (last == std::wstring::npos)
			return false;
		CreateDirectory((ExportName.substr(0, last) + convertOption->getOutputFolderName()).c_str(), NULL);
		ExportName = ExportName.substr(0, last) + convertOption->getOutputFolderName() + ExportName.substr(last);
	}

	ParamStream << L"-o \"" << ExportName << L"\"";

	SHELLEXECUTEINFO si;
	WCHAR param[MAX_PATH] = L"";
	WCHAR lpDir[MAX_PATH] = L"";
	lstrcpyW(param, ParamStream.str().c_str());
	lstrcpyW(lpDir, WorkingDir.c_str());

	memset(&si, 0, sizeof(SHELLEXECUTEINFO));
	si.cbSize = sizeof(SHELLEXECUTEINFO);
	si.nShow = SW_SHOW;
	si.lpVerb = L"open";
	si.lpParameters = param;
	si.hwnd = hWnd;
	si.lpDirectory = lpDir;
	si.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
	si.lpFile = ExePath.c_str();
	if (ShellExecuteExW(&si) == FALSE)
		return false;
	else
		WaitForSingleObject(si.hProcess, INFINITE);
	return true;
}