# virt86

| Windows | Linux and macOS | Downloads |
|:-------:|:---------------:|:---------:|
| [![AppVeyor build status](https://img.shields.io/appveyor/ci/strikerx3/virt86/master.svg?label=AppVeyor%20build&logo=appveyor&style=flat)](https://ci.appveyor.com/project/StrikerX3/virt86/branch/master) | [![Travis CI build status](https://img.shields.io/travis/com/strikerx3/virt86/master.svg?label=Travis%20CI%20build&logo=travis&style=flat)](https://travis-ci.com/StrikerX3/virt86) | [![Bintray downloads](https://img.shields.io/bintray/v/strikerx3/virt86/virt86.svg?style=flat)](https://bintray.com/strikerx3/virt86/virt86/_latestVersion) |

virt86 interfaces with the major x86 hardware-assisted virtualization engines
[Intel HAXM](https://github.com/intel/haxm),
[Microsoft Windows Hypervisor Platform](https://docs.microsoft.com/en-us/virtualization/api/),
[KVM](https://www.linux-kvm.org/page/Main_Page) and
[Hypervisor.Framework](https://developer.apple.com/documentation/hypervisor)<sup>1</sup>, exposing a simple and universal C++ API to consumers while abstracting away the specifics of each platform. It does not provide a hypervisor by itself; instead, it interacts with existing virtualization platforms.

See more details about supported platforms and features [here](https://github.com/StrikerX3/virt86/wiki/Supported-Platforms-and-Features).

<sup>1</sup> *Hypervisor.Framework support is being studied. See issue [#1](https://github.com/StrikerX3/virt86/issues/1) and the [`hvf`](https://github.com/StrikerX3/virt86/tree/hvf) branch.*

## Downloads

You can find the latest release of virt86 [here](https://github.com/StrikerX3/virt86/releases/latest). Older releases are available in the [Releases](https://github.com/StrikerX3/virt86/releases) page. You can also find development builds in [AppVeyor](https://ci.appveyor.com/project/StrikerX3/virt86/branch/master) (Windows only) and [Bintray](https://bintray.com/strikerx3/virt86/virt86/_latestVersion) (all platforms).

## Building virt86

virt86 is built with [CMake](https://cmake.org/). The minimum required version is 3.12.0.

The project has been successfully compiled with the following toolchains:

* Microsoft Visual C++ 19.16.27027.1 (Visual Studio 2017 v15.9.7) on Windows 10, 32- and 64-bit, using Windows 10 SDKs 10.0.17134.0 and 10.0.17763.0
* GCC 7.3.0 on Ubuntu 18.04.0, 64-bit
* GCC 8.2.0 on Ubuntu 18.04.0 (`gcc-8` package), 64-bit
* Apple LLVM 10.0.0 on macOS Mojave 10.14.3, 64-bit

### Building on Windows with Visual Studio 2017

To make a Visual Studio 2017 project you'll need to specify the `"Visual Studio 15 2017"` CMake generator (`-G` command line parameter) and a target architecture (`-A`). Use either `Win32` for a 32-bit build or `x64` for a 64-bit build.

The following commands create and open a Visual Studio 2017 64-bit project:

```cmd
git clone https://github.com/StrikerX3/virt86.git
cd virt86
md build && cd build
cmake -G "Visual Studio 15 2017" -A x64 ..
start virt86.sln
```
The project can be built directly from the command line with CMake, without opening Visual Studio:

```cmd
cd virt86/build
cmake --build . --target ALL_BUILD --config Release -- /nologo /verbosity:minimal /maxcpucount
```

If your installation of Visual Studio 2017 provides a Windows 10 SDK older than 10.0.17134.0, Windows Hypervisor Platform will not be available and the `virt86-whpx` project will neither be included in the solution nor in the library. You'll still be able to use virt86 with HAXM.

To install the library, build the `INSTALL` project from the solution or specify the `INSTALL` target to the `cmake --build` command. The Debug configuration exports `virt86-debug.lib` and `virt86.pdb`. All Release configurations export `virt86.lib`. Note that, by default, CMake projects will install to `C:\Program Files\` on Windows which needs administrative rights to write files to. To work around that, provide the `CMAKE_INSTALL_PREFIX` parameter to the `cmake` command line to point to a directory where you have write permissions: `-DCMAKE_INSTALL_PREFIX=<path>`. Alternatively, you can run Visual Studio with administrative privileges or `cmake` from an elevated command prompt.

### Building on Linux with GCC 7+

Make sure you have at least GCC 7, `make` and CMake 3.12.0 installed on your system.

```bash
git clone https://github.com/StrikerX3/virt86.git
cd virt86
mkdir build; cd build
cmake ..
make
```

To install the library, run `sudo make install` from the `build` directory.

### Building on macOS

Install [brew](https://brew.sh/) if you haven't already. The default installation is sufficient. Xcode is not needed to compile the library -- virt86 can be compiled with the Xcode command line tools included with brew.

```bash
git clone https://github.com/StrikerX3/virt86.git
cd virt86
mkdir build; cd build
cmake ..
make
```

## Using virt86

### Linking with CMake

Once the library is installed on your system, you can link against it as follows:

```cmake
find_package(virt86 CONFIG REQUIRED)
target_link_libraries(YourTarget PUBLIC virt86::virt86)
```

If you installed the library to a non-standard location by specifying `CMAKE_INSTALL_PREFIX` when installing virt86 (or if you downloaded and extracted the package to a different location), you'll have to specify the same path to your project's `CMAKE_INSTALL_PREFIX`.

### Linking against a downloaded release

Extract the file to your preferred location for libraries. Add `<virt86-path>/include` to your project's includes and `<virt86-path>/lib` to your project's library directories. Link against `virt86.lib` on Windows or `libvirt86.a` on Linux and macOS. For debug libraries, use `virt86-debug.lib` or `libvirt86-debug.a`.

### Using the library

`#include "virt86/virt86.hpp"` in your application. The header includes all platforms available on your system and defines a fixed-sized array of platform factories named `virt86::PlatformFactories` for convenience.

Read the [wiki](https://github.com/StrikerX3/virt86/wiki) and check out the [demo projects](https://github.com/StrikerX3/virt86-demos) for more details on how to use virt86.

## Support

If you find this useful and want to support [the author](https://github.com/StrikerX3), consider becoming a [patron](https://www.patreon.com/StrikerX3).
