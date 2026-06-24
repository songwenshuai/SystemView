# SystemView

SystemView 共享内存 RTT 支持。

## 兼容性边界

本工程中的 RTT 是面向异构系统共享内存通信的 fork，不保持标准 SEGGER RTT / J-Link RTT reader 的二进制兼容性。

不兼容点包括：

- RTT API 显式接收 `uintptr_t Address`，调用方必须传入共享内存中的 RTT control block 基地址。
- RTT 模块不再提供默认全局 `_SEGGER_RTT` control block，也不再在模块内部静态分配默认 up/down buffer。
- RTT control block 中的 `sName` 和 `pBuffer` 字段存储 32 位相对 offset，而不是标准 SEGGER RTT 使用的目标地址绝对指针。
- 标准 J-Link RTT reader、未迁移的 SEGGER RTT 示例代码、以及仍按旧函数签名调用 RTT API 的 SystemView 代码不能直接使用这套共享内存 RTT。

这个不兼容性是有意设计：它允许不同核把同一段共享内存映射到不同本地地址，同时仍能用同一份 descriptor 解析 ring buffer。使用本工程时，所有 RTT 调用方都必须遵守下面的共享内存布局约束。

## RTT 共享内存框架

本工程面向异构系统中的 RTT 数据交换。R 核和 A 核通过一段共享内存访问同一个 SEGGER RTT control block 和 ring buffer。共享内存必须映射为 uncache，双方都可以直接进行 32 位 volatile 读写和 payload `memcpy` 访问。

正常 SEGGER RTT 可以类比为 MCU + J-Link：MCU 负责初始化 RTT control block 和 buffer，J-Link 只扫描或读取目标内存里的 RTT control block，不重新初始化它。本框架可以类比为初始化核 + 访问核：初始化核负责创建共享内存里的 RTT control block，访问核像 J-Link 一样解析这块共享内存。

类比关系如下：

- 正常 SEGGER RTT 中，MCU 是初始化方，J-Link 是访问方。
- 本框架中，系统设计指定的初始化核是初始化方，另一个核是访问方。
- 访问核可以像 J-Link 一样检查或搜索 `"SEGGER RTT"`，但只能在约定的共享内存基地址或共享内存映射范围内操作。
- 访问核不能重新初始化或清空 RTT control block。

关键差异如下：

- 正常 SEGGER RTT 的访问方是 J-Link/debug probe，本框架的访问方是另一个 CPU 核。
- 正常 J-Link 可以扫描目标内存，本框架只扫描调用方传入的共享内存映射范围。
- 正常 SEGGER RTT 的 `sName` 和 `pBuffer` 多数是目标地址空间里的指针，本框架必须使用相对 RTT control block 基地址的 offset。
- 正常 SEGGER RTT 通过调试口访问目标内存，本框架由两个核直接访问共享内存，所以共享内存必须是 uncache 映射。
- 正常 J-Link 天然按目标 RTT control block 中的 descriptor 解析，本框架的访问核也必须运行时读取 descriptor，不能使用自己的编译期 RTT 配置推断布局。

角色和启动约束如下：

- RTT control block 的初始化责任由系统设计固定分配给某一方，不因为启动顺序变化而变化。
- R 核或 A 核都可能先启动，软件不能假设初始化核一定先启动完成。
- 访问核只知道共享内存基地址或共享内存映射范围，不知道初始化核编译时的 RTT 配置。
- 如果访问核已知 RTT control block 基地址，必须先检查该地址处的 `acID == "SEGGER RTT"`。
- 如果访问核只知道共享内存映射范围，可以在该范围内搜索 `"SEGGER RTT"`，找到 RTT control block 起始地址后再解析 descriptor。
- 访问核不能重新初始化或清空 control block。
- 访问核不能假设 buffer 个数、buffer 大小、`aDown[]` 位置或完整内存布局与本地编译配置一致。
- 访问核必须通过共享内存里的 RTT control block 解析 descriptor，从 up-buffer 读取初始化核输出的数据，也可以向 down-buffer 写入命令或输入数据。

## Control Block 约定

