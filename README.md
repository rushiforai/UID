# UID Filter / UID 过滤

作者：泷泽。当前版本：0.2.3。

通过过滤指定包的`getpriority(PRIO_USER)`  做到规避检测。

模块面向 arm64、Android 13+、Zygisk API 5，包含中英文 WebUI。过滤仅作用于配置选中的普通应用进程；默认包名为 `com.chunqiunativecheck` 和 `com.chunqiu.appvisibilitydemo`。

## Windows 构建

需要 Python 3.10+ 和 Android NDK `27.3.13750724`。默认从 `%LOCALAPPDATA%/Android/Sdk` 查找 SDK；可用 `ANDROID_SDK_ROOT` / `ANDROID_HOME` 指定 SDK，或用 `ANDROID_NDK_HOME` 直接指定 NDK 根目录。

```powershell
git clone https://github.com/longze777/UID-.git
Set-Location UID-
python tools/uid-seccomp-module/build.py --module-only
```

只构建模块，无需 Java、Gradle 或额外的 Python 包。Zygisk API 5 头文件已随源码提供，构建时会校验其固定 SHA-256。

输出文件：

- `build/0.2.3/cq-uid-seccomp-0.2.3.zip`
- `build/0.2.3/cq-uid-seccomp-0.2.3.zip.sha256`

可用 `--output <目录>` 指定输出目录，或用 `--config <文件>` 指定默认配置。配置首行为 `enabled=1` 或 `enabled=0`，随后每行一个完整包名。省略 `--module-only` 同样只构建模块。

## 安装与配置

使用兼容的模块管理器和 Zygisk 提供程序安装 ZIP，重启后打开模块 WebUI。面板支持开关过滤、添加和移除包名，以及保存并应用配置。配置切换会停止过滤状态发生变化的目标应用，重新打开后生效；不会清除应用数据。

当前操作按钮针对 `me.weishu.kernelsu`，其他管理器的面板入口需要适配验证。未对所有设备或管理器作兼容性保证。

## 源码

- `tools/uid-seccomp-module/module.cpp`、`filter.h`：原生模块。
- `tools/uid-seccomp-module/panel/`：安装、配置脚本及 WebUI 资源。
- `tools/uid-seccomp-module/build.py`：编译与 ZIP 打包。
- `third_party/zygisk/`：原始头文件、许可声明和来源校验信息。

项目沿用 [Apache-2.0 许可证](LICENSE)；第三方 Zygisk 头文件保留其原始许可声明。
