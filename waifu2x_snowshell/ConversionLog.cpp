#include "ConversionLog.h"
#include "SnowSetting.h"
#include <shellapi.h>
#include <vector>
#include <map>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <algorithm>
#include <cerrno>
#include <atomic>

namespace {
// Drain every byte, retaining a bounded tail for each stream.
const size_t OutputLimit = 64 * 1024;
std::mutex NoticeMutex;
std::map<UINT_PTR, ConversionErrorNotice> Notices;
UINT_PTR NextTicket = 0;

std::wstring FullPath(const std::wstring& value) {
    DWORD needed = GetFullPathNameW(value.c_str(), 0, nullptr, nullptr);
    if (!needed) return value;
    std::vector<wchar_t> buffer(needed);
    return GetFullPathNameW(value.c_str(), needed, buffer.data(), nullptr) ? buffer.data() : value;
}
DWORD FileError() {
    unsigned long error = 0;
    _get_doserrno(&error);
    return error ? error : ERROR_WRITE_FAULT;
}
std::wstring Decode(const std::string& bytes) {
    if (bytes.empty()) return L"";
    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(), (int)bytes.size(), nullptr, 0);
    if (count && bytes.find('\0') == std::string::npos) {
        std::wstring value(count, L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(), (int)bytes.size(), &value[0], count);
        // The UTF-16 text writer expands LF to CRLF; do not expand captured CRLF twice.
        std::wstring normalized;
        normalized.reserve(value.size());
        for (size_t i = 0; i < value.size(); ++i) {
            if (value[i] == L'\r' && i + 1 < value.size() && value[i + 1] == L'\n') continue;
            normalized += value[i];
        }
        return normalized;
    }
    std::wstring value = L"[Not valid UTF-8; original bytes in hexadecimal]\n";
    const wchar_t* digits = L"0123456789ABCDEF";
    for (unsigned char byte : bytes) { value += digits[byte >> 4]; value += digits[byte & 15]; value += L' '; }
    return value;
}
std::wstring LastLine(const std::string& bytes) {
    std::wstring text = Decode(bytes);
    size_t end = text.find_last_not_of(L"\r\n \t");
    if (end == std::wstring::npos) return L"";
    size_t start = text.find_last_of(L"\r\n", end);
    size_t offset = start == std::wstring::npos ? 0 : start + 1;
    return text.substr(offset, (std::min)(size_t(600), end - offset + 1));
}

struct HandleList {
    std::vector<unsigned char> storage;
    LPPROC_THREAD_ATTRIBUTE_LIST list = nullptr;
    ~HandleList() { if (list) DeleteProcThreadAttributeList(list); }
    bool initialize(HANDLE* handles, size_t count) {
        SIZE_T size = 0;
        InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
        if (!size) return false;
        storage.resize(size);
        auto* attributes = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
        if (!attributes || !InitializeProcThreadAttributeList(attributes, 1, 0, &size)) return false;
        list = attributes;
        return UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
            handles, count * sizeof(HANDLE), nullptr, nullptr) != FALSE;
    }
};
void Close(HANDLE& handle) { if (handle && handle != INVALID_HANDLE_VALUE) CloseHandle(handle); handle = nullptr; }