RTT control block 保持 SEGGER RTT 的基础字段布局：

- `acID[16]`
- `MaxNumUpBuffers`
- `MaxNumDownBuffers`
- `aUp[]`
- `aDown[]`

每个 up/down buffer descriptor 为 24 字节，字段包括：

- `sName`
- `pBuffer`
- `SizeOfBuffer`
- `WrOff`
- `RdOff`
- `Flags`

`sName` 和 `pBuffer` 存储的是相对 RTT control block 基地址的 32 位 offset，不是绝对指针。每一方访问 name 或 ring-buffer payload 时，都使用本地映射基地址计算：

```c
local_ptr = Address + offset;
```

这样可以支持不同核把同一段共享内存映射到不同本地地址。

共享内存布局约束如下：

- `Address` 必须是 RTT control block 起始地址，并且必须 4 字节对齐。
- 调用方为默认 control block、默认 up/down buffer、默认 terminal name 和 active terminal 状态分配的内存至少为 `SEGGER_RTT__REQUIRED_MEM_SIZE` 字节。
- `SEGGER_RTT_Init(Address)` 会从 `Address` 开始清零 `SEGGER_RTT__REQUIRED_MEM_SIZE` 字节。当前 API 没有 size 参数，因此调用方必须保证这段内存完整、可写、不会覆盖其他对象。
- 通过 `SEGGER_RTT_Config*()` 或 `SEGGER_RTT_Alloc*()` 配置的 `sName` 和 `pBuffer` 必须位于同一段共享内存窗口内，并且地址必须大于 `Address`。
- 任意 `sName - Address`、`pBuffer - Address`、`pBuffer - Address + BufferSize` 都必须能用 32 位无符号 offset 表示。
- descriptor 中的 `MaxNumUpBuffers`、`MaxNumDownBuffers`、`pBuffer`、`SizeOfBuffer`、`RdOff`、`WrOff` 必须只由可信初始化方或受控配置方写入。访问方不能把来自未知内存或损坏内存的 descriptor 当作可信输入。
- down-buffer descriptor 的起始位置由共享内存中的 `MaxNumUpBuffers` 决定，而不是访问方本地编译期的 `SEGGER_RTT_MAX_NUM_UP_BUFFERS`。

## 异构解析要求

访问核必须先检查 `acID == "SEGGER RTT"`，确认 control block 已完成初始化。未初始化时，访问核不能调用会触发初始化或修改 descriptor 的普通 RTT API，只能等待初始化核完成，或在约定共享内存范围内继续检查/搜索。

如果访问核已知的是一段共享内存映射范围，而不是精确 RTT control block 基地址，可以在该范围内搜索 `"SEGGER RTT"`。这个行为类似正常 RTT 中 J-Link 扫描目标内存，但搜索范围只应限定在约定的共享内存区域内。

当前 RTT 提供 `SEGGER_RTT_FindControlBlock(&Address, Size)` 用于这个场景。调用前 `Address` 是共享内存映射起始地址，`Size` 是映射范围大小；找到后 `Address` 会被更新为 RTT control block 起始地址。该 API 只搜索 RTT ID，不会初始化或修改 control block，并且只接受 4 字节对齐且至少能容纳 RTT 基础头部的候选地址。

读取 down-buffer 时，`aDown[]` 的起始地址必须用共享内存中的 `MaxNumUpBuffers` 运行时计算：

```c
aDownBase = Address + SEGGER_RTT__CB_OFF_A_UP + MaxNumUpBuffers * 24;
```

不能使用本地编译期 `SEGGER_RTT_MAX_NUM_UP_BUFFERS` 推导 down-buffer 位置。否则当两个核编译配置不一致时，down-buffer descriptor 会错位。

buffer 个数、buffer 大小和 payload offset 都以共享 control block 中的 descriptor 为准：

- up-buffer 数量读取 `MaxNumUpBuffers`
- down-buffer 数量读取 `MaxNumDownBuffers`
- payload 地址读取 descriptor 的 `pBuffer`
- payload 大小读取 descriptor 的 `SizeOfBuffer`

