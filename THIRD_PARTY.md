# Dependencies and build tools

The application uses C++17 and Windows system APIs. It does not bundle yt-dlp, VRChat,
Easy Anti-Cheat, a proxy engine, an accelerator, or any subscription configuration.

The Windows libraries used at runtime are system components: Kernel32, User32,
Gdi32, Shell32, Comctl32, WinHTTP and the system C runtime.

The GitHub release workflow uses the Microsoft C++ toolchain with the static CRT.
Development builds made with MinGW-w64 statically link GCC/libstdc++ runtime code;
the GCC Runtime Library Exception permits eligible target code to be distributed
under terms of its choice:

- https://www.gnu.org/licenses/gcc-exception-3.1.html
- https://gcc.gnu.org/onlinedocs/libstdc++/manual/license.html
- https://www.mingw-w64.org/credits/

The source code of this project is under the MIT license in LICENSE. Windows is a
Microsoft product. VRChat, Steam, Easy Anti-Cheat and proxy/accelerator product names
belong to their respective owners. This is an independent community utility.
