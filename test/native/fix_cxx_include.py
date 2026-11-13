"""Fix the macOS-native test toolchain: this machine's Apple clang cannot find
the C++ standard headers by default. Point it at the SDK's libc++ headers.
On other platforms, or when the default toolchain works, this is a no-op."""
import subprocess
import sys

Import("env")  # pyright: ignore[reportUndefinedVariable]  # provided by PlatformIO SCons


def add_sdk_cxx_includes(e):
    if sys.platform != "darwin":
        return
    try:
        sdk = (
            subprocess.check_output(["xcrun", "--show-sdk-path"], stderr=subprocess.STDOUT)
            .decode()
            .strip()
        )
    except Exception:
        return
    e.Append(CXXFLAGS=["-nostdinc++", "-isystem", sdk + "/usr/include/c++/v1"])
    e.Append(LINKFLAGS=["-lc++"])


add_sdk_cxx_includes(env)  # pyright: ignore[reportUndefinedVariable]