## 当前实现

共享内存路径已整合到 `RTT/RTT/SEGGER_RTT.c` 和 `RTT/RTT/SEGGER_RTT.h`。

代码已经实现的约束：

- `SEGGER_RTT_CheckInit(Address)` 会检查 control block 地址非空、4 字节对齐且 ID 完整匹配。
- `SEGGER_RTT_InitEx(Address, Size)` 会在清零初始化前检查 `Size >= SEGGER_RTT__REQUIRED_MEM_SIZE`。
- `SEGGER_RTT_CheckRegion(Address, Size)` 会验证完整 ID、runtime buffer count、descriptor table 范围、已配置 payload 范围、`RdOff/WrOff` 和 flags。
- `SEGGER_RTT_FindControlBlock(&Address, Size)` 只在调用方传入的映射范围内搜索，并拒绝非 4 字节对齐或连基础头部都放不下的候选地址。
- 读写 up/down buffer 时，代码运行时读取 `MaxNumUpBuffers`、`MaxNumDownBuffers`、`pBuffer`、`SizeOfBuffer`、`RdOff`、`WrOff`，不使用本地编译期 buffer 个数或大小推断远端 descriptor。
- down-buffer descriptor 的位置用共享 control block 中的 `MaxNumUpBuffers` 运行时计算。
- `Config*`、`Alloc*`、`SetName*`、`SetFlags*` 这类会修改 descriptor 的 API 也按共享 control block 中的 runtime buffer count 和 down-buffer 位置操作。
- `Config*`、`Alloc*`、`SetFlags*` 会拒绝无效 flags、过小 buffer 和不能编码成 32 位相对 offset 的 name/buffer 地址。
- `INIT(Address)` 已从首字节检查升级为完整 `"SEGGER RTT"` ID 检查，避免半初始化或损坏 control block 被当成有效 descriptor。
- `SEGGER_RTT_BUFFER_UP`、`SEGGER_RTT_BUFFER_DOWN` 和 `SEGGER_RTT_CB` 的关键字段 offset 由编译期断言校验，防止结构体布局和 offset 宏漂移。

关键 API：

- `SEGGER_RTT_Init(Address)`：显式初始化并清空 RTT control block，只允许初始化核调用。
- `SEGGER_RTT_InitEx(Address, Size)`：显式初始化并清空 RTT control block，但会先检查共享内存大小；推荐初始化核优先使用。
- `SEGGER_RTT_CheckInit(Address)`：只检查 RTT control block 是否已初始化，不会清空或创建 control block。
- `SEGGER_RTT_CheckRegion(Address, Size)`：只校验已初始化 control block 和 descriptor 是否落在共享内存范围内，不会修改 control block。
- `SEGGER_RTT_FindControlBlock(&Address, Size)`：在共享内存映射范围内搜索 RTT control block，找到后更新 `Address`。
- `SEGGER_RTT_ReadUpBufferNoLock(Address, ...)`：从 up-buffer 读取数据，不自动初始化。
- `SEGGER_RTT_ReadNoLock(Address, ...)`：从 down-buffer 读取数据，不自动初始化。
- `SEGGER_RTT_WriteNoLock(Address, ...)`：写 up-buffer，不自动初始化。
- `SEGGER_RTT_WriteDownBufferNoLock(Address, ...)`：写 down-buffer，不自动初始化。
- `SEGGER_RTT_GetBytesInBuffer(Address, ...)`：精确计算 up-buffer 中已有数据，支持 wrap-around。
- `SEGGER_RTT_GetBytesDownInBuffer(Address, ...)`：精确计算 down-buffer 中已有数据，支持 wrap-around。

`SEGGER_RTT_Read()`、`SEGGER_RTT_Write()`、`SEGGER_RTT_ReadUpBuffer()`、`SEGGER_RTT_WriteDownBuffer()`、`SEGGER_RTT_PutChar()` 等带锁普通 API 保留 SEGGER RTT 的自动初始化语义。它们适合初始化核或已确认 control block 初始化完成后的数据访问，不适合作为访问核探测共享 control block 的第一步。

