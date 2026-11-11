# Changelog

## 0.1.1

- Add the VRCVideoCacher local playback alias `localhost.youtube.com` to new configurations.
- Include both `::1` and `[::1]` loopback forms for clients with different NO_PROXY matching behavior.
- Add a settings button to merge Cacher's local direct hosts into an existing list while preserving custom ports and domains.
- Document wrapping the independently started Cacher process as well as VRChat, including Steam and standalone installations.
- Add environment-contract coverage for an independently launched cache-service probe. Live Cacher/cache/SABR playback remains a separate verification step.
- Rebase the 0.1.1 changes onto the published main branch so the release keeps the parent-console output fix, the `::1`/`[::1]` alias normalization and the CMake release helper.
- Document that in-game players may use the Windows system proxy instead of these variables, and that `vrchat://` / VRCX direct launches bypass the Steam launch option.

## 0.1.0

- Native C++ Windows replacement for the process-scoped proxy PowerShell launcher.
- Settings UI, configuration, full-path Steam launch options and native process tests.
