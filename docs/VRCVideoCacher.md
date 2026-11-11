# VRCVideoCacher compatibility / 兼容说明

这里的第三方工具是 [EllyVR/VRCVideoCacher](https://github.com/EllyVR/VRCVideoCacher)，有时会被写成 VRCVideoCatcher。

## 两个工具如何配合

VRCVideoProxy 负责为本次启动的进程设置 HTTP_PROXY、HTTPS_PROXY、ALL_PROXY 与 NO_PROXY。VRCVideoCacher 是独立的后台应用；根据上游实现，它通过一个本地接口接收游戏的视频请求，并运行自己的下载器及缓存服务。

因此，兼容使用需要同时满足：

1. 游戏与 Cacher 的本地通信直接到本机。
2. Cacher 本身及其 yt-dlp 等子进程也获得合适的代理环境。

启动 VRChat 时设置的环境只会继续传给它的后代进程，不会倒过来修改已经由 VRCX、Steam 或桌面启动的 Cacher。不能仅凭两款应用都在运行，就认定 Cacher 已使用本启动器的分流。

## 第一步：保留本地直连地址

打开 VRCVideoProxy，点击 **添加 Cacher 直连域名**，再点击 **保存并复制启动选项**。

按钮补齐下面五种本地主机写法，并保留代理端口和现有域名：

```text
localhost
127.0.0.1
::1
[::1]
localhost.youtube.com
```

0.1.1 的新配置已包含这些地址。旧配置不会被静默覆盖，需要点击按钮或手动补充。

上游 stub 请求 `http://127.0.0.1:9696/api/getvideo`；缓存服务监听本地 9696 端口。某些配置向游戏返回 `http://localhost.youtube.com:9696/...`，因此这个域名也需跳过 HTTP 代理。它与整个 `youtube.com` 直连不是一回事：**不要为了本地缓存播放，把 youtube.com 或 .youtube.com 整体加入 NO_PROXY。**

如果你自行配置了其他本地播放主机名，需要把那个准确主机名也加入列表。NO_PROXY 只影响支持它的应用层代理选择；TUN 开启时仍需保持本地播放地址解析到回环地址。若在 Clash 中看到该本地域名被送往远端代理，可以给其添加精确的高优先级 DIRECT 规则，并核对 hosts/DNS 配置。

## 第二步：让 Cacher 单独获得代理环境

### Steam 版

假设程序放在 `D:\Tools\VRCVideoProxy`，在 Steam 的两个条目中分别填写同一行：

```text
"D:\Tools\VRCVideoProxy\VRCVideoProxy.exe" %command%
```

- VRChat → 属性 → 启动选项。
- VRCVideoCacher → 属性 → 启动选项。

保留原有参数，将它们放在 `%command%` 后面。先自行正常退出已有的 Cacher，避免新进程发现旧实例后直接退出；然后先从 Steam 启动 Cacher，待它正常就绪后再启动 VRChat。

### 独立版

让本启动器运行 Cacher 的原始 `.exe`，例如在快捷方式的“目标”中填写：

```text
"D:\Tools\VRCVideoProxy\VRCVideoProxy.exe" "D:\Tools\VRCVideoCacher\VRCVideoCacher.exe"
```

需要后台模式时，可在最后保留该版本支持的 `--nogui` 参数。不要把 VRCX 的普通自启动和新的快捷方式同时作为启动途径，以免先运行的旧实例保留另一套环境。

退出旧的 Cacher 后，用这个快捷方式启动它；VRChat 仍按 README 的 Steam 前缀方法启动。无需让 Cacher 成为 VRChat 的子进程，两边可以分别由本程序启动。

## 代理、登录态和缓存各自解决什么问题

- 出口代理及域名分流由本启动器的进程环境与代理客户端共同决定。
- Cookie、PO token、SABR、缓存和下载行为仍由 VRCVideoCacher 处理。
- 本启动器不会导入、导出或修改 Cookie，不会为代理节点改变出口信誉，也不能保证任何节点都能通过 YouTube 的检查。

如果 Cacher 的 `YtdlpAdditionalArgs` 或 `SabrAdditionalArgs` 中已有显式 `--proxy` 参数，需要核对是否与期望线路一致；这些应用自己的参数可能覆盖环境变量。不要直接清空其他已有参数。

本项目不捆绑 Cacher 的二进制文件、源码、配置或账号数据，不负责替代它的安装与恢复流程。

## 验证范围

已核对上游源码的本地接口和独立进程结构；本项目自动化测试验证了：

- 本地域名添加保留自定义端口和原有域名，重复添加不会重复写入。
- 单独作为目标启动的缓存程序探针同样获得代理与本地直连环境。
- 常规游戏启动参数、后代环境继承和父进程隔离仍通过原有测试。
- 用实际 .NET 6 HttpClient 代理选择检查补齐 IPv6 回环写法；新配置让本地接口直连，YouTube 外部地址使用代理。此检查没有发送 HTTP 请求，也未读取登录态。

这些不是对真实 Cacher 缓存、登录态或 SABR 播放的完整联调。实际采用前，至少确认一次 YouTube 首次播放、缓存命中后的再次播放和 B 站直链；本地播放地址不应经过远端代理。

## 上游依据

基于 2026-11-04 查阅的上游修订 `235d20fe50cf0dae01a8c7821e461efc39c8bcfe`：

- [stub 的 127.0.0.1:9696 请求](https://github.com/EllyVR/VRCVideoCacher/blob/235d20fe50cf0dae01a8c7821e461efc39c8bcfe/yt-dlp-stub/Program.cs)
- [缓存服务的本地监听地址](https://github.com/EllyVR/VRCVideoCacher/blob/235d20fe50cf0dae01a8c7821e461efc39c8bcfe/VRCVideoCacher/API/WebServer.cs)
- [独立进程与已有实例处理](https://github.com/EllyVR/VRCVideoCacher/blob/235d20fe50cf0dae01a8c7821e461efc39c8bcfe/VRCVideoCacher/Program.cs)
- [下载器启动子进程的方式](https://github.com/EllyVR/VRCVideoCacher/blob/235d20fe50cf0dae01a8c7821e461efc39c8bcfe/VRCVideoCacher/YTDL/VideoDownloader.cs)
