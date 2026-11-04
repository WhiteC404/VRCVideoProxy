#include "core.hpp"
#include <shellapi.h>
#include <functional>
#include <iostream>
#include <stdexcept>

namespace {
void Check(bool passed, const char* message) { if (!passed) throw std::runtime_error(message); }
void Throws(const std::function<void()>& action, const char* message) {
    try { action(); } catch (const std::exception&) { return; }
    throw std::runtime_error(message);
}
std::wstring Env(const wchar_t* key) {
    const DWORD size = GetEnvironmentVariableW(key, nullptr, 0);
    if (!size) return {};
    std::wstring value(size, L'\0');
    value.resize(GetEnvironmentVariableW(key, value.data(), size));
    return value;
}
struct Temp {
    std::wstring path;
    std::vector<std::wstring> files;
    Temp() {
        wchar_t root[MAX_PATH + 1]{};
        Check(GetTempPathW(MAX_PATH, root) > 0, "GetTempPath failed");
        path = std::wstring(root) + L"VRCVP test 中文 " + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64());
        Check(CreateDirectoryW(path.c_str(), nullptr) != 0, "Cannot create isolated test directory");
    }
    std::wstring File(const std::wstring& name) { files.push_back(path + L"\\" + name); return files.back(); }
    ~Temp() {
        // Only explicitly tracked files and this newly created, empty directory.
        for (const auto& file : files) DeleteFileW(file.c_str());
        RemoveDirectoryW(path.c_str());
    }
};
vrcvp::Environment Decode(const std::vector<wchar_t>& block) {
    vrcvp::Environment result;
    for (const wchar_t* p = block.data(); *p; p += wcslen(p) + 1) {
        const std::wstring value(p);
        const auto equal = value.find(L'=', value[0] == L'=' ? 1 : 0);
        result[value.substr(0, equal)] = value.substr(equal + 1);
    }
    return result;
}
void Quoting() {
    const std::vector<std::wstring> values{L"", L"normal", L"white space", L"\t", L"\\", L"C:\\with space\\", L"one\"two", L"\\\"", L"中文✓", L"a&b|c>file;$(text)"};
    for (const auto& value : values) {
        const auto line = vrcvp::CommandLine(L"C:\\test path\\app.exe", {value, L"--tail"});
        int count = 0;
        auto* decoded = CommandLineToArgvW(line.c_str(), &count);
        const bool correct = decoded && count == 3 && decoded[1] == value && std::wstring(decoded[2]) == L"--tail";
        if (decoded) LocalFree(decoded);
        Check(correct, "Argument quoting round trip failed");
    }
}
void ConfigAndEnvironment(Temp& temp) {
    vrcvp::Config config;
    vrcvp::Validate(config);
    Check(config.proxy == L"http://127.0.0.1:7890", "Default proxy changed");
    Check(vrcvp::NormalizeNoProxy(L"localhost\r\n .bilivideo.com, ::1") == L"localhost,.bilivideo.com,::1", "Domain normalization failed");
    Throws([] { vrcvp::NormalizeNoProxy(L"https://example.com/"); }, "URL accepted as a domain");
    Throws([] { vrcvp::NormalizeNoProxy(L"*.example.com"); }, "Unsupported wildcard accepted");
    Throws([] { vrcvp::Config c; c.proxy = L"http://localhost:0"; vrcvp::Validate(c); }, "Port zero accepted");
    Throws([] { vrcvp::Config c; c.proxy = L"https://localhost:7890"; vrcvp::Validate(c); }, "Unsupported proxy scheme accepted");
    Throws([] { vrcvp::Config c; c.proxy = L"http://user:password@localhost:7890"; vrcvp::Validate(c); }, "Credentials accepted");
    Throws([] { vrcvp::Config c; c.proxy = L"http://localhost:7890/path"; vrcvp::Validate(c); }, "Proxy path accepted");
    const auto path = temp.File(L"设置 with space.ini");
    vrcvp::WriteConfig(path, config);
    config.proxy = L"http://[::1]:17890";
    config.noProxy = L"localhost,.bilivideo.com";
    vrcvp::WriteConfig(path, config);
    const auto decoded = vrcvp::ReadConfig(path);
    Check(decoded.proxy == config.proxy && decoded.noProxy == config.noProxy, "Atomic config round trip failed");
    vrcvp::Environment original{{L"https_proxy",L"old"},{L"HTTP_PROXY",L"old"},{L"NO_PROXY",L"old"},{L"=C:",L"C:\\drive cwd"},{L"KEEP",L"中文"}};
    const auto originalCopy = original;
    const auto block = vrcvp::ChildEnvironment(original, config);
    Check(block.size() >= 2 && block.back() == 0 && block[block.size() - 2] == 0, "Environment is not double terminated");
    const auto child = Decode(block);
    Check(child.at(L"HTTPS_PROXY") == config.proxy && child.at(L"http_proxy") == config.proxy, "Case-insensitive proxy replacement failed");
    Check(child.at(L"KEEP") == L"中文" && child.at(L"=C:") == L"C:\\drive cwd", "Inherited variables were lost");
    Check(child.size() == original.size() + 1, "Duplicate proxy environment keys");
    Check(original == originalCopy, "Parent environment was changed");
}
void Parsing() {
    Check(vrcvp::Directory(L"C:\\launcher.exe") == L"C:\\", "Drive-root working directory is not absolute");
    const auto options = vrcvp::ParseOptions({L"--no-ui",L"launcher.exe",L"--version",L""}, L"settings.ini");
    Check(options.launcher == L"launcher.exe" && options.arguments == std::vector<std::wstring>{L"--version",L""}, "Child arguments were consumed as wrapper options");
    Check(vrcvp::ParseOptions({}, L"settings.ini").settings, "Double-click does not open settings");
    Throws([] { vrcvp::ParseOptions({L"--config"}, L"settings.ini"); }, "Missing configuration argument accepted");
    Throws([] { vrcvp::ParseOptions({L"--version",L"--settings"}, L"settings.ini"); }, "Conflicting actions accepted");
    Check(vrcvp::SteamOption(L"C:\\tool path\\VRCVideoProxy.exe", options) == L"\"C:\\tool path\\VRCVideoProxy.exe\" %command%", "Steam path is not quoted");
}
void Integration(Temp& temp, const std::wstring& launcherSource, const std::wstring& probeSource) {
    const auto launcher = temp.File(L"视频 Proxy launcher.exe");
    const auto probe = temp.File(L"child probe with spaces.exe");
    Check(CopyFileW(launcherSource.c_str(), launcher.c_str(), TRUE) != 0, "Cannot copy launcher into Unicode path");
    Check(CopyFileW(probeSource.c_str(), probe.c_str(), TRUE) != 0, "Cannot copy probe into Unicode path");
    vrcvp::Config config;
    config.proxy = L"http://127.0.0.1:17890";
    config.noProxy = L"localhost,.bilivideo.com";
    const auto path = temp.File(L"custom config 中文.ini");
    vrcvp::WriteConfig(path, config);
    SetEnvironmentVariableW(L"https_proxy", L"parent-sentinel");
    SetEnvironmentVariableW(L"VRCVP_TEST_MARKER", L"keep-中文-✓");
    const auto environmentBefore = vrcvp::InheritedEnvironment();
    std::vector<std::wstring> args{L"--no-ui",L"--config",path,probe,L"--version",L"argument with spaces",L"",L"C:\\path with space\\",L"literal\"quote",L"参数✓",L"url?x=1&y=$(literal)%VALUE%"};
    // Outer launch supplies unrelated defaults; the actual wrapper must read
    // its custom file and replace only its own child's proxy environment.
    Check(vrcvp::Launch(launcher,args,vrcvp::Config{}) == 42, "Real process, CRT argv, working directory, environment or descendant inheritance failed");
    Check(vrcvp::InheritedEnvironment() == environmentBefore, "Launching modified the calling process environment");
    Check(vrcvp::Launch(launcher,{L"--no-ui",L"--config",temp.path + L"\\missing.ini",probe},config) == 2, "Missing custom config did not fail cleanly");
    Throws([&] { vrcvp::Launch(temp.path + L"\\missing.exe",{},config); }, "Missing executable accepted");
    Check(Env(L"https_proxy") == L"parent-sentinel", "Proxy value leaked into parent");
}
}
int wmain(int argc, wchar_t** argv) {
    try {
        Check(argc == 3, "Expected launcher and probe executable paths");
        Temp temp;
        Quoting(); std::cout << "PASS Windows argument quoting\n";
        ConfigAndEnvironment(temp); std::cout << "PASS configuration and isolated Unicode environment\n";
        Parsing(); std::cout << "PASS Steam option and command-line parsing\n";
        Integration(temp, argv[1], argv[2]); std::cout << "PASS real child process, empty/Unicode/special arguments, descendant environment, working directory, exit code and parent isolation\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << "FAIL " << error.what() << '\n'; return 1; }
}
