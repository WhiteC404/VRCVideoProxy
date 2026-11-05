# VRC Video Proxy

**v0.1.0 · Windows 10 / 11 x64 · C++17**

為 VRChat 及其子程序單獨設定 HTTP 代理環境。支援代理環境變數的影片元件可使用本地代理，指定的 B 站與解析站域名則跳過 HTTP 代理。

適合已使用 FLClash、Clash 等代理客戶端，希望同時播放 YouTube 與 B 站影片的 Windows 使用者。

## 功能

- 原生 Windows `.exe`，使用時無需 PowerShell、Python、額外安裝 .NET 或管理員權限。
- 提供圖形設定介面，可修改 HTTP 代理地址與直連域名。
- 產生完整路徑的 Steam 啟動選項，並支援一鍵複製。
- 使用 Steam 傳入的原始啟動器與參數，保留原有啟動流程。
- 僅為本次啟動的程序及其後代提供代理環境，保留其他環境變數。
- 支援中文及含空格的路徑，並保留空參數、引號和特殊字元。

## 使用前準備

| 項目 | 要求 |
| --- | --- |
| 作業系統 | Windows 10 / 11，x64 |
| 遊戲 | Steam 版 VRChat |
| 代理客戶端 | 已啟動，且提供可用的 HTTP 或 mixed 代理端口 |
| 預設代理地址 | `http://127.0.0.1:7890` |

本工具使用你已有的代理服務。若客戶端端口不是 `7890`，請在設定介面中填寫實際端口。

需要遊戲加速器時，可依原有方式啟用 UU 等工具；即時遊戲通信的路由仍由系統、加速器與 TUN 等共同決定。

## 快速開始

### 1. 下載並解壓縮

從本儲存庫的 **Releases** 頁面下載：

```text
VRCVideoProxy-0.1.0-windows-x64.zip
```

將完整內容解壓縮到固定、可寫的資料夾，例如：

```text
D:\Tools\VRCVideoProxy\
```

完成 Steam 設定後若移動程式，需重新複製啟動選項。

### 2. 設定代理

啟動代理客戶端，登入 Steam，再雙擊 `VRCVideoProxy.exe`。

在設定視窗中：

1. 核對「代理地址」，預設為 `http://127.0.0.1:7890`。
2. 按需調整「直连域名」列表。
3. 點擊 **保存并复制启动选项**。

程式會將設定寫入同目錄的 `VRCVideoProxy.ini`，並複製對應的 Steam 啟動選項。

### 3. 加入 Steam 啟動選項

開啟 **Steam → 收藏庫 → VRChat → 內容／屬性 → 一般／通用 → 啟動選項**，貼上剛才複製的整行內容。

若程式位於上述範例路徑，啟動選項為：

```text
"D:\Tools\VRCVideoProxy\VRCVideoProxy.exe" %command%
```

若已有遊戲參數，先保留原內容，再將參數放在 `%command%` 後面，例如：

```text
"D:\Tools\VRCVideoProxy\VRCVideoProxy.exe" %command% --no-vr
```

請保留路徑兩側的雙引號與 `%command%`。使用程式的完整路徑，避免 Steam 找不到可執行檔。

### 4. 從 Steam 啟動

正常啟動 VRChat，分別確認 YouTube 和 B 站影片的播放情況。

設定只對新啟動的程序生效。修改代理地址、直連列表或啟動選項後，需要退出並重新啟動 VRChat。

## 工作方式與流量範圍

程式讀取目前的環境變數，建立新的子程序環境，設定下列四個變數後，啟動 Steam 提供的原始程式：

| 變數 | 用途 |
| --- | --- |
| `HTTP_PROXY` | 為支援此變數的元件指定 HTTP 代理 |
| `HTTPS_PROXY` | 為支援此變數的元件指定 HTTPS 請求所用的代理 |
| `ALL_PROXY` | 為支援此變數的元件提供代理設定 |
| `NO_PROXY` | 指定跳過應用層 HTTP 代理的主機或域名 |

`HTTPS_PROXY` 的值仍可使用 `http://127.0.0.1:7890`，因為這裡填寫的是本地 HTTP 代理服務的地址。

**這些設定不代表全部 VRChat 流量都會經過同一條線路。**

- 影片元件是否採用這些設定，取決於它是否支援代理環境變數。
- Photon 等即時遊戲通信的 UDP 路由，仍由系統路由、加速器及 TUN 等處理。
- TUN 開啟時，跳過 HTTP 代理的流量仍可能進入虛擬網卡。
- 由桌面、VRCX 或其他程序獨立啟動的工具，不會因為 VRChat 的啟動選項而取得這套環境。

程式本身不發起網路請求，也不修改註冊表、全域環境變數、Steam 檔案、遊戲程式、yt-dlp、反作弊設定或代理訂閱；不收集日誌或傳送遙測。

## 設定檔

`VRCVideoProxy.ini` 使用 UTF-8 編碼，與 `.exe` 放在同一個資料夾。0.1.0 的完整預設值為：

```ini
proxy_url=http://127.0.0.1:7890
no_proxy=localhost,127.0.0.1,::1,[::1],ouo.chat,.ouo.chat,bilivideo.com,.bilivideo.com,bilivideo.cn,.bilivideo.cn,bilivideo.net,.bilivideo.net,bilibili.com,.bilibili.com,biliapi.com,.biliapi.com,biliapi.net,.biliapi.net,hdslb.com,.hdslb.com
```

### 代理地址

