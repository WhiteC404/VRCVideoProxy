# Development and release

Use the build and native tests described in README.md. Keep proxy changes confined
to the environment block supplied to CreateProcessW. Preserve the original Steam
launcher, its arguments and the rest of the parent's environment.

Do not add personal test logs, subscription URLs, API secrets, cookies or machine
paths to the repository. The package is assembled from an explicit install list;
probe/test executables are not installed.

## First GitHub publication

1. Review the source and MIT license and choose a repository name, for example
   `vrc-video-proxy`.
2. Create an empty GitHub repository under your own account or organization.
3. Commit the source, add that repository as `origin`, and push `main`.
4. Check the Windows CI result. This includes the first remote MSVC build; a local
   MinGW build does not prove the GitHub runner build has succeeded.
5. Verify the packaged executable in Steam. Update the three version locations
   together: CMakeLists.txt, src/resources.rc, src/app.manifest.
6. For version 0.1.1, create and push `v0.1.1`. The release job validates the tag
   against the source version before uploading a draft release.
7. Review the draft's assets and notes, then publish it on GitHub.

Do not overwrite an existing published tag for a new binary. Bump the version and
release a new tag instead. CI builds MSVC; the initial locally produced package
may use MinGW-w64, so hashes are not expected to match across toolchains.

The workflow's release token is the repository's standard GITHUB_TOKEN with
contents:write permission scoped to the release job. No token is stored in source.

## Before releasing

- Run native tests and inspect the packaged files.
- Confirm the Steam launch option uses the native executable's full path.
- Confirm Bilibili and YouTube work with a real proxy client and the protected
  game launcher. Treat observed speedups as setup-specific.
- Note that the application does not govern all game traffic or the accelerator's
  routing. Keep this distinction in issue reports and release notes.
