#include "Converter.h"
#include "SnowSetting.h"
#include <vector>

namespace {
std::wstring InputNameWithoutExtension(const std::wstring& inputName) {
	const size_t separator = inputName.find_last_of(L"\\/");
	const size_t extension = inputName.find_last_of(L'.');
	return extension != std::wstring::npos && (separator == std::wstring::npos || extension > separator)
		? inputName.substr(0, extension) : inputName;
}

bool EnsureOutputDirectory(const std::wstring& directory) {
	DWORD attributes = GetFileAttributesW(directory.c_str());
	if (attributes != INVALID_FILE_ATTRIBUTES)
		return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	if (CreateDirectoryW(directory.c_str(), nullptr)) return true;
	const DWORD error = GetLastError();
	if (error == ERROR_ALREADY_EXISTS) {
		attributes = GetFileAttributesW(directory.c_str());
		return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	}
	if (error != ERROR_PATH_NOT_FOUND) return false;
	const size_t separator = directory.find_last_of(L"\\/");
	if (separator == std::wstring::npos || separator == 0) return false;
	if (!EnsureOutputDirectory(directory.substr(0, separator))) return false;
	if (CreateDirectoryW(directory.c_str(), nullptr)) return true;
	if (GetLastError() != ERROR_ALREADY_EXISTS) return false;
	attributes = GetFileAttributesW(directory.c_str());
	return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}
}

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

Converter::Converter(std::wstring exePath) : Converter() {
	setExePath(exePath);
	checkAvailable();
}

Converter::~Converter() {
	shutdown();
}

void Converter::shutdown() {
	HANDLE thread = nullptr;
	HWND dialog = nullptr;
	{
		std::lock_guard<std::mutex> lock(QueueMutex);
		Stopping = true;
		CancelRequested = true;
		while (!ConvertQueue.empty()) ConvertQueue.pop();
		if (hConvertProcess != nullptr) TerminateProcess(hConvertProcess, 1);
		thread = hConvertThread;
		hConvertThread = nullptr;
		dialog = hProgressDlg;
		hProgressDlg = nullptr;
	}
	QueueReady.notify_all();
	if (thread != nullptr) {
		WaitForSingleObject(thread, INFINITE);
		CloseHandle(thread);
	}
	if (dialog != nullptr) DestroyWindow(dialog);
}

bool Converter::checkAvailable() {
	std::lock_guard<std::mutex> lock(QueueMutex);
	Available = FileExists(ExePath.c_str()) && !IsDirectory(ExePath.c_str());
	return Available;
}

void Converter::setAvailable(bool available) {
	std::lock_guard<std::mutex> lock(QueueMutex);
	Available = available;
}

void Converter::setExePath(std::wstring exePath) {
	std::lock_guard<std::mutex> lock(QueueMutex);
	ExePath = exePath;
	size_t separator = exePath.find_last_of(L"\\/");
	WorkingDir = separator == std::wstring::npos ? L"" : exePath.substr(0, separator);
	Available = false;
}

void Converter::setWorkingDir(std::wstring workingDir) {
	std::lock_guard<std::mutex> lock(QueueMutex);
	WorkingDir = workingDir;
}

void Converter::setModelDir(std::wstring modelDir) {
	std::lock_guard<std::mutex> lock(QueueMutex);
	ModelDir = modelDir;
}

void Converter::setOptionString(std::wstring optionString) {
	std::lock_guard<std::mutex> lock(QueueMutex);
	CustomOption = optionString;
}

bool Converter::getAvailable() {
	std::lock_guard<std::mutex> lock(QueueMutex);
	return Available;
}

std::wstring Converter::getExePath() {
	std::lock_guard<std::mutex> lock(QueueMutex);
	return ExePath;
}

std::wstring Converter::getWorkingDir() {
	std::lock_guard<std::mutex> lock(QueueMutex);
	return WorkingDir;
}

std::wstring Converter::getModelDir() {
	std::lock_guard<std::mutex> lock(QueueMutex);
	return ModelDir;
}