// A paused/closed debug console must never block the capture readers or conversion.
class ConsoleMirror {
    HANDLE writer = nullptr, process = nullptr;
    std::thread thread;
    std::mutex mirrorMutex;
    std::condition_variable ready;
    std::string pending;
    bool stopping = false;
public:
    ~ConsoleMirror() { finish(); }
    bool start() {
        SECURITY_ATTRIBUTES security = { sizeof(security), nullptr, TRUE };
        HANDLE reader = nullptr, nul = nullptr;
        if (!CreatePipe(&reader, &writer, &security, 0)) return false;
        if (!SetHandleInformation(writer, HANDLE_FLAG_INHERIT, 0)) { Close(reader); Close(writer); return false; }
        nul = CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &security, OPEN_EXISTING, 0, nullptr);
        if (nul == INVALID_HANDLE_VALUE) { Close(reader); Close(writer); return false; }
        HANDLE handles[] = { reader, nul };
        HandleList attributes;
        if (!attributes.initialize(handles, 2)) { Close(reader); Close(nul); Close(writer); return false; }
        STARTUPINFOEXW startup = {};
        startup.StartupInfo.cb = sizeof(startup);
        startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
        startup.StartupInfo.wShowWindow = SW_SHOW;
        startup.StartupInfo.hStdInput = reader;
        startup.StartupInfo.hStdOutput = startup.StartupInfo.hStdError = nul;
        startup.lpAttributeList = attributes.list;
        std::vector<wchar_t> self(32768);
        DWORD length = GetModuleFileNameW(nullptr, self.data(), (DWORD)self.size());
        PROCESS_INFORMATION child = {};
        std::wstring command = L"\"" + std::wstring(self.data(), length) + L"\" --snowshell-conversion-console";
        bool launched = length && length < self.size() && CreateProcessW(self.data(), &command[0], nullptr, nullptr,
            TRUE, EXTENDED_STARTUPINFO_PRESENT | CREATE_NEW_CONSOLE, nullptr, nullptr, &startup.StartupInfo, &child);
        Close(reader); Close(nul);
        if (!launched) { Close(writer); return false; }
        CloseHandle(child.hThread); process = child.hProcess;
        try {
            thread = std::thread([this] {
                for (;;) {
                    std::string bytes;
                    {
                        std::unique_lock<std::mutex> lock(mirrorMutex);
                        ready.wait(lock, [this] { return stopping || !pending.empty(); });
                        if (pending.empty() && stopping) break;
                        bytes.swap(pending);
                    }
                    size_t offset = 0;
                    while (offset < bytes.size()) {
                        DWORD written = 0;
                        if (!WriteFile(writer, bytes.data() + offset, (DWORD)(bytes.size() - offset), &written, nullptr) || !written) return;
                        offset += written;
                    }
                }
            });
        } catch (...) { finish(); return false; }
        return true;
    }
    HANDLE processHandle() const { return process; }
    void send(const char* data, size_t size) {
        if (!writer) return;
        std::lock_guard<std::mutex> lock(mirrorMutex);
        if (size >= OutputLimit) pending.assign(data + size - OutputLimit, OutputLimit);
        else {
            if (pending.size() + size > OutputLimit) pending.erase(0, pending.size() + size - OutputLimit);
            pending.append(data, size);
        }
        ready.notify_one();
    }
    void finish() {
        { std::lock_guard<std::mutex> lock(mirrorMutex); stopping = true; }
        ready.notify_one();
        if (thread.joinable()) {
            if (WaitForSingleObject(thread.native_handle(), 1000) != WAIT_OBJECT_0) {
                if (process) { TerminateProcess(process, 1); WaitForSingleObject(process, INFINITE); }
                CancelSynchronousIo(thread.native_handle());
            }
            thread.join();
        }
        Close(writer);
        if (process) {
            if (WaitForSingleObject(process, 1000) != WAIT_OBJECT_0) { TerminateProcess(process, 1); WaitForSingleObject(process, INFINITE); }
            Close(process);
        }
    }
};
struct CapturedStream {
    HANDLE reader = nullptr, writer = nullptr, thread = nullptr;
    std::string bytes;
    bool truncated = false;
    std::atomic<bool> stopReading{false};
    DWORD error = 0;
    ConsoleMirror* mirror = nullptr;
    bool inspectErrors = false;
    std::string errorLine, line;
    bool lineOverflow = false;
    void inspect(const char* data, size_t size) {
        if (!inspectErrors || !errorLine.empty()) return;
        // Inspect complete stderr lines before tail truncation, including split reads.
        for (size_t i = 0; i < size; ++i) {
            char ch = data[i];
            if (ch == '\r' || ch == '\n') {
                if (!lineOverflow) {
                    const char* prefixes[] = { "vkQueueSubmit failed ", "vkWaitForFences failed ", "vkAllocateMemory failed " };
                    for (const char* prefix : prefixes) {
                        size_t start = strlen(prefix);
                        if (line.compare(0, start, prefix) || line.size() <= start + 1 || line[start] != '-') continue;
                        bool digits = true;
                        for (size_t j = start + 1; j < line.size(); ++j) digits = digits && line[j] >= '0' && line[j] <= '9';
                        if (digits) { errorLine = line; return; }
                    }
                }
                line.clear(); lineOverflow = false;
            } else if (line.size() < 512) line += ch;
            else lineOverflow = true;
        }
    }
    static DWORD WINAPI read(PVOID parameter) {
        auto* stream = static_cast<CapturedStream*>(parameter);
        char buffer[8192]; DWORD count = 0;
        for (;;) {
            if (stream->stopReading) break;
            if (!ReadFile(stream->reader, buffer, sizeof(buffer), &count, nullptr)) {
                DWORD error = GetLastError();
                if (error != ERROR_BROKEN_PIPE) stream->error = error;
                break;
            }
            if (!count) break;
            stream->inspect(buffer, count);
            if (stream->mirror) stream->mirror->send(buffer, count);
            if (stream->bytes.size() + count > OutputLimit) {
                stream->bytes.erase(0, stream->bytes.size() + count - OutputLimit);
                stream->truncated = true;
            }
            stream->bytes.append(buffer, count);
        }
        return 0;
    }
    bool start(ConsoleMirror* console) {
        SECURITY_ATTRIBUTES security = { sizeof(security), nullptr, TRUE };
        if (!CreatePipe(&reader, &writer, &security, 0)) return false;
        if (!SetHandleInformation(reader, HANDLE_FLAG_INHERIT, 0)) return false;
        mirror = console;
        thread = CreateThread(nullptr, 0, read, this, 0, nullptr);
        return thread != nullptr;
    }
    void finish() {
        Close(writer);
        if (thread) {
            if (WaitForSingleObject(thread, 5000) != WAIT_OBJECT_0) {
                // A backend descendant may retain its inherited pipe after the backend exits.
                stopReading = true;
                do { CancelSynchronousIo(thread); } while (WaitForSingleObject(thread, 100) != WAIT_OBJECT_0);
            }
            WaitForSingleObject(thread, INFINITE); Close(thread);
        }
        inspect("\n", 1);
        Close(reader);
    }
    ~CapturedStream() { finish(); }
};
}

