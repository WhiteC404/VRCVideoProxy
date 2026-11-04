"""CI packaging helper. The application and its native tests do not use Python."""
from pathlib import Path
import argparse
import hashlib
import os
import re
import subprocess


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--publish", action="store_true", help="Create a draft GitHub Release")
    parser.add_argument("--dry-run", action="store_true", help="Validate the publish command without uploading")
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    source = (root / "CMakeLists.txt").read_text(encoding="utf-8")
    version = re.search(r"project\(VRCVideoProxy VERSION (\d+\.\d+\.\d+) ", source).group(1)
    expected_resource = version.replace(".", ",") + ",0"
    resource = (root / "src" / "resources.rc").read_text(encoding="utf-8")
    manifest = (root / "src" / "app.manifest").read_text(encoding="utf-8")
    if f"FILEVERSION {expected_resource}" not in resource or f'assemblyIdentity version="{version}.0"' not in manifest:
        raise SystemExit("CMake, resource and manifest versions must match before packaging a release.")
    package = root / "build" / "dist" / f"VRCVideoProxy-{version}-windows-x64.zip"
    if not package.is_file():
        raise SystemExit(f"Package not found: {package.name}; run CPack first.")
    checksum = package.with_name("SHA256SUMS.txt")
    checksum.write_text(f"{hashlib.sha256(package.read_bytes()).hexdigest()}  {package.name}\n", encoding="ascii")
    print(f"Prepared {package.name} and {checksum.name}")
    if not (args.publish or args.dry_run):
        return
    tag = os.environ.get("GITHUB_REF_NAME", "")
    if not re.fullmatch(r"v\d+\.\d+\.\d+", tag) or tag != f"v{version}":
        raise SystemExit("Release tag must exactly match the source version, e.g. v0.1.0.")
    command = ["gh", "release", "create", tag, str(package), str(checksum), "--draft", "--verify-tag",
               "--title", f"VRC Video Proxy {tag}", "--generate-notes"]
    if args.dry_run:
        print(f"Validated draft release command for {tag}; no upload performed.")
    else:
        subprocess.run(command, check=True, cwd=root)


if __name__ == "__main__":
    main()