`SEGGER_RTT_Config*()`、`SEGGER_RTT_Alloc*()`、`SEGGER_RTT_SetName*()`、`SEGGER_RTT_SetFlags*()` 会修改共享 control block，只能由初始化核或系统设计允许的配置方调用。访问核不应调用这些配置 API。

SystemView 使用以下配置宏接入共享 RTT：

- `SEGGER_RTT_CB_ADDRESS`：通用 RTT control block 基地址。系统只有一个共享 RTTCB 时，应用只需要配置这个宏。
- `SEGGER_RTT_SYSCALL_CB_ADDRESS`：`RTT/Syscalls` stdio retargeting 使用的 RTTCB 基地址，默认继承 `SEGGER_RTT_CB_ADDRESS`。
- `SEGGER_SYSVIEW_RTT_CB_ADDRESS`：SystemView 使用的 RTT control block 基地址，默认继承 `SEGGER_RTT_CB_ADDRESS`。只有在 SystemView 和 stdio retargeting 有意使用不同 RTTCB 时，才需要单独覆盖。
- `SEGGER_SYSVIEW_RTT_NAME_ADDRESS`：可选的 SystemView RTT channel name 字符串地址，默认 `0u`，即不安装 name。需要 channel name 时，应用必须把字符串放在共享内存中，并让该地址能编码成相对 `SEGGER_SYSVIEW_RTT_CB_ADDRESS` 的 32 位 offset。

SystemView 内置 `_UpBuffer`/`_DownBuffer` 或 `SEGGER_SYSVIEW_InitAdditionalBuffer()` 传入的 buffer 也必须位于 RTT 共享内存窗口内。通常做法是用 linker section 把这些 buffer 放进共享内存，或者由平台分配共享 buffer 后再传入初始化接口。

## 内存要求

- 共享内存必须是 uncache 映射。
- 共享内存基地址必须满足 4 字节对齐，保证 32 位 control field 访问安全。
- descriptor 字段使用 32 位读写。
- ring-buffer payload 可以用普通 `memcpy` 访问。
- 当前不支持需要平台专用 accessor 的非字节寻址内存。
- 当前不支持 strong device memory 这类不适合普通 `memcpy` payload 访问的映射。
- `RTT__DMB()` 只提供访问顺序约束，不执行 cache clean/invalidate。为了保持 RTT 快路径简单，平台必须把 RTT 共享内存映射为 non-cacheable normal memory；如果平台只能使用 cacheable 共享内存，需要在平台层补充 cache maintenance，而不能只依赖 `RTT__DMB()`。
- shared-memory 模式下 `RTT__DMB()` 提供 GCC/Clang、IAR ARM 和 Keil ARM Compiler 5 的默认实现；其他编译器需要通过 `SEGGER_RTT_SHARED_MEMORY_BARRIER()` 提供平台 barrier。

## 使用约束

- 系统设计必须固定初始化核；启动顺序不能改变初始化责任。
- 只有初始化核能调用 `SEGGER_RTT_Init(Address)`，也只有初始化核能在 control block 尚未出现完整 `"SEGGER RTT"` ID 时调用带自动初始化语义的普通 RTT API。
- 访问核只能等待、检查或搜索已有 control block；确认 ID 完整匹配前，不能调用可能触发 `_DoInit()` 的普通 RTT API。
- `SEGGER_RTT_Config*()`、`SEGGER_RTT_Alloc*()`、`SEGGER_RTT_SetName*()`、`SEGGER_RTT_SetFlags*()` 属于 descriptor 配置操作，访问核默认禁止使用。
- 共享内存的 uncache/normal-memory/MMU 属性由平台保证，RTT C 代码无法可靠检测。
- 调用配置 API 传入的 `sName` 和 `pBuffer` 必须位于同一个共享内存映射内，并且能用相对 RTT control block 基地址的 32 位 offset 表示。
- 当前 address-only 读写 API 没有映射大小参数，因此无法在每次 payload 访问前证明 descriptor offset 一定没有越过映射范围。调用方必须只把可信、已初始化、映射完整的 RTT control block 地址传给这些 API。
- 为了保留 RTT 快路径性能，当前读写 API 不在每次访问时扫描整个共享内存或验证完整 region 边界。边界可信性由初始化协议、共享内存分配和 descriptor 写入权限保证。
- 如果 control block 可能被另一个核部分写入、复位中断打断或被错误写坏，访问方必须先用 `SEGGER_RTT_CheckInit(Address)` 或 `SEGGER_RTT_FindControlBlock(&Address, Size)` 建立完整 ID 前置条件，再进入普通读写路径。
- 如果两个核可能同时写同一个 up-buffer、同时写同一个 down-buffer，或运行时并发重配置 RTT control block，需要额外的跨核同步机制。`SEGGER_RTT_LOCK()` 只解决本核内的临界区，不等价于跨核锁。

