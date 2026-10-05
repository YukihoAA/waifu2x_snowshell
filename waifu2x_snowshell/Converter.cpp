#include "Converter.h"
#include "SnowSetting.h"
#include <vector>
#include <shellapi.h>

namespace {
bool HasEsrganModelOption(const std::wstring& options) {
	const std::wstring command = L"realesrgan " + options;
	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(command.c_str(), &argc);
	if (!argv) return false;
	bool hasModel = false;
	for (int i = 1; i < argc; ++i) {
		if (!wcscmp(argv[i], L"--")) break;
		if (!wcsncmp(argv[i], L"-n", 2)) {
			hasModel = true;
			break;
		}
		// Skip values of other options, which may contain text resembling -n.
		if (argv[i][0] == L'-' && argv[i][1] && !argv[i][2]
			&& wcschr(L"iostmgjf", argv[i][1]) && i + 1 < argc)
			++i;
	}
	LocalFree(argv);
	return hasModel;
}

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
	ConversionErrorLog errors;
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
		{ std::lock_guard<std::mutex> lock(This->DiagnosticMutex); This->LastFailure = ConversionFailure(); }
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
		if (cancelled) errors.reset();
        else if (!success) {
            ConversionFailure failure = This->getLastFailure();
            if (failure.stage.empty()) {
                GetLocalTime(&failure.time);
                failure.stage = L"prepare_arguments";
                failure.input = option.getInputFilePath();
                failure.scale = option.getScaleRatio();
                failure.executable = This->getExePath();
                failure.workDir = This->getWorkingDir();
            }
            errors.add(failure);
        }
        PostMessage(dialog, WM_CONVERT_PROGRESS, generation, 0);
        if (idle) {
            ConversionErrorNotice notice = errors.finish();
            if (notice.failedCount) PostConversionError(hWnd, std::move(notice));
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

ConversionFailure Converter::getLastFailure() {
    std::lock_guard<std::mutex> lock(DiagnosticMutex);
    return LastFailure;
}

bool Converter::convert(std::wstring param, std::wstring exportName, int debug, ConvertOption* option) {
    ConversionFailure failure;
    failure.output = exportName;
    { std::lock_guard<std::mutex> lock(QueueMutex); failure.executable = ExePath; failure.workDir = WorkingDir; failure.modelDirectory = ModelDir; }
    if (option) { failure.input = option->getInputFilePath(); failure.scale = option->getScaleRatio(); }
    DescribeConversion(failure, param, dynamic_cast<Converter_Esrgan*>(this) != nullptr);
    failure.command = L"\"" + failure.executable + L"\" " + param;
    auto failed = [&](const wchar_t* stage, DWORD error = 0) {
        failure.stage = stage; failure.windowsError = error; GetLocalTime(&failure.time);
        std::lock_guard<std::mutex> lock(DiagnosticMutex); LastFailure = failure; return false;
    };
    std::wstring outputArgument = L"-o \"" + exportName + L"\"";
    size_t outputPosition = param.rfind(outputArgument);
    if (outputPosition == std::wstring::npos || outputPosition + outputArgument.size() != param.size()) return failed(L"prepare_arguments", ERROR_INVALID_PARAMETER);
    size_t separator = exportName.find_last_of(L"\\/");
    if (separator == std::wstring::npos) return failed(L"prepare_output", ERROR_INVALID_NAME);
    std::wstring folder = exportName.substr(0, separator);
    if (!EnsureOutputDirectory(folder)) return failed(L"create_output_directory", GetLastError());
    std::vector<WCHAR> reserved(MAX_PATH, L'\0');
    if (!GetTempFileNameW(folder.c_str(), L"snw", 0, reserved.data())) return failed(L"create_temporary_file", GetLastError());
    size_t extension = exportName.find_last_of(L'.');
    std::wstring temporary = reserved.data();
    temporary += extension != std::wstring::npos && extension > separator ? exportName.substr(extension) : L".tmp";
    HANDLE output = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (output == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError(); DeleteFileW(reserved.data()); return failed(L"create_temporary_file", error);
    }
    CloseHandle(output);
    param.replace(outputPosition, outputArgument.size(), L"-o \"" + temporary + L"\"");
    failure.command = L"\"" + failure.executable + L"\" " + param;
    PROCESS_INFORMATION process = {};
    ProcessOutputCapture capture;
    bool launched = false;
    DWORD error = 0;
    {
        std::lock_guard<std::mutex> lock(QueueMutex);
        if (!CancelRequested && !Stopping) {
            launched = capture.launch(failure.executable, failure.command, failure.workDir, debug, process, error);
            if (launched) hConvertProcess = process.hProcess;
        } else failure.stage = L"cancelled";
    }
    bool success = false, captured = false;
    if (launched && process.hProcess && process.hThread) {
        CloseHandle(process.hThread);
        DWORD wait;
        HANDLE console = capture.consoleProcess();
        if (console) {
            HANDLE processes[] = { process.hProcess, console };
            wait = WaitForMultipleObjects(2, processes, FALSE, INFINITE);
            if (wait == WAIT_OBJECT_0 + 1) {
                // Closing the Debug console retains the former backend-console close behavior.
                TerminateProcess(process.hProcess, 1);
                wait = WaitForSingleObject(process.hProcess, INFINITE);
                failure.stage = L"debug_console_closed";
            }
        } else wait = WaitForSingleObject(process.hProcess, INFINITE);
        DWORD waitError = wait != WAIT_OBJECT_0 ? GetLastError() : 0;
        capture.finish(failure); captured = true;
        if (wait != WAIT_OBJECT_0) { failure.stage = L"wait_process"; failure.windowsError = waitError; }
        else if (!GetExitCodeProcess(process.hProcess, &failure.exitCode)) { failure.stage = L"read_exit_code"; failure.windowsError = GetLastError(); }
        else {
            failure.hasExitCode = true;
            if (failure.exitCode != 0) { if (failure.stage.empty()) failure.stage = L"exit_code"; }
            else if (!failure.backendError.empty()) failure.stage = L"backend_error";
            else {
                WIN32_FILE_ATTRIBUTE_DATA attributes = {};
                if (!GetFileAttributesExW(temporary.c_str(), GetFileExInfoStandard, &attributes)) { failure.stage = L"validate_output"; failure.windowsError = GetLastError(); }
                else if ((attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || (!attributes.nFileSizeHigh && !attributes.nFileSizeLow)) failure.stage = L"empty_output";
                else {
                    std::lock_guard<std::mutex> lock(QueueMutex);
                    if (!CancelRequested && !Stopping) {
                        success = MoveFileExW(temporary.c_str(), exportName.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
                        if (!success) { failure.stage = L"publish_output"; failure.windowsError = GetLastError(); }
                    } else failure.stage = L"cancelled";
                }
            }
        }
        { std::lock_guard<std::mutex> lock(QueueMutex); hConvertProcess = nullptr; }
        CloseHandle(process.hProcess);
    } else if (failure.stage.empty()) { failure.stage = L"launch_process"; failure.windowsError = error; }
    if (!captured) capture.finish(failure);
    DeleteFileW(temporary.c_str()); DeleteFileW(reserved.data());
    if (!success) {
        GetLocalTime(&failure.time);
        std::lock_guard<std::mutex> lock(DiagnosticMutex); LastFailure = std::move(failure);
    }
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
	return convert(ParamStream.str(), ExportName, convertOption->getDebugMode(), convertOption);
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
	return convert(ParamStream.str(), ExportName, convertOption->getDebugMode(), convertOption);
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
	return convert(ParamStream.str(), ExportName, convertOption->getDebugMode(), convertOption);
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
	return convert(ParamStream.str(), ExportName, convertOption->getDebugMode(), convertOption);
}


bool Converter_Esrgan::execute(ConvertOption* convertOption, bool noLabel) {
	const std::wstring scale = convertOption->getScaleRatio();
	const bool forceTTA = scale == L"2.0" || scale == L"2";
	// TTA avoids the bundled backend's RGBA x2 Vulkan failure; retain the caller's settings.
	ConvertOption effectiveOption = *convertOption;
	if (forceTTA)
		effectiveOption.setTTAEnabled(true);
	convertOption = &effectiveOption;
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

	const std::wstring customOption = getOptionString();
	// Select a model for this queued job only when the user has not specified -n.
	if (!HasEsrganModelOption(customOption)) {
		ParamStream << L"-n " << (scale == L"2.0" || scale == L"2"
			? L"realesr-animevideov3" : L"realesrgan-x4plus-anime") << L" ";
	}

	// Place forced TTA before custom options so -- cannot suppress it.
	if (forceTTA)
		ParamStream << L"-x ";

	// add custom option (user can use -- / --ignore_rest flag to ignore rest of parameter)
	if (!customOption.empty())
		ParamStream << customOption << L" ";

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
		if (!forceTTA)
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
	return convert(ParamStream.str(), ExportName, convertOption->getDebugMode(), convertOption);
}
