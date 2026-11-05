#pragma once
#include <windows.h>
#include <map>
#include <string>
#include <vector>

namespace vrcvp {
struct Config {
    std::wstring proxy = L"http://127.0.0.1:7890";
    std::wstring noProxy = L"localhost,127.0.0.1,::1,[::1],ouo.chat,.ouo.chat,bilivideo.com,.bilivideo.com,bilivideo.cn,.bilivideo.cn,bilivideo.net,.bilivideo.net,bilibili.com,.bilibili.com,biliapi.com,.biliapi.com,biliapi.net,.biliapi.net,hdslb.com,.hdslb.com";
};
struct IgnoreCase {
    bool operator()(const std::wstring& a, const std::wstring& b) const;
};
using Environment = std::map<std::wstring, std::wstring, IgnoreCase>;
struct Options {
    std::wstring configPath;
    std::wstring launcher;
    std::vector<std::wstring> arguments;
    bool settings = false;
    bool check = false;
    bool printSteam = false;
    bool help = false;
    bool version = false;
    bool noUi = false;
    bool customConfig = false;
};
std::string Utf8(const std::wstring& text);
std::wstring Wide(const std::string& text);
std::wstring ErrorText(DWORD code);
std::wstring ExePath();
std::wstring AbsolutePath(const std::wstring& path);
std::wstring Directory(const std::wstring& path);
std::wstring QuoteArgument(const std::wstring& argument);
std::wstring CommandLine(const std::wstring& executable, const std::vector<std::wstring>& args);
std::wstring NormalizeNoProxy(const std::wstring& text);
void Validate(Config& config);
Config ReadConfig(const std::wstring& path);
void WriteConfig(const std::wstring& path, const Config& config);
Environment InheritedEnvironment();
std::vector<wchar_t> ChildEnvironment(Environment environment, const Config& config);
DWORD Launch(const std::wstring& executable, const std::vector<std::wstring>& args, const Config& config);
Options ParseOptions(const std::vector<std::wstring>& args, const std::wstring& defaultConfig);
std::wstring SteamOption(const std::wstring& executable, const Options& options);
void WriteOutput(const std::wstring& message, bool error = false);
int Settings(const std::wstring& executable, const Options& options, const Config& initial);
}
