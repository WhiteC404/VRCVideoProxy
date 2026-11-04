#include "core.hpp"
#include <shellapi.h>
#include <algorithm>
#include <stdexcept>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    bool noUi = false;
    try {
        int count = 0;
        wchar_t** raw = CommandLineToArgvW(GetCommandLineW(), &count);
        if (!raw) throw std::runtime_error("Cannot read command-line arguments.");
        std::vector<std::wstring> args;
        for (int i = 1; i < count; ++i) args.emplace_back(raw[i]);
        LocalFree(raw);
        noUi = std::find(args.begin(), args.end(), L"--no-ui") != args.end();
        const auto executable = vrcvp::ExePath();
        const auto options = vrcvp::ParseOptions(args, vrcvp::Directory(executable) + L"\\VRCVideoProxy.ini");
        if (options.version) { vrcvp::WriteOutput(L"VRCVideoProxy " VRCVP_VERSION L"\n"); return 0; }
        if (options.help) {
            vrcvp::WriteOutput(
                L"VRCVideoProxy " VRCVP_VERSION L" - Windows per-launch HTTP proxy environment\n"
                L"Steam: \"absolute-path\\VRCVideoProxy.exe\" %command%\n"
                L"No arguments / --settings: open settings and copy the Steam launch option.\n"
                L"--config FILE: use a UTF-8 key=value configuration.\n"
                L"--check-config: validate and print the effective configuration.\n"
                L"--print-steam-option: print the full executable path for Steam.\n"
                L"--no-ui: report errors to stderr without a message box.\n"
                L"-- EXE [ARGS...]: explicitly separate the original launcher and arguments.\n");
            return 0;
        }
        if (options.printSteam) { vrcvp::WriteOutput(vrcvp::SteamOption(executable, options) + L"\n"); return 0; }
        if (options.customConfig && !options.settings && GetFileAttributesW(options.configPath.c_str()) == INVALID_FILE_ATTRIBUTES)
            throw std::runtime_error("The explicitly selected configuration file does not exist.");
        vrcvp::Config config;
        try { config = vrcvp::ReadConfig(options.configPath); }
        catch (const std::exception& error) {
            if (!options.settings || options.noUi) throw;
            const auto warning = L"无法读取配置，设置窗口将显示默认值。保存前请核对。\n\n" + vrcvp::Wide(error.what());
            MessageBoxW(nullptr, warning.c_str(), L"VRC Video Proxy", MB_OK | MB_ICONWARNING);
        }
        if (options.check) {
            vrcvp::WriteOutput(L"proxy_url=" + config.proxy + L"\nno_proxy=" + config.noProxy + L"\n");
            return 0;
        }
        if (options.settings) return vrcvp::Settings(executable, options, config);
        return static_cast<int>(vrcvp::Launch(options.launcher, options.arguments, config));
    } catch (const std::exception& error) {
        const auto text = L"VRC Video Proxy\n\n" + vrcvp::Wide(error.what());
        vrcvp::WriteOutput(text + L"\n", true);
        if (!noUi) MessageBoxW(nullptr, text.c_str(), L"VRC Video Proxy — 启动失败", MB_OK | MB_ICONERROR);
        return 2;
    }
}