std::wstring Converter::getOptionString() {
	std::lock_guard<std::mutex> lock(QueueMutex);
	return CustomOption;
}

extern HINSTANCE g_hInst;
extern HWND hWnd;

void Converter::addQueue(ConvertOption *convertOption) {
	HWND dialog;
	{
		std::lock_guard<std::mutex> lock(QueueMutex);
		if (Stopping) return;
		ConvertQueue.push(*convertOption);
		++ProgressGeneration;
		dialog = hProgressDlg;
	}
	if (dialog == nullptr) {
		dialog = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, Converter::ProgressDlgProc, (LPARAM)this);
		{
			std::lock_guard<std::mutex> lock(QueueMutex);
			hProgressDlg = dialog;
			CompletedCount = 0;
		}
		ShowWindow(dialog, SW_SHOW);
	}
	{
		std::lock_guard<std::mutex> lock(QueueMutex);
		if (hConvertThread == nullptr)
			hConvertThread = CreateThread(nullptr, 0, Converter::ConvertPorc, this, 0, nullptr);
		if (hConvertThread == nullptr) {
			while (!ConvertQueue.empty()) ConvertQueue.pop();
			PostMessage(hWnd, WM_CONVERT_ERROR, 1, 0);
			PostMessage(dialog, WM_CONVERT_FINISHED, ProgressGeneration, 0);
		}
	}
	QueueReady.notify_one();
}

void Converter::emptyQueue() {
	std::lock_guard<std::mutex> lock(QueueMutex);
	CancelRequested = true;
	while (!ConvertQueue.empty()) ConvertQueue.pop();
	if (hConvertProcess != nullptr) TerminateProcess(hConvertProcess, 1);
}

DWORD WINAPI Converter::ConvertPorc(PVOID lParam) {
	Converter* This = (Converter*)lParam;
	std::queue<ConvertOption> ErrorQueue;
	for (;;) {
		ConvertOption option;
		HWND dialog;
		unsigned generation;
		{
			std::unique_lock<std::mutex> lock(This->QueueMutex);
			This->QueueReady.wait(lock, [This] { return This->Stopping || !This->ConvertQueue.empty(); });
			if (This->Stopping) return 0;
			option = This->ConvertQueue.front();
			This->ConvertQueue.pop();
			This->CancelRequested = false;
			This->Converting = true;
			dialog = This->hProgressDlg;
			generation = This->ProgressGeneration;
		}
		PostMessage(dialog, WM_CONVERT_PROGRESS, generation, 0);
		bool success = This->execute(&option, option.getNoLabel());
		bool idle, cancelled;
		{
			std::lock_guard<std::mutex> lock(This->QueueMutex);
			This->Converting = false;
			cancelled = This->CancelRequested || This->Stopping;
			++This->CompletedCount;
			idle = This->ConvertQueue.empty();
			dialog = This->hProgressDlg;
			generation = This->ProgressGeneration;
		}
		if (cancelled) {
			while (!ErrorQueue.empty()) ErrorQueue.pop();
		} else if (!success) ErrorQueue.push(option);
		PostMessage(dialog, WM_CONVERT_PROGRESS, generation, 0);
		if (idle) {
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
				PostMessage(hWnd, WM_CONVERT_ERROR, 0, 0);
			}
			PostMessage(dialog, WM_CONVERT_FINISHED, generation, 0);
		}
	}
}

