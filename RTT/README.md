# SEGGER Real Time Transfer

## Shared-Memory Fork Notice

This repository carries a shared-memory RTT fork for heterogeneous systems.
It is intentionally not binary-compatible with standard SEGGER RTT / J-Link RTT readers:

- Public RTT APIs take an explicit `uintptr_t Address` control-block base.
- The RTT module does not allocate the default global `_SEGGER_RTT` control block.
- Descriptor `sName` and `pBuffer` fields contain fixed 64-bit offsets relative to the RTT control-block base, not absolute target pointers.
- Callers must provide an 8-byte aligned shared-memory region of at least `SEGGER_RTT__REQUIRED_MEM_SIZE` bytes before calling `SEGGER_RTT_Init(Address)` or `SEGGER_RTT_InitEx(Address, Size)`.
- `SEGGER_RTT_CheckRegion(Address, Size)` validates an initialized control block, descriptor table, and configured payload buffers against the mapped region without requiring the local default payload layout.
- `SEGGER_RTT_FindControlBlock()` and `SEGGER_RTT_FindValidControlBlock()` are available for range-limited discovery of an initialized control block.
- The shared-memory region must be mapped as non-cacheable normal memory, or the platform must provide cache maintenance outside the RTT fast path.
- Toolchains not covered by the built-in shared-memory barrier handling can define `SEGGER_RTT_SHARED_MEMORY_BARRIER()` in `SEGGER_RTT_Conf.h`.
- The read/write fast paths intentionally do not revalidate the full shared-memory region, probe caller payload pointers, add blocking timeouts, scan all descriptors, or detect cache/MMU attributes on every access. These are integration constraints documented in the top-level `README.md`, not fast-path defects.

See the top-level `README.md` for the full shared-memory layout and ownership constraints.

SEGGER's Real Time Transfer (RTT) is the proven technology for system monitoring and interactive user I/O in
embedded applications.
It combines the advantages of SWO and semihosting at very high performance.

For more information refer to

- [RTT - SEGGER Knowledge Base](https://kb.segger.com/RTT)
- [RTT on segger.com](https://www.segger.com/products/debug-probes/j-link/technology/about-real-time-transfer)

## Usage

This package contains the RTT Target Sources.
It can be used directly,
either by including the [Git Repository](https://www.github.com/SEGGERMicro/RTT) as a submodule,
or by [Download](https://github.com/SEGGERMicro/RTT/releases/latest) of the latest package,
or as an [CMSIS-Pack](https://www.keil.arm.com/packs/?vendor=segger) in [CMSIS](https://www.keil.arm.com/cmsis)
development tools.

### General Usage and Configuration

To include RTT into a project, add `RTT/SEGGER_RTT.c` to the project sources and `RTT` as include directory.

`RTT/SEGGER_RTT_ASM_ARMv7M.S` is optional and can be added as additional source file for an assembly-optimized
implementation of RTT functions.
This file is compatible with ARMv7M and ARMv8M processors and most toolchains.

RTT can be configured through `SEGGER_RTT_Conf.h`.
In most cases the default configuration is sufficient and `SEGGER_RTT_Conf.h` can be left untouched.
In this case, `Config` can be added as include directory.
If additional configuration is required, it is recommended to copy `SEGGER_RTT_Conf.h` to the project's 
configuration directory and make changes to the copy.
That way, configuration does not get overwritten by an update of the package.
In that case, do not add `Config` as include directory.

### Additional Modules

The RTT Target Sources contain additional modules for printf-like output.

`SEGGER_RTT_Printf.c` provides a simple implementation of "printf" to write formatted strings directly via RTT.

`Syscalls/SEGGER_RTT_Syscalls_*.c` provides low-level retargeting functions for `printf()` to RTT with different toolchains.

### Examples

The `Examples` dirctory includes some simple example and test applications to demonstrate RTT.
Some examples can be run as bare-metal applications, some examples require additional modules, such as an RTOS.

- `Main_RTT_InputEchoApp.c`    - Example application which echoes input on Channel 0.
- `Main_RTT_MenuApp.c`         - Example application to demonstrate RTT bi-directional functionality.
- `Main_RTT_PrintfTest.c`      - Example application to test RTT's simple printf implementation.
- `Main_RTT_SpeedTestApp.c`    - Example application to measure RTT performance. (Requires embOS)

**NOTE:** The examples are not included in the CMSIS-Pack.

### CMSIS-Pack

The CMSIS-Pack enables easy integration of RTT into CMSIS-based projects.

To use RTT in a *csolution project* add `pack: SEGGER::RTT` and `component: SEGGER:RTT`.
The [RTE directory](https://open-cmsis-pack.github.io/cmsis-toolbox/build-overview/#rte-directory-structure)
is the project's config directory and contains the local copy of `SEGGER_RTT_Conf.h` for additional configuration.
