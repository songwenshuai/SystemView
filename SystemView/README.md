# SystemView

SEGGER SystemView is a real-time recording and visualization tool for embedded systems that reveals the true
runtime behavior of an application, going far deeper than the system insights provided by debuggers.

This package contains the SystemView target source code.

For more information refer to

- [SystemView - SEGGER Knowledge Base](https://kb.segger.com/SystemView)
- [SystemView on segger.com](https://www.segger.com/systemview)

## Getting Started

Clone the SystemView [Git Repository](https://www.github.com/SEGGERMicro/SystemView)
and the RTT [Git Repository](https://www.github.com/SEGGERMicro/RTT) into your project or add them as a submodule.
Alternatively [Download](https://www.github.com/SEGGERMicro/SystemView/releases/latest) the latest package
and the RTT package and extract them to the project.

### Usage and Configuration

Follow the documentation of RTT on how to add it to a project.

To include SystemView, add the SystemView core source code `SYSVIEW/*.c` and the project-specific runtime
integration code required by the operating system or scheduler.

SystemView requires project-specific runtime configuration.
Provide the configuration source code for the target and implement the required SystemView runtime interface.

SystemView can be compile-time configured through `SEGGER_SYSVIEW_Conf.h`.
This package uses the shared-memory RTT address API only.  The project configuration must define
`SEGGER_RTT_CB_ADDRESS`, `SEGGER_SYSVIEW_RTT_UP_BUFFER_ADDRESS`, and
`SEGGER_SYSVIEW_RTT_DOWN_BUFFER_ADDRESS` so the RTT control block and SystemView transport buffers are all
inside the same mapped shared-memory region.  `SEGGER_SYSVIEW_RTT_NAME_ADDRESS` is optional and must also point
inside that region when a channel name is required.
The shared-memory transport addresses and the address-aware public SystemView APIs use `PTR_ADDR` and are encoded
with address-sized variable-length fields.
It is recommended to copy `SEGGER_SYSVIEW_Conf.h` to the project's configuration directory and make changes to
the copy.  That way, configuration does not get overwritten by an update of the package.  In that case, do not add
`Config` as include directory.

Add `SEGGER` and `SYSVIEW` as include directories.

For more information on target-specific configuration and instrumentation of not-yet-supported RTOSes, refer to
the [SystemView User Guide](https://doc.segger.com/UM08027_SystemView.html).

### CMSIS-Pack

The CMSIS-Pack enables easy integration of SystemView into CMSIS-based projects.

To use SystemView in a *csolution project* add `pack: SEGGER::SystemView` and
`component: SEGGER:SystemView`.

The component requires the RTT pack.
The [RTE directory](https://open-cmsis-pack.github.io/cmsis-toolbox/build-overview/#rte-directory-structure)
is the project's config directory and contains the local copy of `SEGGER_SYSVIEW_Conf.h` for additional configuration.