INT_PTR CALLBACK Converter::ProgressDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	Converter* converter = (Converter*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	switch (uMsg) {
	case WM_INITDIALOG:
		SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
		SetDlgItemText(hDlg, IDC_TEXT1, L"In queue: 0");
		return TRUE;
	case WM_SET_CONVERTER:
		SetWindowLongPtr(hDlg, GWLP_USERDATA, wParam);
		return TRUE;
	case WM_CONVERT_PROGRESS: {
		if (converter == nullptr) return TRUE;
		size_t completed, total;
		{
			std::lock_guard<std::mutex> lock(converter->QueueMutex);
			if (wParam != converter->ProgressGeneration || hDlg != converter->hProgressDlg) return TRUE;
			completed = converter->CompletedCount;
			total = completed + converter->ConvertQueue.size() + (converter->Converting ? 1 : 0);
		}
		std::wstring text = L"In queue: " + std::to_wstring(completed) + L"/" + std::to_wstring(total);
		SetDlgItemText(hDlg, IDC_TEXT1, text.c_str());
		SendDlgItemMessage(hDlg, IDC_PROGRESS1, PBM_SETRANGE32, 0, (LPARAM)total);
		SendDlgItemMessage(hDlg, IDC_PROGRESS1, PBM_SETPOS, (WPARAM)completed, 0);
		return TRUE;
	}
	case WM_CONVERT_FINISHED:
		if (converter != nullptr) {
			std::lock_guard<std::mutex> lock(converter->QueueMutex);
			if (wParam != converter->ProgressGeneration || converter->Converting || !converter->ConvertQueue.empty()) return TRUE;
			if (hDlg == converter->hProgressDlg) converter->hProgressDlg = nullptr;
		}
		DestroyWindow(hDlg);
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) != IDCANCEL) return FALSE;
		// Fall through to the same cancellation path as the title-bar close button.
	case WM_CLOSE:
		if (converter != nullptr && MessageBox(hWnd, STRING_TEXT_ABORT_CONVERT_MESSAGE.c_str(), STRING_TEXT_ABORT_CONVERT_TITLE.c_str(), MB_YESNO | MB_ICONEXCLAMATION | MB_SYSTEMMODAL) != IDYES) return TRUE;
		if (converter != nullptr) {
			converter->emptyQueue();
			std::lock_guard<std::mutex> lock(converter->QueueMutex);
			if (hDlg == converter->hProgressDlg) converter->hProgressDlg = nullptr;
		}
		DestroyWindow(hDlg);
		return TRUE;
	}
	return FALSE;
}