struct ProcessOutputCapture::Impl {
    ConsoleMirror console;
    CapturedStream out, err;
    DWORD consoleError = 0;
};
ProcessOutputCapture::ProcessOutputCapture() : impl(new Impl) {}
ProcessOutputCapture::~ProcessOutputCapture() = default;
HANDLE ProcessOutputCapture::consoleProcess() const { return impl->console.processHandle(); }
bool ProcessOutputCapture::launch(const std::wstring& executable, std::wstring command,
    const std::wstring& directory, int debug, PROCESS_INFORMATION& process, DWORD& error) {
    if (debug && !impl->console.start()) impl->consoleError = GetLastError();
    ConsoleMirror* console = debug ? &impl->console : nullptr;
    impl->err.inspectErrors = true;
    if (!impl->out.start(console) || !impl->err.start(console)) { error = GetLastError(); return false; }
    SECURITY_ATTRIBUTES security = { sizeof(security), nullptr, TRUE };
    HANDLE input = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &security, OPEN_EXISTING, 0, nullptr);
    if (input == INVALID_HANDLE_VALUE) { error = GetLastError(); return false; }
    HANDLE handles[] = { input, impl->out.writer, impl->err.writer };
    HandleList attributes;
    if (!attributes.initialize(handles, 3)) { error = GetLastError(); Close(input); return false; }
    STARTUPINFOEXW startup = {};
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    startup.StartupInfo.hStdInput = input;
    startup.StartupInfo.hStdOutput = impl->out.writer;
    startup.StartupInfo.hStdError = impl->err.writer;
    startup.lpAttributeList = attributes.list;
    bool launched = CreateProcessW(executable.c_str(), &command[0], nullptr, nullptr, TRUE,
        EXTENDED_STARTUPINFO_PRESENT | CREATE_NO_WINDOW, nullptr, directory.empty() ? nullptr : directory.c_str(), &startup.StartupInfo, &process) != FALSE;
    error = launched ? 0 : GetLastError();
    Close(input); Close(impl->out.writer); Close(impl->err.writer);
    return launched;
}
void ProcessOutputCapture::finish(ConversionFailure& failure) {
    impl->out.finish(); impl->err.finish(); impl->console.finish();
    failure.stdoutBytes.swap(impl->out.bytes); failure.stderrBytes.swap(impl->err.bytes);
    failure.stdoutTruncated = impl->out.truncated; failure.stderrTruncated = impl->err.truncated;
    failure.captureError = impl->out.error ? impl->out.error : impl->err.error;
    failure.debugConsoleError = impl->consoleError;
    failure.backendError = impl->err.errorLine;
}

