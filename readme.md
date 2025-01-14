# vtm

Text-baased desktop inside your console.

<a href="https://www.youtube.com/watch?v=kofkoxGjFWQ">
  <img width="400" alt="Demo on YouTube" src="https://user-images.githubusercontent.com/11535558/146906370-c9705579-1bbb-4e9e-8977-47312f551cc8.gif">
</a>

# Supported Platforms

- Windows
  - Server/Desktop
- Unix
  - Linux
  - Android <sup><sup>Linux kernel</sup></sup>
  - macOS
  - FreeBSD
  - NetBSD
  - OpenBSD
  - [`...`](https://en.wikipedia.org/wiki/POSIX#POSIX-oriented_operating_systems)
- [Tested Terminals](https://github.com/netxs-group/vtm/discussions/72)

# Building from Source

### Unix

Build-time dependencies
 - 64-bit system host
 - `git`, `cmake`,  `C++20 compiler` ([GCC 11](https://gcc.gnu.org/projects/cxx-status.html), [Clang 14](https://clang.llvm.org/cxx_status.html), [MSVC](https://visualstudio.microsoft.com/downloads/))
 - RAM requirements for compilation:
   - Compiling with GCC — 4GB of RAM
   - Compiling with Clang — 9GB of RAM

Use any terminal as a build environment
```
git clone https://github.com/netxs-group/vtm.git
cd vtm
cmake . -B bin
cmake --build bin
sudo cmake --install bin
vtm
```

Note: A 32-bit binary executable can only be built using cross-compilation on a 64-bit system. In order to do so make sure you have additional cross-compilation libraries installed, e.g. on Linux `sudo apt install gcc-i686-linux-gnu g++-i686-linux-gnu` (x86) or `sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf` (ARM32).

Example of cross-compilation for x86 Linux
```
cmake . -B bin -DCMAKE_CXX_COMPILER="/bin/i686-linux-gnu-g++" -DCMAKE_CXX_FLAGS="-static -pthread -s"
cmake --build bin
```

Example of cross-compilation for ARM32 Linux
```
cmake . -B bin -DCMAKE_CXX_COMPILER="/bin/arm-linux-gnueabihf-g++" -DCMAKE_CXX_FLAGS="-static -pthread -s -Wno-psabi"
cmake --build bin
```

### Windows

Build-time dependencies
 - [git](https://git-scm.com/download/win), [cmake](https://learn.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio?view=msvc-170#installation), [MSVC (Desktop Development with C++)](https://visualstudio.microsoft.com/downloads/)

Use Developer Command Prompt as a build environment

```
git clone https://github.com/netxs-group/vtm.git
cd vtm
cmake . -B bin
cmake --build bin --config Release
bin\Release\vtm.exe
```

# Binary Downloads

![macOS](.resources/status/macos.svg)     [![Universal](.resources/status/arch_any.svg)](https://github.com/netxs-group/vtm/releases/latest/download/vtm_macos_any.tar.gz)  
![Linux](.resources/status/linux.svg)     [![Intel 64-bit](.resources/status/arch_x86_64.svg)](https://github.com/netxs-group/vtm/releases/latest/download/vtm_linux_x86_64.tar.gz) [![Intel 32-bit](.resources/status/arch_x86.svg)](https://github.com/netxs-group/vtm/releases/latest/download/vtm_linux_x86.tar.gz) [![ARM 64-bit](.resources/status/arch_arm64.svg)](https://github.com/netxs-group/vtm/releases/latest/download/vtm_linux_arm64.tar.gz) [![ARM 32-bit](.resources/status/arch_arm32.svg)](https://github.com/netxs-group/vtm/releases/latest/download/vtm_linux_arm32.tar.gz)  
![Windows](.resources/status/windows.svg) [![Intel 64-bit](.resources/status/arch_x86_64.svg)](https://github.com/netxs-group/vtm/releases/latest/download/vtm_windows_x86_64.zip)  [![Intel 32-bit](.resources/status/arch_x86.svg)](https://github.com/netxs-group/vtm/releases/latest/download/vtm_windows_x86.zip)  [![ARM 64-bit](.resources/status/arch_arm64.svg)](https://github.com/netxs-group/vtm/releases/latest/download/vtm_windows_arm64.zip)  [![ARM 32-bit](.resources/status/arch_arm32.svg)](https://github.com/netxs-group/vtm/releases/latest/download/vtm_windows_arm32.zip)  

---

# Documentation

- [Command line Options](doc/command-line-options.md)
- [User Interface](doc/user-interface.md)
- [Settings](doc/settings.md)
- [Built-in Applications](doc/apps.md)
- Draft: [VT Input Mode](doc/vt-input-mode.md)

# Related Repositories

[Desktopio Framework Documentation](https://github.com/netxs-group/Desktopio-Docs)

---

[![HitCount](https://views.whatilearened.today/views/github/netxs-group/vtm.svg)](https://github.com/netxs-group/vtm) [![Twitter handle][]][twitter badge]

[//]: # (LINKS)
[twitter handle]: https://img.shields.io/twitter/follow/desktopio.svg?style=social&label=Follow
[twitter badge]: https://twitter.com/desktopio