以下检查刻意不放在 RTT 每次读写的热路径中，而是作为文档约束、初始化约束或显式 checked API 约束处理。这是为了保留 RTT 数据路径性能，不是实现缺陷：

- 不在每次 `Read/Write/PutChar` 中验证 `pBuffer + SizeOfBuffer` 是否仍落在共享 region 内；需要时使用 `SEGGER_RTT_CheckRegion(Address, Size)` 在初始化后或诊断阶段验证。
- 不在每次访问中扫描全部 up/down descriptor table；runtime buffer count 和 descriptor table 必须由可信初始化方写入。
- 不在每次访问中重新校验 descriptor 是否被并发重配置；配置 API 和数据读写 API 不能无同步地并发访问同一个 descriptor。
- 不在每次访问中探测调用方传入的 payload 指针是否可读写；`pData`、`pBuffer` 和字符串指针必须是调用方本地地址空间中的有效内存，并且长度参数必须准确。
- 不在 `SEGGER_RTT_WriteString()` 和 `SEGGER_RTT_TerminalOut()` 中限制 `STRLEN()` 扫描上限；字符串必须以 NUL 终止且长度受调用方控制。高频或二进制数据应使用带显式长度的 `SEGGER_RTT_Write()`。
- 不在 overwrite/blocking 写路径中限制单次 `NumBytes` 的最大值；如果实时延迟重要，调用方必须把单次写入大小限制在系统约定的消息上限内。
- 不在 `SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL`、`SEGGER_RTT_PutChar()`、`SEGGER_RTT_WaitKey()` 或 terminal blocking 路径中加入超时；调用方只能在确认对端会推进读写指针的上下文中使用阻塞模式。
- 不在每次访问中验证 `sName` 是否 NUL 终止；name 只用于识别和诊断，不参与数据传输热路径。
- 不在 C 代码中检测 MMU/cache 属性；共享内存必须由平台映射为 non-cacheable normal memory，或由平台层承担 cache maintenance。
- 不在每次 payload 拷贝时判断共享内存是否适合普通 `memcpy`；默认要求 RTT payload 区是 normal memory。若平台需要逐字节 volatile 访问，应配置 `SEGGER_RTT_MEMCPY_USE_BYTELOOP`，这是平台集成选择，不是数据路径自动探测。
- 不在 RTT lock 中实现跨核互斥；如果多个核可能同时写同一 ring buffer 或重配置 descriptor，系统必须提供跨核同步。
- 不在每次读写时防御任意内存破坏；运行期 descriptor 被未授权写坏属于系统集成错误，应通过所有权、MPU/MMU 权限或显式诊断检查处理。

## 注意事项

标准 J-Link SEGGER RTT reader 不是该共享内存数据路径的一部分，因为本工程中 descriptor 的 `sName` 和 `pBuffer` 使用相对 offset，而不是传统 SEGGER RTT 的绝对指针。

当前 RTT 单元测试覆盖了共享内存模式下的初始化、读写、wrap-around、trim/skip/block 模式、动态 buffer 配置和 byte-loop memcpy 路径。测试通过的前提是：调用方正确分配共享内存，参与方遵守 offset layout，descriptor 不被未授权写坏，且共享内存具备上述 cache 属性。