bool Converter::convert(std::wstring param, std::wstring exportName, int debug) {
	// All five converters append this output argument after their custom options.
	std::wstring outputArgument = L"-o \"" + exportName + L"\"";
	size_t outputPosition = param.rfind(outputArgument);
	if (outputPosition == std::wstring::npos || outputPosition + outputArgument.size() != param.size()) return false;
	size_t separator = exportName.find_last_of(L"\\/");
	if (separator == std::wstring::npos) return false;
	std::wstring folder = exportName.substr(0, separator);
	if (!EnsureOutputDirectory(folder)) return false;
	std::vector<WCHAR> reserved(MAX_PATH, L'\0');
	if (!GetTempFileNameW(folder.c_str(), L"snw", 0, reserved.data())) return false;
	size_t extension = exportName.find_last_of(L'.');
	std::wstring temporary = reserved.data();
	temporary += extension != std::wstring::npos && extension > separator ? exportName.substr(extension) : L".tmp";
	HANDLE output = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (output == INVALID_HANDLE_VALUE) {
		DeleteFileW(reserved.data());
		return false;
	}
	CloseHandle(output);
	param.replace(outputPosition, outputArgument.size(), L"-o \"" + temporary + L"\"");
	PROCESS_INFORMATION process = {};
	STARTUPINFOW startup = {};
	startup.cb = sizeof(startup);
	startup.dwFlags = STARTF_USESHOWWINDOW;
	startup.wShowWindow = debug == 0 ? SW_HIDE : SW_SHOW;
	bool launched = false;
	{
		std::lock_guard<std::mutex> lock(QueueMutex);
		if (!CancelRequested && !Stopping) {
			std::wstring command = L"\"" + ExePath + L"\" " + param;
			launched = CreateProcessW(ExePath.c_str(), &command[0], nullptr, nullptr, FALSE,
				debug == 0 ? CREATE_NO_WINDOW : 0, nullptr, WorkingDir.empty() ? nullptr : WorkingDir.c_str(), &startup, &process) != FALSE;
			if (launched) hConvertProcess = process.hProcess;
		}
	}
	bool success = false;
	if (launched) {
		CloseHandle(process.hThread);
		DWORD wait = WaitForSingleObject(process.hProcess, INFINITE);
		DWORD exitCode = 1;
		WIN32_FILE_ATTRIBUTE_DATA attributes = {};
		bool valid = wait == WAIT_OBJECT_0 && GetExitCodeProcess(process.hProcess, &exitCode) && exitCode == 0
			&& GetFileAttributesExW(temporary.c_str(), GetFileExInfoStandard, &attributes)
			&& !(attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			&& (attributes.nFileSizeHigh != 0 || attributes.nFileSizeLow != 0);
		{
			std::lock_guard<std::mutex> lock(QueueMutex);
			hConvertProcess = nullptr;
			if (valid && !CancelRequested && !Stopping)
				success = MoveFileExW(temporary.c_str(), exportName.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
		}
		CloseHandle(process.hProcess);
	}
	DeleteFileW(temporary.c_str());
	DeleteFileW(reserved.data());
	return success;
}

bool Converter_Cpp::execute(ConvertOption *convertOption, bool noLabel) {
	noLabel = noLabel || convertOption->getNoLabel();
	size_t last;
	std::wstring ExportName;
	std::wstring InputName = convertOption->getInputFilePath();
	std::wstringstream ExportNameStream;
	std::wstringstream ParamStream;

	last = InputName.find_last_of(L'\\');
	if (last == std::wstring::npos)
		return false;

	ParamStream << L"-i \"" << InputName << L"\" ";

	ExportNameStream << InputNameWithoutExtension(InputName) << L"_waifu2x";

	// add custom option (user can use -- / --ignore_rest flag to ignore rest of parameter)
	if (getOptionString() != L"")
		ParamStream << getOptionString() << L" ";

	if (convertOption->getTileSize() > 0)
		ParamStream << L"--block-size " << convertOption->getTileSize() << L" ";

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

	// set output path for folder conversion
	if (convertOption->getOutputFolderName() != L"") {
		ExportName = convertOption->getOutputFolderName() + InputNameWithoutExtension(InputName).substr(last) + L'.' + convertOption->getOutputFileExtension();
	}

	// set model directory
	if (getModelDir() != L"")
		ParamStream << L"--model-dir \"" << getModelDir() << L"\" ";

	// set output name
	ParamStream << L"-o \"" << ExportName << L"\"";

	// Execute
	return convert(ParamStream.str(), ExportName, convertOption->getDebugMode());
}


bool Converter_Caffe::execute(ConvertOption *convertOption, bool noLabel) {
	noLabel = noLabel || convertOption->getNoLabel();
	size_t last;
	std::wstring ExportName;
	std::wstring InputName = convertOption->getInputFilePath();
	std::wstringstream ExportNameStream;
	std::wstringstream ParamStream;

	last = InputName.find_last_of(L'\\');
	if (last == std::wstring::npos)
		return false;

	ParamStream << L"-i \"" << InputName << L"\" ";

	ExportNameStream << InputNameWithoutExtension(InputName) << L"_waifu2x";

	// add custom option (user can use -- / --ignore_rest flag to ignore rest of parameter)
	if (getOptionString() != L"")
		ParamStream << getOptionString() << L" ";

	if (convertOption->getTileSize() > 0)
		ParamStream << L"-c " << convertOption->getTileSize() << L" ";

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

	// set output path for folder conversion
	if (convertOption->getOutputFolderName() != L"") {
		ExportName = convertOption->getOutputFolderName() + InputNameWithoutExtension(InputName).substr(last) + L'.' + convertOption->getOutputFileExtension();
	}

	// set model directory
	if (getModelDir() != L"")
		ParamStream << L"--model_dir \"" << getModelDir() << L"\" ";

	// set output name
	ParamStream << L"-o \"" << ExportName << L"\"";

	// Execute
	return convert(ParamStream.str(), ExportName, convertOption->getDebugMode());
}


bool Converter_Vulkan::execute(ConvertOption* convertOption, bool noLabel) {
	noLabel = noLabel || convertOption->getNoLabel();
	size_t last;
	std::wstring ExportName;
	std::wstring InputName = convertOption->getInputFilePath();
	std::wstringstream ExportNameStream;
	std::wstringstream ParamStream;

	last = InputName.find_last_of(L'\\');
	if (last == std::wstring::npos)
		return false;

	ParamStream << L"-i \"" << InputName << L"\" ";

	ExportNameStream << InputNameWithoutExtension(InputName) << L"_waifu2x";

	// add custom option (user can use -- / --ignore_rest flag to ignore rest of parameter)
	if (getOptionString() != L"")
		ParamStream << getOptionString() << L" ";

	if (convertOption->getTileSize() > 0)
		ParamStream << L"-t " << convertOption->getTileSize() << L" ";

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

	// set output path for folder conversion
	if (convertOption->getOutputFolderName() != L"") {
		ExportName = convertOption->getOutputFolderName() + InputNameWithoutExtension(InputName).substr(last) + L'.' + convertOption->getOutputFileExtension();
	}

	// set model directory
	if (getModelDir() != L"")
		ParamStream << L"-m \"" << getModelDir() << L"\" ";

	// set output name
	ParamStream << L"-o \"" << ExportName << L"\"";

	// Execute
	return convert(ParamStream.str(), ExportName, convertOption->getDebugMode());
}


bool Converter_Cugan::execute(ConvertOption* convertOption, bool noLabel) {
	noLabel = noLabel || convertOption->getNoLabel();
	size_t last;
	std::wstring ExportName;
	std::wstring InputName = convertOption->getInputFilePath();
	std::wstringstream ExportNameStream;
	std::wstringstream ParamStream;

	last = InputName.find_last_of(L'\\');
	if (last == std::wstring::npos)
		return false;

	ParamStream << L"-i \"" << InputName << L"\" ";

	ExportNameStream << InputNameWithoutExtension(InputName) << L"_cugan";

	// add custom option (user can use -- / --ignore_rest flag to ignore rest of parameter)
	if (getOptionString() != L"")
		ParamStream << getOptionString() << L" ";

	if (convertOption->getTileSize() > 0)
		ParamStream << L"-t " << convertOption->getTileSize() << L" ";

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

	// set output path for folder conversion
	if (convertOption->getOutputFolderName() != L"") {
		ExportName = convertOption->getOutputFolderName() + InputNameWithoutExtension(InputName).substr(last) + L'.' + convertOption->getOutputFileExtension();
	}

	// set model directory
	if (getModelDir() != L"")
		ParamStream << L"-m \"" << getModelDir() << L"\" ";

	// set output name
	ParamStream << L"-o \"" << ExportName << L"\"";

	// Execute
	return convert(ParamStream.str(), ExportName, convertOption->getDebugMode());
}


bool Converter_Esrgan::execute(ConvertOption* convertOption, bool noLabel) {
	noLabel = noLabel || convertOption->getNoLabel();
	size_t last;
	std::wstring ExportName;
	std::wstring InputName = convertOption->getInputFilePath();
	std::wstringstream ExportNameStream;
	std::wstringstream ParamStream;

	last = InputName.find_last_of(L'\\');
	if (last == std::wstring::npos)
		return false;

	ParamStream << L"-i \"" << InputName << L"\" ";

	ExportNameStream << InputNameWithoutExtension(InputName) << L"_esrgan";

	// add custom option (user can use -- / --ignore_rest flag to ignore rest of parameter)
	if (getOptionString() != L"")
		ParamStream << getOptionString() << L" ";

	if (convertOption->getTileSize() > 0)
		ParamStream << L"-t " << convertOption->getTileSize() << L" ";

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

	// set output path for folder conversion
	if (convertOption->getOutputFolderName() != L"") {
		ExportName = convertOption->getOutputFolderName() + InputNameWithoutExtension(InputName).substr(last) + L'.' + convertOption->getOutputFileExtension();
	}

	// set model directory
	if (getModelDir() != L"")
		ParamStream << L"-m \"" << getModelDir() << L"\" ";

	// set output name
	ParamStream << L"-o \"" << ExportName << L"\"";

	// Execute
	return convert(ParamStream.str(), ExportName, convertOption->getDebugMode());
}
