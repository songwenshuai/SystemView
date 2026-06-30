ReadMe.txt for the Simulation start projects.

The Simulation board support package contains a CMake-based Visual Studio Code
project. Legacy Eclipse MinGW-w64 notes are retained as historical Windows
project information only; no Eclipse project files are maintained in this tree.

Supported host platforms:
=========================
- Windows PC
- Linux PC
- macOS

Graphical simulation:
=====================
Windows, Linux and macOS builds enable the graphical simulation window by
default. Linux and macOS use SDL2; install the SDL2 development package before
configuring the project with EMBOS_SIM_GUI enabled.
Use ./build.sh --no-gui to build without the graphical simulation window.

POSIX simulation:
=================
The POSIX scheduler uses SIGUSR1 and SIGUSR2 for host thread suspend and
resume. POSIX simulation builds therefore set OS_SUPPORT_CALL_ISR to 0 and do
not provide OS_INT_Call() or OS_INT_CallNestable().
The POSIX tick controller caps host catch-up with TIME_DIFF_MAX, matching the
Windows simulation behavior. Delayed host scheduling is not converted into a
burst of missed ticks by default.

Allocator interposition:
========================
Linux GNU and Clang builds enable allocator interposition by default with
linker wrapping for malloc(), free(), calloc() and realloc(). Configure with
EMBOS_SIM_ENABLE_ALLOCATOR_INTERPOSITION=OFF to disable it. macOS and Windows
builds do not support this option. AddressSanitizer builds cannot use allocator
interposition.

AddressSanitizer:
=================
AddressSanitizer builds keep LeakSanitizer enabled. Linux GUI runs started
through ./build.sh --asan --run execute the LeakSanitizer check before
SDL_Quit() unloads host GUI driver modules, then disable the exit-time leak
check for that run. lsan_gui.supp suppresses process-lifetime allocations from
the host SDL2/Mesa/LLVM/NVIDIA graphics stack. Project code leaks remain
visible unless their stack matches one of those host graphics frames.

SEGGER support layout:
======================
Shared SEGGER RTT and SystemView core source files are stored in ../../SEGGER.
Simulation-specific SEGGER configuration headers, platform configuration
sources and the embOS SystemView interface are stored in the local SEGGER
directory so each board support package can own its target configuration.
The Simulation CMake project includes the local SEGGER directory before the
shared SEGGER directory.

Legacy Eclipse MinGW notes:
===========================
The historical Eclipse setup was built for Eclipse 2021-06 (4.20.0) and
MinGW-w64 12.2.0. If an external Eclipse project is still used, keep its
preprocessor definitions synchronized with the CMake Windows build:
EMBOS_SIM_HOST_WINDOWS=1, EMBOS_SIM_HOST_POSIX=0 and EMBOS_SIM_GUI=1.

Historical configurations:
--------------------------
- Debug:
  An embOS debug and profiling library is used.
  To use SEGGER SystemView with this configuration, configure
  SystemView to use the IP Recorder and select the IP of the
  target with the default port 19111.

- Release:
  This configuration is prepared to build a "release"
  output. An embOS release library is used.

- Debug_64:
  An embOS debug and profiling library is used.

- Release_64:
  This configuration is prepared to build a "release"
  output. An embOS release library is used.

Visual Studio Code project:
===========================
This project was built for Visual Studio Code.
The sample project is prepared to run on Linux, macOS and Windows hosts.
Linux and Windows support 32-bit and 64-bit configurations. macOS supports
the 64-bit configurations only. Platform-specific Visual Studio Code launch
settings use GDB on Linux and LLDB on macOS. Windows IntelliSense
configurations use the Win32 simulation headers and MSVC mode.

This Visual Studio Code project contains the following tasks and launch
configurations:

- Start Simulation (Debug, 32-bit):
  This configuration is prepared to build a simulation application for debugging
  purposes. A 32-bit embOS debug and profiling library is used. This
  configuration is Linux-only.

- Start Simulation (Release, 32-bit):
  This configuration is prepared to build a "release" output.
  A 32-bit embOS release library is used. This configuration is Linux-only.

- Start Simulation (Debug, 64-bit):
  This configuration is prepared to build a simulation application for debugging
  purposes. A 64-bit embOS debug and profiling library is used.

- Start Simulation (Release, 64-bit):
  This configuration is prepared to build a "release" output.
  A 64-bit embOS release library is used.
