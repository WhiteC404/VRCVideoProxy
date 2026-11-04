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
}
int wmain(int argc, wchar_t** argv) {
    if (argc == 2 && std::wstring(argv[1]) == L"--descendant") return EnvironmentMatches() ? 42 : 71;
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
