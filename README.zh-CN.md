# VRC Video Proxy

[English](README.md)

给 VRChat 及其子进程提供明确的 HTTP 代理环境，让 YouTube 使用本地代理，让指定解析站和 B 站域名跳过 HTTP 代理。

这是原有 PowerShell 启动脚本的 **C++ 原生版本**。用户只需运行 `.exe`，无需 PowerShell、Python、额外安装 .NET 或管理员权限。

## 使用

1. 将 Windows x64 发布包解压到固定、可写的目录。
2. 登录 Steam，开启 FLClash / Clash 等代理客户端；需要 UU 时先开启加速。默认 HTTP/mixed 端口为 `7890`。
3. 双击 `VRCVideoProxy.exe`，核对代理地址和直连域名，点击 **保存并复制启动选项**。
4. 打开 Steam → VRChat → 属性 → 通用 → 启动选项，粘贴复制的整行内容。已有游戏参数放在 `%command%` 后面。
5. 从 Steam 正常启动 VRChat。

例如程序在 `D:\Tools\VRCVideoProxy` 时，启动选项是：

```text
"D:\Tools\VRCVideoProxy\VRCVideoProxy.exe" %command%
```

必须使用程序的**完整路径**。程序继续启动 Steam 传入的原启动器与参数，保留其原有受保护的启动链路。

更改配置后需退出并重新启动游戏；撤销时恢复之前的 Steam 启动选项，再重启游戏即可。

## 流量与作用范围

只修改本次新启动进程的环境：

| 变量 | 作用 |
|---|---|
| `HTTP_PROXY`、`HTTPS_PROXY`、`ALL_PROXY` | 指定 HTTP 代理，默认 `http://127.0.0.1:7890` |
| `NO_PROXY` | 指定跳过应用层 HTTP 代理的域名 |

程序保留其他环境变量、中文路径、原启动参数、工作目录及原启动器退出码。解析器等组件需要支持这些环境变量，才会按它们选择代理。

它不会决定全部游戏流量的线路。实时游戏 UDP 仍由系统路由、UU 和 TUN 等决定；TUN 开启时，跳过 HTTP 代理的请求仍可能被虚拟网卡接管。视频加载速度与 VRC 内 Ping 需要分别判断。

程序本身不发起网络请求，不修改注册表、全局环境、Steam 文件、游戏文件、yt-dlp、反作弊设置或代理订阅，也不收集日志和遥测。

## 配置

`VRCVideoProxy.ini` 位于程序旁边，使用 UTF-8 编码：

```ini
proxy_url=http://127.0.0.1:7890
no_proxy=localhost,127.0.0.1,::1,.ouo.chat,.bilivideo.com,.bilivideo.cn,.bilivideo.net
```

发布包的完整默认值还包含原脚本使用的其他 B 站域名。直连列表用逗号或换行分隔；使用 `.example.com` 表示域名及子域，不填写 `https://` 或 Windows 风格的 `*.example.com`。

缺少默认 INI 时使用内置值；通过 `--config` 明确指定的文件不存在时会报错。设置窗口只在点击保存按钮时写入文件。

## 开发与构建

需要 Windows 10/11 x64、CMake 3.20+、C++17 编译器。支持 MSVC 与 MinGW-w64，没有需要下载的第三方应用库。应用使用静态 C++ 运行库；发布程序仅依赖 Windows 系统 DLL，目前未进行代码签名。

MSVC 开发者命令提示符：

```bat
cmake -S . -B build -A x64 -DBUILD_TESTING=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cpack --config build\CPackConfig.cmake -C Release
```

MinGW-w64：

```bat
cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build-mingw --parallel
ctest --test-dir build-mingw --output-on-failure
cpack --config build-mingw\CPackConfig.cmake
```

ZIP 输出到对应构建目录的 `dist` 文件夹。测试会实际启动探针子进程及后代进程，检查空参数、引号、中文、空格路径、环境继承与隔离、退出码等，不启动真实游戏。

## 后续发布到 GitHub

项目包含 MIT 许可证、CI 和 Release 工作流。建立仓库并推送源码后，推送与源码版本匹配的 `vX.Y.Z` 标签，会构建 ZIP、生成 SHA-256 校验值并创建 **Release 草稿**。检查草稿后再公开发布。

CI 使用 CMake 脚本核对版本并生成校验文件，再调用 GitHub CLI 创建 Release 草稿。

见 [CONTRIBUTING.md](CONTRIBUTING.md)。本项目是独立社区工具，与 VRChat、UU 或代理客户端厂商没有隶属关系。
