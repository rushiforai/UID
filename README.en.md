English | [中文](README.md)

# UID Filter

Author: 泷泽. Current version: 0.2.3.

Avoids detection by filtering `getpriority(PRIO_USER)` for the selected packages.

The module targets arm64, Android 13+, and Zygisk API 5, and ships a WebUI in both Chinese and English. Filtering only affects the ordinary app processes selected in the configuration; the default package names are `com.chunqiunativecheck` and `com.chunqiu.appvisibilitydemo`.

## Local build

Requires Python 3.10+ and Android NDK `27.3.13750724`. The SDK is looked up in the standard location per platform (`%LOCALAPPDATA%/Android/Sdk` on Windows, `~/Library/Android/Sdk` on macOS, `~/Android/Sdk` on Linux); you can point at it with `ANDROID_SDK_ROOT` / `ANDROID_HOME`, or set `ANDROID_NDK_HOME` to the NDK root directly. The build script selects the NDK toolchain for the current host (`windows-x86_64`, `linux-x86_64`, or `darwin-x86_64`) on its own.

```powershell
git clone https://github.com/longze777/UID-.git
Set-Location UID-
python tools/uid-seccomp-module/build.py --variant release
python tools/uid-seccomp-module/build.py --variant debug
```

This builds the module only — no Java, Gradle, or extra Python packages are needed. The Zygisk API 5 header is vendored in the source tree and its pinned SHA-256 is verified during the build.

`--variant` accepts `release` (the default: `-O2`, module version `0.2.3`) or `debug` (`-O0 -g`, module version `0.2.3-debug`, so the two packages are easy to tell apart in a module manager). Filtering behaves identically in both; only the compiler optimisation and the metadata differ.

Output files:

- `build/0.2.3/release/cq-uid-seccomp-0.2.3.zip`
- `build/0.2.3/release/cq-uid-seccomp-0.2.3.zip.sha256`
- `build/0.2.3/debug/cq-uid-seccomp-0.2.3-debug.zip`
- `build/0.2.3/debug/cq-uid-seccomp-0.2.3-debug.zip.sha256`

Use `--output <directory>` to choose a different output directory, or `--config <file>` to supply the default configuration. The first line of the configuration is `enabled=1` or `enabled=0`, followed by one full package name per line. Omitting `--module-only` also builds the module only.

## Continuous integration and releases

`.github/workflows/build.yml` installs NDK `27.3.13750724` on Linux, builds the `release` and `debug` ZIPs in parallel, and unzips each one afterwards to check `module.prop` and the `.zip.sha256` file.

- Push to `dev` / `main`, and pull requests: both variants are built and uploaded as workflow artifacts.
- Push a `v*` tag (for example `git tag v0.2.3 && git push origin v0.2.3`): a GitHub Release is created as well, with both ZIPs and their `.sha256` files attached.
- Manual run (`workflow_dispatch`): fill in `release_tag` to publish a Release from the current branch's code, or leave it empty to only build.

Publishing a Release needs the repository's `contents: write` permission; the workflow as a whole requests only `contents: read`, and the release job alone declares `contents: write`.

## Install and configure

Install the ZIP with a compatible module manager and Zygisk provider, then reboot and open the module WebUI. The panel can toggle filtering, add and remove package names, and save and apply the configuration. Applying a configuration stops the target apps whose filtering state changed, so it takes effect once they are reopened; app data is not cleared.

The action button currently targets `me.weishu.kernelsu`; the panel entry point for other managers still needs to be adapted and verified. No compatibility guarantee is made for every device or manager.

## Source layout

- `tools/uid-seccomp-module/module.cpp`, `filter.h`: the native module.
- `tools/uid-seccomp-module/panel/`: install and configuration scripts plus WebUI assets.
- `tools/uid-seccomp-module/build.py`: compilation and ZIP packaging.
- `third_party/zygisk/`: the original header, its licence notice, and provenance information.

The project remains under the [Apache-2.0 licence](LICENSE); the third-party Zygisk header keeps its original licence notice.