`proxy_url` 接受 HTTP 代理地址，例如 `http://127.0.0.1:7890`。0.1.0 的設定介面不接受 SOCKS 地址，或包含帳號、密碼、路徑及查詢參數的代理 URL。

### 直連域名

- 設定介面可以每行輸入一個域名，或使用逗號分隔。
- **INI 檔案中的 `no_proxy` 值需放在同一行。**
- 建議同時列出 `example.com` 與 `.example.com`，兼顧不同元件對主域名和子域名的匹配差異。
- 填寫主機或域名，不填完整 URL，也不使用 `*.example.com` 形式。

讀入舊設定時，程式會為 `::1` 或 `[::1]` 補齊另一種回環地址寫法，保留原有代理端口及其他域名。

缺少預設 INI 時，程式使用內建設定；透過 `--config` 明確指定的檔案不存在時，則會回報錯誤。設定視窗只在點擊保存按鈕時寫入檔案。

## 命令列參數

| 參數 | 說明 |
| --- | --- |
| 無參數／`--settings` | 開啟設定視窗 |
| `--version` | 輸出程式版本 |
| `--help` | 輸出參數說明 |
| `--check-config` | 驗證並輸出有效設定 |
| `--print-steam-option` | 輸出完整路徑的 Steam 啟動選項 |
| `--config FILE` | 指定另一份 UTF-8 設定檔 |
| `--no-ui` | 發生錯誤時只向標準錯誤輸出，不顯示訊息視窗 |
| `-- EXE [ARGS...]` | 明確分隔原始啟動器與其參數 |

使用自訂設定檔產生 Steam 啟動選項：

```text
VRCVideoProxy.exe --config "D:\Settings\vrc.ini" --print-steam-option
```

程式在遇到原始可執行檔後，會把其後的參數原樣交給該程式。一般遊玩建議使用 Steam 的 `%command%`，讓 Steam 提供原始啟動器及對應模式的參數。

## 常見問題

### Steam 提示「遊戲可執行檔缺失」

先檢查啟動選項中的 `VRCVideoProxy.exe` 是否使用完整路徑，路徑是否仍然存在，以及雙引號和 `%command%` 是否完整。移動程式後，請重新開啟設定視窗並複製啟動選項。

### 影片仍然無法播放

確認本地代理端口可用，並分別檢查代理規則、節點連通性及影片來源。YouTube 的登入或反機器人驗證、來源失效等問題，仍可能影響播放；本工具不提供 Cookie 管理，也不保證特定節點一定可用。

### 影片加快了，遊戲 Ping 仍然很高

影片解析與即時遊戲通信使用不同的連線。請檢查實例區域、遊戲加速器線路與 TUN 接管情況。`NO_PROXY` 表示跳過應用層 HTTP 代理，不能據此判斷遊戲 UDP 是否繞過 TUN 或加速器。

### 修改設定後沒有變化

確認已保存設定，並退出後重新啟動 VRChat。已經運行的程序不會套用新的環境變數。

### 如何撤銷

在 Steam 恢復原本的啟動選項，再重新啟動遊戲。程式沒有寫入需要另外清理的全域代理環境變數。

## 從原始碼建置

需要 Windows x64、CMake 3.20 以上，以及支援 C++17 的 MSVC 或 MinGW-w64。應用程式不需要下載第三方應用程式庫。

### MSVC

在 Visual Studio 的開發者命令提示字元中執行：

```bat
cmake -S . -B build -A x64 -DBUILD_TESTING=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cpack --config build\CPackConfig.cmake -C Release
```

### MinGW-w64

確認 `g++`、`windres`、`mingw32-make` 與 CMake 已在 PATH 中，再執行：

```bat
cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build-mingw --parallel
ctest --test-dir build-mingw --output-on-failure
cpack --config build-mingw\CPackConfig.cmake
```

ZIP 套件輸出至相應建置資料夾內的 `dist` 目錄。應用程式靜態連結 C++ 執行階段，發行檔僅依賴 Windows 系統 DLL。

## 測試與版本範圍

0.1.0 已在本機以 MinGW-w64 建置並通過原生程序測試，涵蓋：

- 中文、空格路徑與空參數。
- 引號、反斜線及特殊字元的傳遞。
- 代理環境覆寫、後代程序繼承與父程序隔離。
- 原始啟動器的工作目錄及退出碼。
- 設定驗證，以及設定介面的保存與複製操作。

這些檢查不等於每種代理、加速器與遊戲環境均已完成實機驗證。VRCVideoCacher 等獨立快取工具的整合不列為本版已驗證功能；請依自己的環境確認實際影片播放情況。

0.1.0 發行檔尚未進行數位簽章。儲存庫提供的 GitHub Actions 工作流程需要實際執行，才能確認遠端 MSVC 建置是否通過。

## 發佈

儲存庫包含 Windows 建置／測試與 Release 草稿工作流程。推送與原始碼版本一致的 `v0.1.0` 標籤後，Release 工作流程會建置套件、產生 SHA-256 校驗值，並建立 GitHub Release 草稿。

檢查草稿內容後再公開發佈。CI 使用 CMake 腳本核對版本與產生校驗檔，再呼叫 GitHub CLI 建立 Release 草稿。

開發與發佈細節見 [CONTRIBUTING.md](CONTRIBUTING.md)。

## 授權

本專案採用 [MIT License](LICENSE)。建置工具及執行階段相關資訊見 [THIRD_PARTY.md](THIRD_PARTY.md)。

這是獨立社群工具，與 VRChat、Steam、UU 或任何代理客戶端廠商沒有隸屬關係。
