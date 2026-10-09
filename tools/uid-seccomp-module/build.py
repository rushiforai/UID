from pathlib import Path
import argparse, hashlib, json, os, platform, subprocess, zipfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
NDK_VERSION = '27.3.13750724'
VERSION = '0.2.3'
VERSION_CODE = 5
HEADER = ROOT / 'third_party/zygisk/zygisk.hpp'
HEADER_SHA = 'f8d55e8b4f89d418c5941afe62ce6a09ddec1f4afd9a1b0a01eb40a93310dd28'


def host_tag():
    system = platform.system()
    if system == 'Windows':
        return 'windows-x86_64', '.exe'
    if system == 'Darwin':
        return 'darwin-x86_64', ''
    return 'linux-x86_64', ''


def default_sdk():
    sdk = os.environ.get('ANDROID_SDK_ROOT') or os.environ.get('ANDROID_HOME')
    if sdk:
        return Path(sdk)
    if platform.system() == 'Windows':
        local = os.environ.get('LOCALAPPDATA') or str(Path.home() / 'AppData/Local')
        return Path(local) / 'Android/Sdk'
    if platform.system() == 'Darwin':
        return Path.home() / 'Library/Android/Sdk'
    return Path.home() / 'Android/Sdk'


HOST, EXE = host_tag()
NDK = Path(os.environ.get('ANDROID_NDK_HOME') or default_sdk() / 'ndk' / NDK_VERSION)
CXX = NDK / 'toolchains/llvm/prebuilt' / HOST / 'bin' / f'clang++{EXE}'

# Keep the module's runtime behaviour identical: the variant only changes compiler
# optimisation and the metadata used to tell the two packages apart.
VARIANTS = {
    'release': {'flags': ['-O2', '-DNDEBUG'], 'version': VERSION, 'name': 'UID 过滤'},
    'debug': {'flags': ['-O0', '-g', '-DDEBUG'], 'version': f'{VERSION}-debug', 'name': 'UID 过滤 (debug)'},
}
BASE_FLAGS = ['--target=aarch64-linux-android33', '-shared', '-fPIC', '-std=c++17',
              '-static-libstdc++', '-Wall', '-Wextra', '-Werror',
              '-Wl,-z,max-page-size=16384', '-Wl,-z,relro,-z,now', '-fvisibility=hidden']


def build(variant, output, config=None):
    spec = VARIANTS[variant]
    build = output.resolve()
    (build / 'module/zygisk').mkdir(parents=True, exist_ok=True)
    if hashlib.sha256(HEADER.read_bytes()).hexdigest() != HEADER_SHA:
        raise SystemExit(f'{HEADER} does not match the pinned SHA-256 in third_party/zygisk/source.json')

    def run(command):
        subprocess.run([str(x) for x in command], env=dict(os.environ), check=True, timeout=90)

    run([CXX] + BASE_FLAGS + spec['flags'] + ['-I', HEADER.parent, HERE / 'module.cpp', '-llog',
                                              '-o', build / 'module/zygisk/arm64-v8a.so'])
    (build / 'module/module.prop').write_text(
        f'id=cq_uid_seccomp\nname={spec["name"]}\nversion={spec["version"]}\nversionCode={VERSION_CODE}\n'
        f'author=泷泽\ndescription=通过过滤指定包的`getpriority(PRIO_USER)`  做到规避检测\n',
        encoding='utf-8', newline='\n')
    (build / 'module/skip_mount').write_bytes(b'')
    for path in (HERE / 'panel').rglob('*'):
        if path.is_file():
            target = build / 'module' / path.relative_to(HERE / 'panel')
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(path.read_bytes().replace(b'\r\n', b'\n'))
    # A debug build must never inherit the previous variant's config file.
    (build / 'module/config').unlink(missing_ok=True)
    if config:
        (build / 'module/defaults').write_bytes(config.read_bytes().replace(b'\r\n', b'\n'))
    (build / 'module/config').write_bytes((build / 'module/defaults').read_bytes())
    module_files = sorted(p for p in (build / 'module').rglob('*')
                          if p.is_file() and p.name != 'SHA256SUMS')
    (build / 'module/SHA256SUMS').write_text(''.join(
        hashlib.sha256(path.read_bytes()).hexdigest() + '  ' + path.relative_to(build / 'module').as_posix() + '\n'
        for path in module_files), encoding='ascii', newline='\n')
    with zipfile.ZipFile(build / f'cq-uid-seccomp-{spec["version"]}.zip', 'w', zipfile.ZIP_DEFLATED) as z:
        for path in sorted((build / 'module').rglob('*')):
            if path.is_file(): z.write(path, path.relative_to(build / 'module').as_posix())
    module_artifacts = [build / f'cq-uid-seccomp-{spec["version"]}.zip', build / 'module/zygisk/arm64-v8a.so']
    package = module_artifacts[0]
    package.with_suffix('.zip.sha256').write_text(
        hashlib.sha256(package.read_bytes()).hexdigest() + '  ' + package.name + '\n',
        encoding='ascii', newline='\n')
    artifacts = {path.name: {'sha256': hashlib.sha256(path.read_bytes()).hexdigest(), 'bytes': path.stat().st_size}
                 for path in module_artifacts}
    (build / 'artifacts.json').write_text(json.dumps(artifacts, indent=2), encoding='utf-8')
    print(json.dumps(artifacts, indent=2))


def main():
    parser = argparse.ArgumentParser(description='Build the UID Filter Zygisk module ZIP.')
    parser.add_argument('--config', type=Path)
    parser.add_argument('--variant', choices=sorted(VARIANTS), default='release',
                        help='release (-O2) or debug (-O0 -g, version suffixed with -debug)')
    parser.add_argument('--module-only', action='store_true', help='Accepted for compatibility; only the module is built.')
    parser.add_argument('--output', type=Path, help='Output directory; defaults to build/<version>/<variant>.')
    args = parser.parse_args()
    if not CXX.is_file():
        parser.error(f'Android NDK {NDK_VERSION} was not found at {NDK} (missing {CXX}). '
                     'Set ANDROID_SDK_ROOT or ANDROID_NDK_HOME.')
    build(args.variant, args.output or ROOT / 'build' / VERSION / args.variant, args.config)


if __name__ == '__main__': main()
