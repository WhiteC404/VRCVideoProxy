#include "core.hpp"
#include <array>
#include <string>

namespace {
std::wstring Env(const wchar_t* key) {
    const DWORD length = GetEnvironmentVariableW(key, nullptr, 0);
    if (!length) return {};
    std::wstring result(length, L'\0');
    const DWORD count = GetEnvironmentVariableW(key, result.data(), length);
    result.resize(count);
    return result;
}
bool EnvironmentMatches() {
    return Env(L"HTTP_PROXY") == L"http://127.0.0.1:17890" &&
           Env(L"https_proxy") == L"http://127.0.0.1:17890" &&
           Env(L"ALL_PROXY") == L"http://127.0.0.1:17890" &&
           Env(L"no_proxy") == L"localhost,.bilivideo.com" &&
           Env(L"VRCVP_TEST_MARKER") == L"keep-中文-✓";
}
int ConsoleHost(const std::wstring& launcher) {
    const HANDLE screen = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(screen, &info)) return 82;
    auto run = [&](const std::vector<std::wstring>& args, HANDLE redirected, DWORD expectedCode, const std::wstring& expected) {
        DWORD count = 0;
        const DWORD length = 8192;
        FillConsoleOutputCharacterW(screen, L' ', length, {0, 0}, &count);
        SetConsoleCursorPosition(screen, {0, 0});
        auto command = vrcvp::CommandLine(launcher, args);
        STARTUPINFOW start{}; start.cb = sizeof(start);
        start.dwFlags = STARTF_USESTDHANDLES;
        start.hStdOutput = redirected;
        PROCESS_INFORMATION process{};
        if (!CreateProcessW(launcher.c_str(), command.data(), nullptr, nullptr, redirected != nullptr, 0, nullptr, nullptr, &start, &process)) return false;
        CloseHandle(process.hThread);
        const auto wait = WaitForSingleObject(process.hProcess, 5000);
        if (wait != WAIT_OBJECT_0) TerminateProcess(process.hProcess, 98);
        DWORD code = 0;
        GetExitCodeProcess(process.hProcess, &code);
        CloseHandle(process.hProcess);
        if (wait != WAIT_OBJECT_0 || code != expectedCode) return false;
        if (redirected) {
            SetFilePointer(redirected, 0, nullptr, FILE_BEGIN);
            char text[4096]{};
            if (!ReadFile(redirected, text, sizeof(text), &count, nullptr)) return false;
            return std::string(text, count).find(vrcvp::Utf8(expected)) != std::string::npos;
        }
        std::wstring text(length, L'\0');
        if (!ReadConsoleOutputCharacterW(screen, text.data(), length, {0, 0}, &count)) return false;
        return text.find(expected) != std::wstring::npos;
    };
    if (!run({L"--version"}, nullptr, 0, L"VRCVideoProxy " VRCVP_VERSION)) return 83;
    if (!run({L"--help"}, nullptr, 0, L"--print-steam-option")) return 84;
    if (!run({L"--check-config"}, nullptr, 0, L"proxy_url=")) return 85;
    if (!run({L"--print-steam-option"}, nullptr, 0, L"%command%")) return 86;
    if (!run({L"--no-ui", L"--invalid-option"}, nullptr, 2, L"Unknown launcher option")) return 87;
    wchar_t temp[MAX_PATH + 1]{}, file[MAX_PATH + 1]{};
    if (!GetTempPathW(MAX_PATH, temp) || !GetTempFileNameW(temp, L"vcp", 0, file)) return 88;
    SECURITY_ATTRIBUTES security{sizeof(security), nullptr, TRUE};
    HANDLE output = CreateFileW(file, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        &security, OPEN_EXISTING, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (output == INVALID_HANDLE_VALUE) { DeleteFileW(file); return 89; }
    const bool ok = run({L"--version"}, output, 0, L"VRCVideoProxy " VRCVP_VERSION);
    CloseHandle(output);
    return ok ? 0 : 90;
}
}
int wmain(int argc, wchar_t** argv) {
    if (argc == 3 && std::wstring(argv[1]) == L"--console-host") return ConsoleHost(argv[2]);
    if (argc > 1 && std::wstring(argv[1]) == L"release") {
        std::wstring arguments;
        for (int i = 1; i < argc; ++i) {
            if (i > 1) arguments += L'\n';
            arguments += argv[i];
        }
        const auto bytes = vrcvp::Utf8(arguments);
        const auto record = Env(L"VRCVP_TEST_RECORD");
        HANDLE file = CreateFileW(record.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return 80;
        DWORD written = 0;
        const bool ok = WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) && written == bytes.size();
        CloseHandle(file);
        if (!ok) return 81;
        return Env(L"VRCVP_TEST_FAIL") == L"1" ? 17 : 0;
    }
    if (argc == 2 && std::wstring(argv[1]) == L"--descendant") return EnvironmentMatches() ? 42 : 71;
    if (argc == 2 && std::wstring(argv[1]) == L"--cacher") {
        const auto list = L"," + Env(L"NO_PROXY") + L",";
        const bool ready = Env(L"HTTP_PROXY") == L"http://127.0.0.1:17890" &&
            Env(L"HTTPS_PROXY") == L"http://127.0.0.1:17890" &&
            list.find(L",localhost.youtube.com,") != std::wstring::npos &&
            list.find(L",127.0.0.1,") != std::wstring::npos &&
            list.find(L",::1,") != std::wstring::npos &&
            list.find(L",[::1],") != std::wstring::npos &&
            list.find(L",localhost,") != std::wstring::npos;
        return ready ? 43 : 77;
    }
    const std::array<std::wstring, 7> expected{L"--version", L"argument with spaces", L"", L"C:\\path with space\\", L"literal\"quote", L"参数✓", L"url?x=1&y=$(literal)%VALUE%"};
    if (argc != static_cast<int>(expected.size()) + 1) return 72;
    for (size_t i = 0; i < expected.size(); ++i) if (argv[i + 1] != expected[i]) return 73;
    if (!EnvironmentMatches()) return 74;
    std::wstring cwd(32768, L'\0');
    cwd.resize(GetCurrentDirectoryW(static_cast<DWORD>(cwd.size()), cwd.data()));
    if (_wcsicmp(cwd.c_str(), vrcvp::Directory(vrcvp::ExePath()).c_str()) != 0) return 75;
    auto command = vrcvp::CommandLine(vrcvp::ExePath(), {L"--descendant"});
    STARTUPINFOW start{}; start.cb = sizeof(start);
    PROCESS_INFORMATION process{};
    // No environment block: this descendant must inherit the launcher's values.
    if (!CreateProcessW(vrcvp::ExePath().c_str(), command.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &start, &process)) return 76;
    CloseHandle(process.hThread);
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(process.hProcess, &code);
    CloseHandle(process.hProcess);
    return static_cast<int>(code);
}
