[English](README.en.md) | 中文

# UID Filter / UID 过滤

作者：泷泽。当前版本：0.2.3。

通过过滤指定包的`getpriority(PRIO_USER)`  做到规避检测。

模块面向 arm64、Android 13+、Zygisk API 5，包含中英文 WebUI。过滤仅作用于配置选中的普通应用进程；默认包名为 `com.chunqiunativecheck` 和 `com.chunqiu.appvisibilitydemo`。

## 本地构建

需要 Python 3.10+ 和 Android NDK `27.3.13750724`。默认从各平台的标准位置查找 SDK（Windows 为 `%LOCALAPPDATA%/Android/Sdk`，macOS 为 `~/Library/Android/Sdk`，Linux 为 `~/Android/Sdk`）；可用 `ANDROID_SDK_ROOT` / `ANDROID_HOME` 指定 SDK，或用 `ANDROID_NDK_HOME` 直接指定 NDK 根目录。构建脚本会自动选择当前系统的 NDK 工具链（`windows-x86_64`、`linux-x86_64` 或 `darwin-x86_64`）。

```powershell
git clone https://github.com/longze777/UID-.git
Set-Location UID-
python tools/uid-seccomp-module/build.py --variant release
python tools/uid-seccomp-module/build.py --variant debug
```

只构建模块，无需 Java、Gradle 或额外的 Python 包。Zygisk API 5 头文件已随源码提供，构建时会校验其固定 SHA-256。

`--variant` 可取 `release`（默认，`-O2`，模块版本为 `0.2.3`）或 `debug`（`-O0 -g`，模块版本为 `0.2.3-debug`，便于在模块管理器里区分）。两种变体的过滤行为完全一致，仅编译优化与元数据不同。

输出文件：

- `build/0.2.3/release/cq-uid-seccomp-0.2.3.zip`
- `build/0.2.3/release/cq-uid-seccomp-0.2.3.zip.sha256`
- `build/0.2.3/debug/cq-uid-seccomp-0.2.3-debug.zip`
- `build/0.2.3/debug/cq-uid-seccomp-0.2.3-debug.zip.sha256`

可用 `--output <目录>` 指定输出目录，或用 `--config <文件>` 指定默认配置。配置首行为 `enabled=1` 或 `enabled=0`，随后每行一个完整包名。省略 `--module-only` 同样只构建模块。

## 持续集成与发布

`.github/workflows/build.yml` 会在 Linux 上安装 NDK `27.3.13750724`，并行构建 `release` 与 `debug` 两个 ZIP，并在打包后解压校验 `module.prop` 与 `.zip.sha256`。

- 推送到 `dev` / `main`、以及 Pull Request：构建两个变体并上传为构建产物（Artifacts）。
- 推送 `v*` 标签（例如 `git tag v0.2.3 && git push origin v0.2.3`）：额外创建 GitHub Release，并附上两个 ZIP 及其 `.sha256`。
- 手动运行（workflow_dispatch）：填写 `release_tag` 即可用当前分支的代码发布 Release，留空则只构建。

发布 Release 需要仓库的 `contents: write` 权限；工作流整体只申请 `contents: read`，仅发布作业单独声明 `contents: write`。

## 安装与配置

使用兼容的模块管理器和 Zygisk 提供程序安装 ZIP，重启后打开模块 WebUI。面板支持开关过滤、添加和移除包名，以及保存并应用配置。配置切换会停止过滤状态发生变化的目标应用，重新打开后生效；不会清除应用数据。

当前操作按钮针对 `me.weishu.kernelsu`，其他管理器的面板入口需要适配验证。未对所有设备或管理器作兼容性保证。

## 源码

- `tools/uid-seccomp-module/module.cpp`、`filter.h`：原生模块。
- `tools/uid-seccomp-module/panel/`：安装、配置脚本及 WebUI 资源。
- `tools/uid-seccomp-module/build.py`：编译与 ZIP 打包。
- `third_party/zygisk/`：原始头文件、许可声明和来源校验信息。

项目沿用 [Apache-2.0 许可证](LICENSE)；第三方 Zygisk 头文件保留其原始许可声明。
