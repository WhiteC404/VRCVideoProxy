#include "core.hpp"
#include <winhttp.h>
#include <algorithm>
#include <cwctype>
#include <limits>
#include <stdexcept>

namespace vrcvp {
namespace {
std::wstring Trim(const std::wstring& s) {
    const auto start = s.find_first_not_of(L" \t\r\n");
    if (start == std::wstring::npos) return {};
    return s.substr(start, s.find_last_not_of(L" \t\r\n") - start + 1);
}
[[noreturn]] void Fail(const std::wstring& message) { throw std::runtime_error(Utf8(message)); }
class Handle {
public:
    explicit Handle(HANDLE value) : value_(value) {}
    ~Handle() { if (value_ && value_ != INVALID_HANDLE_VALUE) CloseHandle(value_); }
    HANDLE get() const { return value_; }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
private:
    HANDLE value_;
};
}
std::string Utf8(const std::wstring& text) {
    if (text.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (!size) throw std::runtime_error("Invalid Unicode text.");
    std::string result(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), result.data(), size, nullptr, nullptr);
    return result;
}
std::wstring Wide(const std::string& text) {
    if (text.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (!size) throw std::runtime_error("Configuration must be UTF-8 text.");
    std::wstring result(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), result.data(), size);
    return result;
}
std::wstring ErrorText(DWORD code) {
    wchar_t* message = nullptr;
    const DWORD length = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, code, 0, reinterpret_cast<wchar_t*>(&message), 0, nullptr);
    std::wstring result = length ? std::wstring(message, length) : L"Windows error " + std::to_wstring(code);
    if (message) LocalFree(message);
    return Trim(result);
}
bool IgnoreCase::operator()(const std::wstring& a, const std::wstring& b) const {
    return CompareStringOrdinal(a.data(), static_cast<int>(a.size()), b.data(), static_cast<int>(b.size()), TRUE) == CSTR_LESS_THAN;
}
std::wstring ExePath() {
    std::vector<wchar_t> buffer(32768);
    const DWORD count = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (!count || count >= buffer.size()) Fail(L"Cannot locate the executable.");
    return std::wstring(buffer.data(), count);
}
std::wstring AbsolutePath(const std::wstring& path) {
    if (path.empty() || path.find(L'\0') != std::wstring::npos) Fail(L"A non-empty file path is required.");
    const DWORD length = GetFullPathNameW(path.c_str(), 0, nullptr, nullptr);
    if (!length) Fail(L"Cannot resolve the file path: " + ErrorText(GetLastError()));
    std::vector<wchar_t> buffer(static_cast<size_t>(length) + 1);
    if (!GetFullPathNameW(path.c_str(), static_cast<DWORD>(buffer.size()), buffer.data(), nullptr)) Fail(L"Cannot resolve the file path.");
    return buffer.data();
}
std::wstring Directory(const std::wstring& path) {
    const auto index = path.find_last_of(L"\\/");
    if (index == 2 && path.size() > 2 && path[1] == L':') return path.substr(0, 3);
    return index == std::wstring::npos ? L"." : path.substr(0, index);
}
std::wstring QuoteArgument(const std::wstring& argument) {
    // Quote every argument. Backslashes preceding a quote or the closing quote
    // follow the Microsoft CRT rules; no shell is involved.
    std::wstring result = L"\"";
    size_t slashes = 0;
    for (wchar_t c : argument) {
        if (c == L'\\') { ++slashes; continue; }
        if (c == L'"') { result.append(slashes * 2 + 1, L'\\'); result += c; }
        else { result.append(slashes, L'\\'); result += c; }
        slashes = 0;
    }
    result.append(slashes * 2, L'\\');
    return result + L'"';
}
std::wstring CommandLine(const std::wstring& executable, const std::vector<std::wstring>& args) {
    std::wstring line = QuoteArgument(executable);
    for (const auto& arg : args) line += L" " + QuoteArgument(arg);
    if (line.size() >= 32767) Fail(L"The launch command is too long for Windows.");
    return line;
}
std::wstring NormalizeNoProxy(const std::wstring& text) {
    if (text.size() > 16384) Fail(L"The direct-domain list is too long.");
    std::wstring result, token;
    bool bareLoopback = false, bracketLoopback = false;
    auto append = [&]() {
        if (token.empty()) return;
        if (token.find(L"://") != std::wstring::npos || token.find(L';') != std::wstring::npos ||
            (token != L"*" && token.find_first_of(L"*?") != std::wstring::npos)) {
            Fail(L"Use hostnames such as .bilivideo.com; separate them with commas or new lines. Do not enter URLs or *.example.com.");
        }
        for (wchar_t c : token) {
            if (c > 127 || !(iswalnum(c) || c == L'.' || c == L'-' || c == L'_' || c == L':' || c == L'[' || c == L']' || c == L'*'))
                Fail(L"The direct-domain list contains an invalid hostname.");
        }
        bareLoopback = bareLoopback || token == L"::1";
        bracketLoopback = bracketLoopback || token == L"[::1]";
        if (!result.empty()) result += L',';
        result += token;
        token.clear();
    };
    for (wchar_t c : text) {
        if (c == L',' || iswspace(c)) append();
        else token += c;
    }
    append();
    if (bareLoopback && !bracketLoopback) result += L",[::1]";
    if (bracketLoopback && !bareLoopback) result += L",::1";
    return result;
}
void EnableCacherCompatibility(Config& config) {
    config.noProxy = NormalizeNoProxy(config.noProxy);
    const std::vector<std::wstring> required{L"localhost", L"127.0.0.1", L"::1", L"[::1]", L"localhost.youtube.com"};
    for (const auto& host : required) {
        bool present = false;
        size_t offset = 0;
        while (offset < config.noProxy.size()) {
            const auto end = config.noProxy.find(L',', offset);
            const auto value = config.noProxy.substr(offset, end == std::wstring::npos ? end : end - offset);
            if (_wcsicmp(value.c_str(), host.c_str()) == 0) { present = true; break; }
            offset = end == std::wstring::npos ? config.noProxy.size() : end + 1;
        }
        if (!present) {
            if (!config.noProxy.empty()) config.noProxy += L',';
            config.noProxy += host;
        }
    }
}
void Validate(Config& config) {
    config.proxy = Trim(config.proxy);
    if (config.proxy.empty() || config.proxy.size() > 2048 || config.proxy.find_first_of(L"\r\n\t ") != std::wstring::npos || config.proxy.find(L'\0') != std::wstring::npos)
        Fail(L"Enter an HTTP proxy URL, for example http://127.0.0.1:7890.");
    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    parts.dwHostNameLength = parts.dwUserNameLength = parts.dwPasswordLength = parts.dwUrlPathLength = parts.dwExtraInfoLength = static_cast<DWORD>(-1);
    if (!WinHttpCrackUrl(config.proxy.c_str(), static_cast<DWORD>(config.proxy.size()), 0, &parts) ||
        parts.nScheme != INTERNET_SCHEME_HTTP || !parts.dwHostNameLength || !parts.nPort ||
        parts.dwUserNameLength || parts.dwPasswordLength || parts.dwExtraInfoLength ||
        (parts.dwUrlPathLength && std::wstring(parts.lpszUrlPath, parts.dwUrlPathLength) != L"/"))
        Fail(L"Use an HTTP proxy URL without a path, query or credentials, for example http://127.0.0.1:7890.");
    if (config.proxy.back() == L'/') config.proxy.pop_back();
    config.noProxy = NormalizeNoProxy(config.noProxy);
}
Config ReadConfig(const std::wstring& path) {
    Config result;
    Handle file(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (file.get() == INVALID_HANDLE_VALUE) {
        const DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) return result;
        Fail(L"Cannot read the configuration: " + ErrorText(error));
    }
    LARGE_INTEGER length{};
    if (!GetFileSizeEx(file.get(), &length) || length.QuadPart > 65536) Fail(L"Configuration exceeds 64 KiB.");
    std::string bytes(static_cast<size_t>(length.QuadPart), '\0');
    DWORD read = 0;
    if (!ReadFile(file.get(), bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr) || read != bytes.size()) Fail(L"Cannot read the configuration.");
    if (bytes.compare(0, 3, "\xef\xbb\xbf") == 0) bytes.erase(0, 3);
    const auto text = Wide(bytes);
    if (text.find(L'\0') != std::wstring::npos) Fail(L"Configuration must not contain NUL characters.");
    bool proxySeen = false, noProxySeen = false;
    size_t offset = 0;
    while (offset < text.size()) {
        const auto end = text.find(L'\n', offset);
        const auto line = Trim(text.substr(offset, end == std::wstring::npos ? end : end - offset));
        offset = end == std::wstring::npos ? text.size() : end + 1;
        if (line.empty() || line[0] == L'#' || line[0] == L';') continue;
        const auto equal = line.find(L'=');
        if (equal == std::wstring::npos) Fail(L"Each configuration entry must be key=value.");
        const auto key = Trim(line.substr(0, equal));
        const auto value = Trim(line.substr(equal + 1));
        if (key == L"proxy_url" && !proxySeen) { result.proxy = value; proxySeen = true; }
        else if (key == L"no_proxy" && !noProxySeen) { result.noProxy = value; noProxySeen = true; }
        else Fail(L"Unknown or duplicate configuration key: " + key);
    }
    Validate(result);
    return result;
}
void WriteConfig(const std::wstring& path, const Config& config) {
    Config normalized = config;
    Validate(normalized);
    const std::string text = "# VRC Video Proxy - UTF-8\r\nproxy_url=" + Utf8(normalized.proxy) + "\r\nno_proxy=" + Utf8(normalized.noProxy) + "\r\n";
    const std::wstring temp = path + L".tmp." + std::to_wstring(GetCurrentProcessId()) + L"." + std::to_wstring(GetTickCount64());
    bool written = false;
    {
        Handle file(CreateFileW(temp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
        if (file.get() == INVALID_HANDLE_VALUE) Fail(L"Cannot save settings. Place the program in a writable folder. " + ErrorText(GetLastError()));
        DWORD count = 0;
        written = WriteFile(file.get(), text.data(), static_cast<DWORD>(text.size()), &count, nullptr) && count == text.size() && FlushFileBuffers(file.get());
    }
    if (!written) { DeleteFileW(temp.c_str()); Fail(L"Cannot finish writing settings."); }
    if (!MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        const auto error = GetLastError();
        DeleteFileW(temp.c_str());
        Fail(L"Cannot replace the configuration: " + ErrorText(error));
    }
}
Environment InheritedEnvironment() {
    Environment result;
    const auto block = GetEnvironmentStringsW();
    if (!block) Fail(L"Cannot read the parent environment.");
    for (const wchar_t* item = block; *item; item += wcslen(item) + 1) {
        const std::wstring entry(item);
        const auto equal = entry.find(L'=', entry[0] == L'=' ? 1 : 0);
        if (equal != std::wstring::npos) result[entry.substr(0, equal)] = entry.substr(equal + 1);
    }
    FreeEnvironmentStringsW(block);
    return result;
}
std::vector<wchar_t> ChildEnvironment(Environment environment, const Config& config) {
    environment[L"HTTP_PROXY"] = config.proxy;
    environment[L"HTTPS_PROXY"] = config.proxy;
    environment[L"ALL_PROXY"] = config.proxy;
    environment[L"NO_PROXY"] = config.noProxy;
    std::vector<wchar_t> block;
    for (const auto& entry : environment) {
        const auto value = entry.first + L"=" + entry.second;
        block.insert(block.end(), value.begin(), value.end());
        block.push_back(L'\0');
    }
    block.push_back(L'\0');
    return block;
}
DWORD Launch(const std::wstring& executable, const std::vector<std::wstring>& args, const Config& config) {
    const auto path = AbsolutePath(executable);
    const auto attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY)) Fail(L"Launcher not found: " + path);
    if (path.size() < 4 || _wcsicmp(path.c_str() + path.size() - 4, L".exe") != 0) Fail(L"Steam must pass an .exe launcher as the first argument.");
    auto command = CommandLine(path, args);
    auto block = ChildEnvironment(InheritedEnvironment(), config);
    const auto directory = Directory(path);
    STARTUPINFOW start{};
    start.cb = sizeof(start);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(path.c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT,
                        block.data(), directory.c_str(), &start, &process))
        Fail(L"Cannot start the original launcher: " + ErrorText(GetLastError()));
    Handle thread(process.hThread), child(process.hProcess);
    if (WaitForSingleObject(child.get(), INFINITE) != WAIT_OBJECT_0) Fail(L"Cannot wait for the launcher.");
    DWORD code = 0;
    if (!GetExitCodeProcess(child.get(), &code)) Fail(L"Cannot read the launcher exit code.");
    return code;
}
Options ParseOptions(const std::vector<std::wstring>& args, const std::wstring& defaultConfig) {
    Options result;
    result.configPath = defaultConfig;
    if (args.empty()) { result.settings = true; return result; }
    size_t i = 0;
    for (; i < args.size(); ++i) {
        const auto& arg = args[i];
        if (arg == L"--") { ++i; break; }
        if (arg == L"--no-ui") result.noUi = true;
        else if (arg == L"--config") {
            if (++i == args.size()) Fail(L"--config requires a file path.");
            result.configPath = AbsolutePath(args[i]); result.customConfig = true;
        } else if (arg == L"--settings") result.settings = true;
        else if (arg == L"--check-config") result.check = true;
        else if (arg == L"--print-steam-option") result.printSteam = true;
        else if (arg == L"--help" || arg == L"-h") result.help = true;
        else if (arg == L"--version") result.version = true;
        else if (!arg.empty() && arg[0] == L'-') Fail(L"Unknown launcher option: " + arg);
        else break;
    }
    if (i < args.size()) {
        result.launcher = args[i++];
        result.arguments.assign(args.begin() + static_cast<std::ptrdiff_t>(i), args.end());
    }
    const int actions = static_cast<int>(result.settings) + result.check + result.printSteam + result.help + result.version;
    if (actions > 1 || (actions && !result.launcher.empty())) Fail(L"Choose one launcher action.");
    if (!actions && result.launcher.empty()) Fail(L"Pass Steam's original launcher after VRCVideoProxy.exe, or run --settings.");
    return result;
}
std::wstring SteamOption(const std::wstring& executable, const Options& options) {
    auto result = QuoteArgument(AbsolutePath(executable));
    if (options.customConfig) result += L" --config " + QuoteArgument(options.configPath);
    return result + L" %command%";
}
void WriteOutput(const std::wstring& message, bool error) {
    HANDLE output = GetStdHandle(error ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
    if (!output || output == INVALID_HANDLE_VALUE) {
        // Attach only when needed, retaining any redirected stream supplied by
        // the caller because AttachConsole can replace both standard handles.
        const HANDLE previousOut = GetStdHandle(STD_OUTPUT_HANDLE);
        const HANDLE previousError = GetStdHandle(STD_ERROR_HANDLE);
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            if (previousOut && previousOut != INVALID_HANDLE_VALUE) SetStdHandle(STD_OUTPUT_HANDLE, previousOut);
            if (previousError && previousError != INVALID_HANDLE_VALUE) SetStdHandle(STD_ERROR_HANDLE, previousError);
        }
        output = GetStdHandle(error ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
    }
    if (!output || output == INVALID_HANDLE_VALUE) return;
    DWORD mode = 0, count = 0;
    if (GetConsoleMode(output, &mode)) WriteConsoleW(output, message.data(), static_cast<DWORD>(message.size()), &count, nullptr);
    else {
        const auto bytes = Utf8(message);
        WriteFile(output, bytes.data(), static_cast<DWORD>(bytes.size()), &count, nullptr);
    }
}
}