int RunConversionLogConsole() {
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    AllocConsole();
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitleW(L"Snowshell - Conversion Log");
    HANDLE console = CreateFileW(L"CONOUT$", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (console == INVALID_HANDLE_VALUE) return 1;
    char buffer[8192]; DWORD count = 0;
    while (ReadFile(input, buffer, sizeof(buffer), &count, nullptr) && count) {
        DWORD offset = 0;
        while (offset < count) {
            DWORD written = 0;
            if (!WriteFile(console, buffer + offset, count - offset, &written, nullptr) || !written) { CloseHandle(console); return 1; }
            offset += written;
        }
    }
    CloseHandle(console); return 0;
}

std::wstring ConversionWindowsError(DWORD error) {
    if (!error) return L"";
    LPWSTR buffer = nullptr;
    DWORD size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, error, 0, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
    std::wstring value = size ? std::wstring(buffer, size) : L"Unknown Windows error";
    if (buffer) LocalFree(buffer);
    while (!value.empty() && (value.back() == L'\r' || value.back() == L'\n')) value.pop_back();
    return value;
}
void DescribeConversion(ConversionFailure& failure, const std::wstring& arguments, bool esrgan) {
    int count = 0;
    std::wstring command = L"converter " + arguments;
    LPWSTR* argv = CommandLineToArgvW(command.c_str(), &count);
    std::wstring model = L"models";
    if (argv) {
        for (int i = 1; i < count; ++i) {
            if (!wcscmp(argv[i], L"--")) break;
            std::wstring token = argv[i];
            if (token.size() == 2 && token[0] == L'-' && wcschr(L"iostmgjfn", token[1]) && i + 1 < count) {
                std::wstring value = argv[++i];
                if (token == L"-i" && failure.input.empty()) failure.input = value;
                if (token == L"-s" && failure.scale.empty()) failure.scale = value;
                if (esrgan && token == L"-n") failure.modelName = value;
                if (esrgan && token == L"-m") model = value;
            }
        }
        LocalFree(argv);
    }
    if (esrgan) {
        failure.modelArgument = model;
        // The bundled backend prefixes its executable directory, including for -m.
        failure.modelDirectory = FullPath(failure.workDir + L"\\" + model);
    }
}

ConversionErrorLog::~ConversionErrorLog() { closeTemporary(); }
void ConversionErrorLog::closeTemporary() {
    if (file) { fclose(file); file = nullptr; }
    if (!temporary.empty()) { DeleteFileW(temporary.c_str()); temporary.clear(); }
}
void ConversionErrorLog::reset() { closeTemporary(); notice = ConversionErrorNotice(); }
void ConversionErrorLog::add(const ConversionFailure& failure) {
    if (!notice.failedCount) {
        notice.logPath = FullPath(L"error.log");
        notice.firstFailure = failure;
        std::wstring detail = LastLine(failure.stderrBytes);
        int size = WideCharToMultiByte(CP_UTF8, 0, detail.data(), (int)detail.size(), nullptr, 0, nullptr, nullptr);
        notice.firstFailure.stderrBytes.assign(size, '\0');
        if (size) WideCharToMultiByte(CP_UTF8, 0, detail.data(), (int)detail.size(), &notice.firstFailure.stderrBytes[0], size, nullptr, nullptr);
        notice.firstFailure.stdoutBytes.clear();
        std::wstring directory = notice.logPath.substr(0, notice.logPath.find_last_of(L"\\/"));
        std::vector<wchar_t> path(32768);
        if (!GetTempFileNameW(directory.c_str(), L"err", 0, path.data())) notice.logError = GetLastError();
        else {
            temporary = path.data();
            if (_wfopen_s(&file, temporary.c_str(), L"wt,ccs=UTF-16LE")) notice.logError = FileError();
        }
        if (file) {
            fwprintf(file, L"Snowshell conversion error log\n[System]\nCuda: %s\nVulkan: %s\n", SnowSetting::checkCuda() ? L"OK" : L"Unavailable", SnowSetting::checkVulkan() ? L"OK" : L"Unavailable");
            fwprintf(file, L"Converter executable availability (does not validate models):\n");
            for (Converter* converter : std::vector<Converter*>{ &SnowSetting::CONVERTER_CPP, &SnowSetting::CONVERTER_CAFFE, &SnowSetting::CONVERTER_VULKAN, &SnowSetting::CONVERTER_CUGAN, &SnowSetting::CONVERTER_ESRGAN })
                fwprintf(file, L"%s: %s\n", converter->getExePath().c_str(), converter->getAvailable() ? L"Present" : L"Unavailable");
            fwprintf(file, L"\n[Processor listing]\n");
            if (!SnowSetting::checkProcessor(file)) fwprintf(file, L"Processor listing unavailable\n");
        }
    }
    ++notice.failedCount;
    if (!file) return;
    const auto& t = failure.time;
    fwprintf(file, L"\n[Failure %zu]\nLocal time: %04u-%02u-%02u %02u:%02u:%02u\nStage: %s\n", notice.failedCount, t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, failure.stage.c_str());
    fwprintf(file, L"Input: %s\nOutput: %s\nScale: %s\nExecutable: %s\nWorkDir: %s\nModel name: %s\nModel directory: %s\nModel argument: %s\nCommand: %s\n",
        failure.input.c_str(), failure.output.c_str(), failure.scale.c_str(), failure.executable.c_str(), failure.workDir.c_str(), failure.modelName.empty() ? L"(converter default / not specified)" : failure.modelName.c_str(), failure.modelDirectory.c_str(), failure.modelArgument.c_str(), failure.command.c_str());
    if (failure.windowsError) fwprintf(file, L"Windows error: %lu (0x%08lX) %s\n", failure.windowsError, failure.windowsError, ConversionWindowsError(failure.windowsError).c_str());
    if (failure.captureError) fwprintf(file, L"Output capture warning: %lu %s\n", failure.captureError, ConversionWindowsError(failure.captureError).c_str());
    if (failure.debugConsoleError) fwprintf(file, L"Debug console warning: %lu %s\n", failure.debugConsoleError, ConversionWindowsError(failure.debugConsoleError).c_str());
    if (failure.hasExitCode) fwprintf(file, L"Exit code: %lu (0x%08lX)\n", failure.exitCode, failure.exitCode);
    else fwprintf(file, L"Exit code: (process not completed)\n");
    if (!failure.backendError.empty()) fwprintf(file, L"Backend error: %s\n", Decode(failure.backendError).c_str());
    fwprintf(file, L"[stdout%s]\n%s\n[stderr%s]\n%s\n", failure.stdoutTruncated ? L"; truncated, last 64 KiB retained" : L"", Decode(failure.stdoutBytes).c_str(), failure.stderrTruncated ? L"; truncated, last 64 KiB retained" : L"", Decode(failure.stderrBytes).c_str());
    if (ferror(file) || fflush(file)) notice.logError = FileError();
}
ConversionErrorNotice ConversionErrorLog::finish() {
    if (file) { if (fclose(file)) notice.logError = FileError(); file = nullptr; }
    if (!notice.logError && !temporary.empty()) {
        notice.logSaved = MoveFileExW(temporary.c_str(), notice.logPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
        if (!notice.logSaved) notice.logError = GetLastError();
    }
    closeTemporary();
    ConversionErrorNotice result = std::move(notice); notice = ConversionErrorNotice(); return result;
}
bool PostConversionError(HWND window, ConversionErrorNotice notice) {
    std::lock_guard<std::mutex> lock(NoticeMutex);
    do { ++NextTicket; } while (!NextTicket || Notices.count(NextTicket));
    UINT_PTR ticket = NextTicket;
    Notices.emplace(ticket, std::move(notice));
    if (PostMessageW(window, WM_CONVERT_ERROR, 0, (LPARAM)ticket)) return true;
    Notices.erase(ticket); return false;
}
bool TakeConversionError(UINT_PTR ticket, ConversionErrorNotice& notice) {
    std::lock_guard<std::mutex> lock(NoticeMutex);
    auto found = Notices.find(ticket);
    if (found == Notices.end()) return false;
    notice = std::move(found->second); Notices.erase(found); return true;
}
std::wstring FormatConversionError(const ConversionErrorNotice& notice) {
    const auto& failure = notice.firstFailure;
    std::wstring reason = failure.windowsError ? ConversionWindowsError(failure.windowsError)
        : !failure.backendError.empty() ? Decode(failure.backendError) : LastLine(failure.stderrBytes);
    if (reason.empty()) reason = failure.hasExitCode && failure.exitCode != 0 ? STRING_TEXT_CONVERT_PROCESS_FAILED : STRING_TEXT_CONVERT_OUTPUT_FAILED;
    std::wstring text = std::to_wstring(notice.failedCount) + L" " + STRING_TEXT_CONVERT_ERROR_MESSAGE;
    text += L"\n\n" + STRING_TEXT_CONVERT_ERROR_INPUT + L": " + failure.input;
    if (!failure.modelName.empty()) text += L"\n" + STRING_TEXT_CONVERT_ERROR_MODEL + L": " + failure.modelName;
    if (!failure.scale.empty()) text += L"\n" + STRING_TEXT_SCALE + L": x" + failure.scale;
    text += L"\n" + STRING_TEXT_CONVERT_ERROR_REASON + L": " + reason;
    text += L"\n\n" + (notice.logSaved ? STRING_TEXT_CONVERT_ERROR_LOG : STRING_TEXT_CONVERT_ERROR_LOG_FAILED) + L":\n" + notice.logPath;
    if (!notice.logSaved) text += L"\n" + ConversionWindowsError(notice.logError);
    return text;
}
