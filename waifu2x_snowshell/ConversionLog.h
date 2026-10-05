#pragma once
#include <Windows.h>
#include <string>
#include <memory>
#include <cstdio>

struct ConversionFailure {
    SYSTEMTIME time = {};
    std::wstring input, output, executable, workDir, modelDirectory, modelArgument, modelName, scale, command, stage;
    DWORD windowsError = 0, exitCode = 0;
    DWORD captureError = 0, debugConsoleError = 0;
    bool hasExitCode = false, stdoutTruncated = false, stderrTruncated = false;
    std::string stdoutBytes, stderrBytes, backendError;
};

struct ConversionErrorNotice {
    size_t failedCount = 0;
    std::wstring logPath;
    bool logSaved = false;
    DWORD logError = 0;
    ConversionFailure firstFailure;
};

class ProcessOutputCapture {
    struct Impl;
    std::unique_ptr<Impl> impl;
public:
    ProcessOutputCapture();
    ~ProcessOutputCapture();
    bool launch(const std::wstring& executable, std::wstring command, const std::wstring& directory,
        int debug, PROCESS_INFORMATION& process, DWORD& error);
    void finish(ConversionFailure& failure);
    HANDLE consoleProcess() const;
};

class ConversionErrorLog {
    FILE* file = nullptr;
    std::wstring temporary;
    ConversionErrorNotice notice;
    void closeTemporary();
public:
    ConversionErrorLog() = default;
    ConversionErrorLog(const ConversionErrorLog&) = delete;
    ConversionErrorLog& operator=(const ConversionErrorLog&) = delete;
    ~ConversionErrorLog();
    void add(const ConversionFailure& failure);
    ConversionErrorNotice finish();
    void reset();
};

void DescribeConversion(ConversionFailure& failure, const std::wstring& arguments, bool esrgan);
std::wstring ConversionWindowsError(DWORD error);
bool PostConversionError(HWND window, ConversionErrorNotice notice);
bool TakeConversionError(UINT_PTR ticket, ConversionErrorNotice& notice);
std::wstring FormatConversionError(const ConversionErrorNotice& notice);
int RunConversionLogConsole();
