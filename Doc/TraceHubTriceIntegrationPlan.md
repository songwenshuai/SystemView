# TraceHub Trice 集成计划

## 1. 目标

本计划面向 TraceHub 集成 Trice 日志格式，满足以下目标：

1. 两个异构核心都输出 Trice 二进制日志，TraceHub 负责采集、解码、合并、显示和落盘。
2. 使用 Trice 过程中不依赖 Go 工具，不要求用户安装或执行 `trice` Go 命令。
3. Trice 目标端继续使用 `Trice/src` 的 C 代码，TraceHub 侧补齐 Go 工具原本承担的主机端能力。
4. 保留 TraceHub 当前 RTT、共享内存、SystemView、文本日志路径，不破坏现有工作流。
5. 日志等级作为 Trice tag 或 ID 区间语义在 TraceHub 侧处理，不要求修改 Trice 目标端宏体系。

## 2. 非目标

1. 不在 TraceHub 中嵌入 Go runtime、cgo 或外部 Go 可执行文件。
2. 不移植 `displayServer` 的浏览器服务能力，TraceHub 已有终端显示和文件输出路径。
3. 不移植 `scan` 的串口发现能力，当前需求以 RTT 和共享内存通道为采集入口。
4. 不重写 `Trice/src` 的目标端核心库。该目录已经是 C 实现，应作为固件侧依赖集成。
5. 不把日志等级做成目标端运行时过滤开关。Trice 的等级语义来自格式字符串 tag、ID 区间和主机端过滤。

## 3. 当前代码事实

### 3.1 TraceHub 现状

TraceHub 当前是 C11 工程，主流程由 `main.c`、`App/CLI.c`、`App/OptionsApi.c`、`App/OptionsParse.c`、`App/OptionsGetoptWin.c`、`App/ServicePlan.c` 和 `App/RunLoop.c` 组织。日志路径以 Linux、RTOS、SystemView 三类固定服务为中心。

关键限制如下：

1. `Log/LogEntry.h` 中 `LogSource_t` 固定为 `LOG_SOURCE_LINUX` 和 `LOG_SOURCE_RTOS`。
2. `Log/LogCollector.h` 中采集配置固定为 `linux_channel` 和 `rtos_channel`。
3. `Log/LogCollectorRuntime.c` 以文本行作为输入，按 CR/LF 切分后交给 `LogLineParser`。
4. `Log/LogLineParser.c` 解析文本时间戳，不理解 Trice 二进制帧。
5. `Log/LogMerger.h`、`Log/SwimLaneRenderer.c` 和 `Log/LogSwimLane.c` 都默认两个文本日志来源。

结论：TraceHub 要接收两个异构核心的 Trice 日志，必须把固定 Linux/RTOS 文本模型改成表驱动的来源模型，并引入解码器接口。

### 3.2 Trice 现状

Trice 仓库由三部分组成：

1. `Trice/src`：目标端 C 库，负责在固件中编码 Trice 日志。
2. `Trice/cmd`、`Trice/internal`、`Trice/pkg`：Go 主机工具，负责 ID 管理、源码改写、查表解码、过滤、串口扫描和显示服务。
3. `til.json`、`li.json`：Go 工具维护的 ID 查找表和源码位置信息。

结论：目标端不用重写，主机端 Go 工具能力需要按 TraceHub 工作流改写为 C。

## 4. 必须用 C 重写的 Go 能力

### 4.1 必须重写

| 功能 | Go 位置 | C 侧目标 | 必要性 |
| --- | --- | --- | --- |
| `insert` | `Trice/internal/id/insertIDs.go` | 扫描 C/C++ 源码，为 Trice 宏写入 `iD(n)`，更新 ID 表 | 没有它就无法在无 Go 环境中维护 Trice ID |
| ID 分配 | `Trice/internal/id/manage.go`、`helper.go`、`switchIDs.go` | 生成、复用、校验 ID，处理冲突和 ID 区间 | 保证固件编译产物和 TraceHub 解码表一致 |
| `til.json` 读写 | `Trice/internal/id/*` | 维护 ID 到类型、格式字符串、参数个数的映射 | TraceHub 解码必须依赖 ID 查表 |
| `li.json` 读写 | `Trice/internal/id/*` | 维护 ID 到源码文件、行号、宏位置的映射 | `insert`、`clean`、诊断信息需要同一事实源 |
| TREX 解码 | `Trice/internal/trexDecoder/trexDecoder.go` | 解析 Trice 二进制记录，按 ID 查表还原文本 | TraceHub 运行时必须直接解码二进制日志 |
| 格式化解析 | `Trice/internal/fmtspec/fmtspec.go` | 解析 `%d`、`%u`、`%x`、`%f`、`%s`、宽度修饰等格式 | 参数类型和字节宽度由格式字符串决定 |
| 帧解码 | `Trice/internal/trexDecoder` 与 `Trice/src/*Decode.c` | 阶段二支持 none、COBS、TCOBSv1；TCOBSv2 仅在补齐 C 实现和样例后启用 | RTT 通道读到的是帧流，不是完整日志文本 |
| tag 解析和过滤 | `Trice/internal/emitter/*`、`Trice/internal/args/init.go` | 从格式字符串中识别 `err:`、`wrn:`、`info:`、`dbg:` 等 tag | Trice 没有目标端日志等级，TraceHub 必须在主机侧承接 |

帧格式边界必须以当前可验证实现为准。Go 工具当前能够接受 `tcobsv2` 配置，但 `Trice/src` 中可直接复用的 C 解码实现只有 COBS 和 TCOBSv1，阶段二不能承诺 TCOBSv2 运行时支持。用户配置了未支持的 framing 时，`ServicePlan` 必须直接报错，不能静默回退到其他 framing。

### 4.2 按工作流决定是否重写

| 功能 | Go 位置 | C 侧目标 | 采用条件 |
| --- | --- | --- | --- |
| `clean` | `Trice/internal/id/cleanIDs.go`、`zeroIDs.go` | 移除或清零源码中的 `iD(n)`，同步清理表文件 | 需要保持源码不带固定 ID 时采用 |
| `add` | `Trice/internal/id/addIDs.go` | 扫描已存在 ID 的源码，只更新 `til.json` 和 `li.json` | 引入已有 Trice 源码或第三方库时采用 |
| `generate` | `Trice/internal/id/generate.go` | 生成 C 查找表、头文件或 TraceHub 内置表 | 不希望运行时读取 JSON 时采用 |
| XTEA 解密 | `Trice/pkg/cipher/cipher.go` | 解密加密 Trice 帧 | 固件启用 Trice 加密时采用 |
| `TRICE_X0` 扩展 | `Trice/internal/decoder/typeX0.go` | 解析扩展记录 | 固件实际使用 X0 记录时采用 |
| ANSI 着色 | `Trice/internal/emitter/lineTransformerANSI.go` | 终端颜色渲染 | TraceHub 需要彩色终端输出时采用 |

### 4.3 不需要重写

| 功能 | 原因 |
| --- | --- |
| `displayServer` | TraceHub 已经负责显示和落盘，不需要 Go 的 Web 显示服务 |
| `scan` | 当前采集入口是 RTT 和共享内存，不依赖串口扫描 |
| Go CLI 框架 | TraceHub 应使用自身 CLI 风格扩展选项 |
| 目标端编码宏 | `Trice/src` 已是 C 实现，应直接集成到两个核心的固件工程 |

## 5. TraceHub 架构改造方案

### 5.1 数据流

目标数据流如下：

```text
RTT/shared-memory channel
  -> TraceSource
  -> TraceDecoder(TEXT or TRICE)
  -> LogEntry
  -> LogMerger
  -> SwimLaneRenderer / log file
```

构建期数据流如下：

```text
C/C++ firmware source
  -> tracehub-trice-id
  -> source with iD(n)
  -> til.json / li.json or generated C table
  -> firmware build and TraceHub runtime
```

### 5.2 工具边界原则

编译前工具应独立成单独可执行文件，但归属于 TraceHub 套件统一维护、构建和发布。

推荐入口如下：

```text
tracehub              # runtime tool: collect, decode, merge, display, write logs
tracehub-trice-id     # pre-build tool: insert, clean, add, generate
```

边界规则如下：

1. `tracehub` 只负责运行时采集、解码、合并、显示和落盘，不改写固件源码，不维护 `til.json` 和 `li.json`。
2. `tracehub-trice-id` 只负责编译前源码扫描、ID 分配、源码改写、ID 表维护和生成表输出。
3. 两个可执行文件共享 `TriceIdTable`、`TriceFormat`、`TriceTag` 等库代码，避免重复解析 ID 表、格式字符串和 tag。
4. 源码扫描和源码改写只存在于 `Tools/TriceIdTool`，不能进入 TraceHub 运行时路径。
5. RTT 采集、帧解码、日志合并和显示只存在于 `tracehub` 运行时路径。
6. `tracehub-trice-id` 在固件编译前执行，遇到 ID 冲突、格式字符串不一致、参数数量不一致时以非零返回码失败，阻止固件继续构建。

推荐目录组织如下：

```text
TraceHub/
  Core/
    TraceSource.h
    TraceSource.c
  App/
  Log/
  RTT/
  Trice/
    TriceDecoder.c
    TriceIdTable.c
    TriceFormat.c
    TriceTag.c
    TriceFraming.c
  Tools/
    TriceIdTool/
      main.c
      TriceSourceScan.c
      TriceIdInsert.c
      TriceIdClean.c
      TriceIdAdd.c
      TriceIdGenerate.c
```

核心原则是独立入口、共享库代码。这样能够保持运行时工具职责清晰，同时让编译前工具与 TraceHub 运行时使用同一套 Trice 语义模型。

### 5.3 新模块划分

| 模块 | 职责 |
| --- | --- |
| `TraceSource` | 描述日志来源，包括名称、核心类型、RTT 通道、解码类型、ID 表路径；该模块属于公共基础层，不能放在 App 层导致 Log 反向依赖 App |
| `TraceDecoder` | 解码器统一接口，屏蔽文本日志和 Trice 二进制日志差异 |
| `TextTraceDecoder` | 承接现有 `LogLineParser` 文本解析能力 |
| `TriceDecoder` | 解析 Trice 帧、TREX 记录和参数格式化 |
| `TriceIdTable` | 加载或接收生成的 ID 查找表 |
| `TriceFormat` | 解析格式字符串，计算参数类型和宽度 |
| `TriceTag` | 提取 tag、映射日志等级、执行 pick/ban 过滤 |
| `TriceFraming` | 统一 none、COBS、TCOBSv1 帧处理；TCOBSv2 仅在补齐 C 解码实现后加入 |
| `Tools/TriceIdTool` | 生成独立可执行文件 `tracehub-trice-id`，实现 `insert`、`clean`、`add`、`generate` |

### 5.4 TraceHub 现有模块调整

1. `App/Options` 增加表驱动 source 配置，不再把 Linux/RTOS 写死为唯一日志来源。
2. `App/CLI` 增加 Trice source 描述选项，例如 source 名称、RTT 通道、ID 表、帧格式、字节序、过滤 tag。
3. `App/ServicePlan` 从固定三类服务改成统一服务描述，保持通道冲突检查。
4. `Log/LogEntry` 将 `LogSource_t` 改成 source id 或 source descriptor 引用。
5. `Log/LogCollector` 从两个固定 source 改成 N 个 source，每个 source 绑定一个 decoder。
6. `Log/LogMerger` 从固定 `LOG_SOURCE_MAX` 改成按 source 数量初始化。
7. `Log/SwimLaneRenderer` 从双列固定布局改成按 source 列表渲染，两个核心场景仍保持紧凑显示。
8. `Log/LogSwimLane` 输出 source 名称，不依赖 Linux/RTOS 枚举名称。

### 5.5 Source 模型和容量约束

source 模型必须先定义容量和依赖边界，避免下游模块各自推导。

1. `TraceSource` 放在公共基础层，例如 `TraceHub/Core`，由 `App`、`Log` 和 `Trice` 共同依赖；`Log` 不能包含 `App` 头文件。
2. 全局定义 `TRACEHUB_MAX_SOURCES`，所有 source id、数组上限、renderer 列数、merger 状态和 recorder 状态都使用同一个上限。
3. `ServicePlan` 负责校验配置的 source 数量、source 名称唯一性、RTT 通道唯一性和 SystemView 通道冲突。
4. 共享内存仿真和 RTT 配置必须显式保证 up-buffer 数量大于等于所有 enabled source 与 SystemView 所需通道数；容量不足时在 plan 阶段报错。
5. `CoreLogRecorder` 按 source 配置 `record_mode`，取值为 `text_clean`、`raw_binary`、`off`。文本 source 才能使用 text clean；Trice source 的原始帧只能使用 raw binary 或 off。

## 6. Trice 日志等级处理方案

Trice 不在目标端维护日志等级字段，原因是它追求目标端低开销：目标端只编码 ID、类型、参数和可选时间戳，等级语义由格式字符串 tag、ID 区间和主机工具解释。

TraceHub 应按以下规则承接日志等级：

1. 从 `til.json` 或生成表中读取格式字符串。
2. 从格式字符串前缀提取 tag，例如 `err:`、`wrn:`、`info:`、`dbg:`、`trace:`。
3. 将 tag 映射为 TraceHub 内部等级。
4. 支持按等级或 tag 过滤显示和落盘。
5. 对没有 tag 的 Trice 记录标记为 `unclassified`，并在严格模式下报错或拒绝通过。
6. 对两个核心使用同一套等级映射，避免每个核心重复定义等级规则。

如果固件需要在目标端减少日志输出，应使用 Trice 的编译期开关、宏开关或 ID 区间控制，而不是在 TraceHub 中模拟目标端等级开关。

## 7. 实施阶段

### 7.1 阶段一：TraceHub 来源模型重构

目标：把 TraceHub 从固定 Linux/RTOS 文本日志模型改成统一 source 模型。

交付内容：

1. 新增 source descriptor，集中保存 source 名称、通道、解码器类型和输出标签。
2. 改造 `LogEntry`、`LogCollector`、`LogMerger`、`SwimLaneRenderer`，消除 Linux/RTOS 固定枚举依赖。
3. 保持现有 Linux/RTOS 文本路径行为一致。
4. 保持 SystemView 服务路径独立运行。

完成标准：

1. 现有两个文本核心日志仍能按时间戳合并。
2. swimlane 输出不再依赖硬编码 Linux/RTOS 枚举。
3. 新 source 模型能够表达两个 Trice 核心。
4. source 数量、source id 和 RTT 通道容量在 `ServicePlan` 阶段统一校验，下游模块不再重复判断。
5. `CoreLogRecorder` 能按 source 区分 `text_clean`、`raw_binary` 和 `off`，二进制 source 不会进入文本清理函数。

### 7.2 阶段二：TraceHub 运行时 Trice 解码

目标：TraceHub 能从 RTT 通道读取 Trice 二进制帧并还原文本日志。

交付内容：

1. 引入 `TriceDecoder`，阶段二支持 none、COBS、TCOBSv1；TCOBSv2 必须等到 C 解码实现和固定样例齐全后再启用。
2. 复用或移植 `Trice/src` 中已有 COBS/TCOBSv1 解码代码。
3. 实现 TREX 记录解析，包括 ID、类型、cycle counter、时间戳和参数 payload。
4. 实现 `til.json` 加载或 generated C table 读取。
5. 实现格式字符串参数解析和安全格式化。
6. 实现 tag 提取、等级映射和过滤。
7. 建立运行时 Trice 配置矩阵，明确每个 Go `log` 选项在 C runtime 中是支持、拒绝还是延期。

完成标准：

1. 给定 Trice 原始字节流和匹配 ID 表，TraceHub 输出确定文本。
2. 两个核心的 Trice 日志能进入同一个 `LogMerger`。
3. 解码错误包含 source 名称、通道、ID、帧位置等诊断信息。
4. 未支持的 framing、timestamp 模式、ID 模式和 X0/XTEA 选项必须在 plan 阶段失败，不能运行时静默降级。
5. `til.json` 与 generated C table 的同一输入样例解码输出一致。

### 7.3 阶段三：C 版 Trice ID 工具

目标：用户不再执行 Go `trice insert`，TraceHub 套件提供独立可执行文件 `tracehub-trice-id`，在固件编译前完成 ID 管理。

交付内容：

1. 新增 `tracehub-trice-id` 可执行入口，不挂到 `tracehub` 运行时 CLI。
2. 扫描 C/C++ 源码中的 Trice 宏，识别已有 `iD(n)`、缺失 ID、格式字符串和参数个数。
3. 维护 `til.json` 和 `li.json`，保证 ID 表是单一真相源。
4. 支持 ID 区间配置，使两个核心或不同 tag 能使用明确 ID 范围。
5. 对 ID 冲突、格式字符串变化、参数数量变化严格报错。
6. 共享 `TriceIdTable`、`TriceFormat`、`TriceTag`，不复制运行时已有语义规则。

完成标准：

1. 同一份源码重复执行 `insert` 不产生无意义变更。
2. ID 表与源码中的 `iD(n)` 完全一致。
3. 冲突和不一致状态能在构建期暴露。
4. 固件构建系统能够把 `tracehub-trice-id insert` 作为编译前步骤调用。

### 7.4 阶段四：补齐源码管理命令

目标：覆盖 Trice 常用源码维护工作流。

交付内容：

1. `clean`：移除或清零源码中的 `iD(n)`，并同步 ID 表。
2. `add`：扫描已有 ID 的源码，更新 ID 表，不改写源码。
3. `generate`：从 ID 表生成 C 查找表或 TraceHub 解码表。

完成标准：

1. `insert`、`clean`、`add`、`generate` 共享同一套解析和 ID 表逻辑。
2. 不存在多个模块重复解析 Trice 宏或重复维护 ID 规则。
3. 所有命令对异常输入执行严格报错。

### 7.5 阶段五：扩展能力

目标：覆盖固件实际使用的高级 Trice 特性。

交付内容：

1. XTEA 解密。
2. `TRICE_X0` 扩展记录。
3. 在阶段二已覆盖固件实际使用基础形式的前提下，补齐 `TRICE_S`、`TRICE_N`、`TRICE_B`、`TRICE_F` 的所有变体和边界样例。
4. 彩色终端输出和更细的 tag 过滤策略。

完成标准：

1. 每项扩展都有独立输入样例和期望输出。
2. 未启用的扩展不会影响主路径。
3. 启用扩展时，错误路径可定位到 source、ID 和帧。

## 8. 推荐命令形态

TraceHub 套件应提供两个命令入口：`tracehub` 用于运行时，`tracehub-trice-id` 用于编译前 ID 管理。

### 8.1 运行时命令

`tracehub` CLI 应使用统一 source 描述，而不是继续增加 `--linux`、`--rtos` 同级固定选项。

示例形态：

```text
tracehub \
  --source name=core0,type=trice,channel=0,til=build/core0_til.json,framing=tcobsv1 \
  --source name=core1,type=trice,channel=1,til=build/core1_til.json,framing=tcobsv1 \
  --swimlane \
  --log-level info
```

兼容现有文本日志时使用：

```text
tracehub \
  --source name=linux,type=text,channel=0 \
  --source name=rtos,type=text,channel=1 \
  --swimlane
```

`--linux` 和 `--rtos` 旧选项可以作为外层解析入口转换成 source descriptor，但内部状态必须统一落到 source 列表。

### 8.2 编译前命令

`tracehub-trice-id` 作为固件构建前工具使用，命令语义只围绕源码和 ID 表，不承担日志采集职责。

推荐形态：

```text
tracehub-trice-id insert \
  --source-root firmware/src \
  --til build/trice/til.json \
  --li build/trice/li.json \
  --id-range core0=1000:1999 \
  --id-range core1=2000:2999
```

补齐源码维护流程后，命令入口保持一致：

```text
tracehub-trice-id clean --source-root firmware/src --til build/trice/til.json --li build/trice/li.json
tracehub-trice-id add --source-root firmware/src --til build/trice/til.json --li build/trice/li.json
tracehub-trice-id generate --til build/trice/til.json --output build/trice/trice_id_table.c
```

`tracehub-trice-id` 的返回码必须直接表达构建前检查结果：成功返回 0，任何 ID 冲突、源码解析失败、表文件不一致和格式参数不匹配都返回非零。

### 8.3 运行时 Trice 选项矩阵

运行时只迁移 Go `log` 命令中会影响 TraceHub 解码和显示的选项。阶段二必须按下表实现或拒绝，不能隐式忽略。

| Go 选项语义 | C runtime 配置 | 阶段二要求 |
| --- | --- | --- |
| `framing` / `packageFraming` | `framing=none,cobs,tcobsv1` | 支持；`tcobsv2` 阶段二拒绝 |
| endianness | `endian=little,big` | 支持 |
| `til` / `li` | `til=...`、`li=...` | `til` 必需；`li` 可选 |
| `logLevel`、`pick`、`ban` | `log-level=...`、`pick=...`、`ban=...` | 支持；`pick` 和 `ban` 同时设置时报错 |
| `defaultTRICEBitwidth` | `default-bit-width=8,16,32,64` | 支持或在 generated table 中固化 |
| target timestamp size | `target-timestamp=none,16,32` | 支持实际固件模式；未知模式拒绝 |
| `doubled16BitID` | `doubled-16-bit-id=true,false` | 若固件使用则必须支持；否则显式拒绝 |
| `singleFraming` | `single-framing=true,false` | 支持或拒绝，行为必须明确 |
| `noCycleCheck` | `cycle-check=true,false` | 支持；关闭时仍记录诊断计数 |
| `addNL` | decoder 输出策略 | TraceHub 统一管理换行，runtime decoder 不直接追加终端换行 |
| XTEA password/key | `password=...` 或 `key=...` | 阶段五支持；阶段二拒绝 |
| X0 selector mode | `x0-mode=...` | 阶段五支持；阶段二拒绝 |

## 9. 验证计划

### 9.1 运行时解码验证

1. 准备固定 Trice 字节流和匹配 ID 表，验证输出文本完全一致。
2. 覆盖 none、COBS、TCOBSv1 中实际启用的帧格式。
3. 覆盖 S0、S2、S4、整型、无符号、十六进制、浮点、字符串、布尔参数。
4. 覆盖 cycle counter 不连续、未知 ID、payload 长度错误、格式字符串不匹配。
5. 验证两个 source 交错输入后按时间戳稳定合并。

### 9.2 ID 工具验证

1. 输入缺失 `iD(n)` 的源码，输出带 ID 的源码和完整 ID 表。
2. 输入已有 ID 的源码，重复执行后文件内容不变化。
3. 输入 ID 冲突源码，工具必须报错。
4. 输入格式字符串变化源码，工具必须更新或拒绝不一致状态。
5. 输入多文件源码，ID 分配保持全局唯一。

### 9.3 TraceHub 回归验证

1. 文本 Linux/RTOS 日志路径保持原输出。
2. SystemView 输出路径不受 Trice 模块影响。
3. RTT 通道冲突检查对 text、trice、systemview 一致生效。
4. swimlane 输出列名来自 source descriptor。

### 9.4 Go 与 C 黄金样例验证

1. 从当前 Go `trice` 工具生成一组固定输入和期望输出，作为 C runtime 的黄金样例。
2. 黄金样例同时覆盖 `til.json` 加载路径和 generated C table 静态表路径。
3. 每个样例保存原始 bytes、framing、endianness、TIL、可选 LI 和期望文本输出。
4. 对 TCOBSv2、XTEA、X0 等阶段二未支持能力，验证配置阶段必须失败。
5. 对 malformed frame、unknown ID、payload length mismatch、format count mismatch，验证错误信息包含 source、channel、frame offset 和 ID。

## 10. 风险和约束

1. C/C++ 源码扫描不能只依赖正则表达式，必须正确处理注释、字符串、宏续行和嵌套括号。
2. ID 表是单一真相源，源码改写、运行时解码和生成表必须共享同一份语义模型。
3. 运行时 JSON 解析会增加部署复杂度。若 TraceHub 希望减少运行时依赖，应使用 `generate` 生成 C 查找表。
4. Trice 格式化必须严格检查参数数量和宽度，不允许在 payload 不匹配时继续输出。
5. 多 source 合并必须保留每个 source 的顺序和诊断信息，不能只按文本内容排序。
6. 日志等级规则必须集中定义，不允许在两个核心或多个模块中重复维护 tag 映射。
7. generated C table 是运行时解码的规范化输入，必须包含所有运行时需要的派生字段，避免 runtime 重复解析和重复推导。
8. TCOBSv2、XTEA、X0 等延期能力必须显式拒绝，不允许通过默认值或 fallback 掩盖配置错误。

## 11. 推荐落地顺序

1. 重构 TraceHub source 模型，保持文本日志路径行为一致。
2. 实现 TraceHub 运行时 Trice 解码，验证两个核心 Trice 日志合并。
3. 新增独立可执行文件 `tracehub-trice-id`，实现 C 版 `insert` 和 ID 表维护，移除 Go 工具依赖。
4. 实现 `clean`、`add`、`generate`，覆盖源码维护流程。
5. 根据固件实际配置补齐 XTEA、X0、ANSI 和高级过滤能力。

完成以上工作后，用户使用 Trice 的完整路径为：

```text
tracehub-trice-id updates firmware source and ID table
  -> firmware builds with Trice/src
  -> two cores emit Trice binary logs
  -> TraceHub reads RTT/shared-memory channels
  -> TraceHub decodes, filters, merges, displays and writes logs
```

## 12. 文件级实施指引

本节把实施范围落实到文件。执行时应按阶段推进，每个阶段完成后通过代码逻辑检查确认接口一致。编译和测试验证由用户执行。

### 12.1 阶段一：建立统一 source 模型

目标：把 TraceHub 从固定 Linux/RTOS 两路文本日志改成表驱动 source 模型。此阶段不引入 Trice 二进制解码，只让现有文本路径通过新 source 模型运行。

#### 新增文件

| 文件 | 放置位置 | 功能 |
| --- | --- | --- |
| `TraceSource.h` | `TraceHub/Core` | 定义 `TraceHubSource_t`、source 类型、source id、source 名称、RTT 通道、输出前缀、解码器类型和 source 上限 |
| `TraceSource.c` | `TraceHub/Core` | 提供 source 初始化、名称校验、通道冲突检查、默认 source 构造、旧 Linux/RTOS 选项到 source 的转换 |
| `TraceDecoder.h` | `TraceHub/Log` | 定义统一解码器接口，包含 init、feed、flush、cleanup 和输出 `LogEntry_t` 的回调 |
| `TextTraceDecoder.c` | `TraceHub/Log` | 封装现有 `LogLineParser` 和文本行缓冲逻辑，使文本日志成为一种 decoder |
| `TextTraceDecoder.h` | `TraceHub/Log` | 暴露文本 decoder 创建和配置接口 |

#### 修改文件

| 文件 | 当前问题 | 修改内容 | 完成标准 |
| --- | --- | --- | --- |
| `TraceHub/App/Options.h` | `TraceHubOptions_t` 固定保存 `linux_requested`、`rtos_requested`、`linux_channel`、`rtos_channel` | 增加 `TraceHubSource_t sources[]` 和 `source_count`；保留旧字段作为 CLI 兼容输入；新增 `--source` 解析结果字段 | raw options 能表达 `name=core0,type=text,channel=0` 这类 source |
| `TraceHub/App/OptionsApi.c`、`TraceHub/App/OptionsParse.c` | `_options` 只认识固定 Linux/RTOS 选项 | 增加 `--source` 长选项；新增 source 描述解析函数；旧 `--linux`、`--rtos`、`--linux-channel`、`--rtos-channel` 只作为 legacy input | `TraceHubOptions_Parse` 输出统一 source 列表，不在下游重复推导 |
| `TraceHub/App/CLI.c` | help 文本只描述 Linux/RTOS 固定模式 | 增加 `--source name=...,type=...,channel=...` 用法；保留 legacy 示例并标注会转换为 source | 用户能从帮助信息明确 runtime source 配置方式 |
| `TraceHub/App/ServicePlan.h` | `TraceHubServicePlan_t` 固定有 linux/rtos prefix 和 fixed config | 增加 resolved source list；把 source label、channel、decoder type、log prefix 放入 plan | run loop 只消费 resolved source，不读取 legacy 字段 |
| `TraceHub/App/ServicePlan.c` | 通道冲突检查和 log prefix 构造写死 Linux/RTOS/SystemView | 使用 `TraceSource` 集中校验所有 enabled source 与 SystemView 通道；循环构造 source log prefix；旧字段只在构造 source 时使用 | 通道冲突检查对 text、trice、systemview 一致 |
| `TraceHub/App/RunLoop.h` | `TraceHubRunLoop_Config_t` 固定 `linux_channel` 和 `rtos_channel` | 增加 source list、source_count、source labels、source log prefix | `_RunSwimLaneMode` 不依赖 Linux/RTOS 字段 |
| `TraceHub/App/RunLoop.c` | `_RunSwimLaneMode` 固定初始化两路 collector、两个 merger required source、两列 swimlane | 使用 source list 初始化 `LogCollector`、`LogMerger`、`SwimLaneRenderer`；启动状态 banner 循环打印 source | 两路文本日志仍能显示，source 名称来自配置 |
| `TraceHub/Log/LogEntry.h` | `LogSource_t` 是固定 enum | 替换为数值型 `LogSourceId_t`，增加无效值和最大 source 数限制；`LogEntry_t` 保存 source id | entry 不再绑定 Linux/RTOS enum |
| `TraceHub/Log/LogEntry.c` | `LogEntry_IsValid` 和 `LogEntry_Compare` 使用 `LOG_SOURCE_MAX` | 按 source_count 之外的固定上限验证 source id；排序仍按 timestamp、source id、sequence | 同 timestamp 下排序稳定 |
| `TraceHub/Log/LogCollector.h` | `LogCollector_Config_t` 固定 `linux_channel`、`rtos_channel` | 改为 source config 数组，每个 source 包含 channel、decoder type、decoder config、pending buffer size | collector 能表达 N 个 text source |
| `TraceHub/Log/LogCollector_internal.h` | `sources[LOG_SOURCE_MAX]` 固定大小 | 使用动态 source 数组或 `TRACEHUB_MAX_SOURCES` 上限数组加 `source_count`；`LogCollector_SourceState_t` 增加 source name 和 decoder 实例 | 所有 source 状态由 config 初始化 |
| `TraceHub/Log/LogCollectorStorage.c`、`TraceHub/Log/LogCollectorConsumer.c`、`TraceHub/Log/LogCollectorError.c` | pending buffer 分配、释放、重置、consumer 注册和错误信息写死 Linux/RTOS | 把所有 Linux/RTOS 分支改成按 `source_count` 循环；错误信息用 source name | 新增 source 不需要改 state 代码 |
| `TraceHub/Log/LogCollectorRuntime.c` | `_CollectFromSource` 直接按 CR/LF 切文本行 | 改为读取 raw bytes 后调用 source 绑定的 `TraceDecoder_Feed`；文本切行迁移到 `TextTraceDecoder` | collector 不关心文本或 Trice 格式 |
| `TraceHub/Log/LogCollectorSource.c` | 文本行处理和 source 状态强耦合 | 保留文本行处理能力并迁入 `TextTraceDecoder.c`；`LogCollectorSource` 只负责 entry delivery、timestamp reserve 和 pending untimed 公共逻辑 | 文本 decoder 与 collector 边界清晰 |
| `TraceHub/Log/CoreLogRecorder.h` | config 和 stats 固定 Linux/RTOS | 改为 source config 数组；每个 source 指定 channel、name、record mode、consumer queue | recorder 只负责 RTT bytes，不承载 Linux/RTOS 语义 |
| `TraceHub/Log/CoreLogRecorder_internal.h` | 固定 `CORE_LOG_RECORDER_LINUX/RTOS`、两个线程字段 | 改为 source array 和线程数组；source 保存 record mode：text clean、raw binary、off | Trice raw bytes 不再经过文本清理路径 |
| `TraceHub/Log/CoreLogRecorderState.c` | 初始化固定两个 source 和两个文件前缀 | 按 source list 初始化；text source 写 clean text 文件，Trice source 默认不写文本文件，可选 raw dump | 二进制 Trice 不被当作文本落盘 |
| `TraceHub/Log/CoreLogRecorderRegistry.c`、`TraceHub/Log/CoreLogRecorderConsumer.c`、`TraceHub/Log/CoreLogRecorderFile.c` | `_Recorder_FindSource` 只查 Linux/RTOS | 按 source_count 查找 channel；consumer queue 和 raw bytes 保持通用 | 任意 source channel 可注册 consumer |
| `TraceHub/Log/CoreLogRecorderRuntime.c` | 固定 Linux/RTOS drain thread | 每个 enabled source 启动一个 drain thread；线程入口使用 source index | source 数变化不需要新增线程函数 |
| `TraceHub/Log/CoreLogRecorderStats.c` | stats 输出固定 linux/rtos 字段 | 输出动态 source stats；若保留公共 stats struct，应包含 source_count 和 source_stats 数组 | 诊断信息包含 source name |
| `TraceHub/Log/LogMerger.h` | `required_source[LOG_SOURCE_MAX]` 固定 | 改为 required source 数组和 source label 数组，由 config 提供 source_count | merger 能合并两个 Trice core 或更多 source |
| `TraceHub/Log/LogMerger.c` | watermark、seen、label、clean state 固定 `LOG_SOURCE_MAX` | 按 source_count 分配并循环；`LogMerger_WriteEntry` 通过 source id 查 label | merged log 文件 source 名称来自 source descriptor |
| `TraceHub/Log/SwimLaneRenderer.h` | config 没有 source 列信息 | 增加 source label 数组和 source_count；保留 width、color、stream | renderer 布局由 source 列表驱动 |
| `TraceHub/Log/SwimLaneRenderer.c` | 固定 Linux/RTOS 两列、两种颜色、两种宽度 | 把列宽、label、颜色、空列输出改成按 source_count 循环；两个 source 场景保持紧凑布局 | source id 决定列位置，不出现 Linux/RTOS 条件 |
| `TraceHub/Log/LogSwimLane.c` | 已经接收 source 字符串 | 保持接口；调用方传入动态 source label | 不需要引入 Trice 条件 |
| `TraceHub/CMakeLists.txt` | 只有 `tracehub` 一个目标和固定 source 列表 | 加入新增 Core/Log 文件；增加 include dir；不新增 `tracehub-trice-id` 入口到 runtime target | 阶段一只改变 runtime source 模型 |

#### 阶段一测试文件

| 文件 | 修改内容 |
| --- | --- |
| `TraceHub/Tests/Unit/LogCollectorTest.c` | 把固定 Linux/RTOS 测试改成 source list 测试；保留两路文本行为断言 |
| `TraceHub/Tests/Unit/LogMergerTest.c` | required source 改成动态配置；新增 source id 排序测试 |
| `TraceHub/Tests/Unit/SwimLaneRendererTest.c` | 两列测试改成 source label 驱动；新增非 Linux/RTOS label 测试 |
| `TraceHub/Tests/Unit/LogLineParserTest.c` | 不改核心断言；必要时迁移到 `TextTraceDecoder` 测试 |

### 12.2 阶段二：加入 TraceHub 运行时 Trice 解码

目标：TraceHub 读取 RTT raw bytes 后，通过 Trice decoder 输出普通 `LogEntry_t`，进入现有 merger 和 renderer。

#### 新增文件

| 文件 | 放置位置 | 功能 | 对应 Go 来源 |
| --- | --- | --- | --- |
| `TriceConfig.h` | `TraceHub/Trice` | 定义 Trice endianness、framing、timestamp、cycle、filter、default bit width、single framing、doubled ID 配置结构 | `Trice/internal/args/init.go` 中 log flags 子集 |
| `TriceFraming.h` | `TraceHub/Trice` | 声明 none、COBS、TCOBSv1 帧状态机接口；TCOBSv2 枚举保留为 unsupported | `Trice/internal/trexDecoder/trexDecoder.go` 的 `nextPackage` |
| `TriceFraming.c` | `TraceHub/Trice` | 管理 0 分隔帧、leftover buffer、decode buffer；调用 COBS/TCOBSv1 decode；TCOBSv2 直接报 unsupported | `nextPackage` |
| `TriceRecord.h` | `TraceHub/Trice` | 定义 TREX record 头解析结果：type、id、timestamp、cycle、param bytes | `trexDecoder.Read` 中 tyId/nc 解析 |
| `TriceRecord.c` | `TraceHub/Trice` | 解析 2 字节 tyId、S0/S2/S4/X0、nc count/cycle、payload 长度和 padding | `trexDecoder.Read` |
| `TriceIdTable.h` | `TraceHub/Trice` | 定义 `TriceIdTable_t`、`TriceIdEntry_t`、TIL/LI 记录结构 | `Trice/internal/id/id.go` |
| `TriceIdTable.c` | `TraceHub/Trice` | 加载、查找、销毁 ID 表；支持 generated table 输入 | `Trice/internal/id/manage.go` |
| `TriceJson.h` | `TraceHub/Trice` | 声明严格 JSON 读取接口，只服务 `til.json` 和 `li.json` | Go `encoding/json` 使用点 |
| `TriceJson.c` | `TraceHub/Trice` | 解析 `{ "123": { "Type": "...", "Strg": "..." } }` 与 LI 结构；对未知字段和非法类型报错 | `NewLut`、`NewLutLI` |
| `TriceFormat.h` | `TraceHub/Trice` | 定义格式 spec、kind、规范化结果和参数计数接口 | `Trice/internal/fmtspec/fmtspec.go` |
| `TriceFormat.c` | `TraceHub/Trice` | 解析 C printf 子集，处理 `%zu`、`%lu`、`%llx`、`%i`、`%t`、`%p`、`%s` | `fmtspec.Normalize`、`decoder.UReplaceN` |
| `TriceType.h` | `TraceHub/Trice` | 定义 Trice type 解析结果：bit width、param count、special kind | `id.ConstructFullTriceInfo` |
| `TriceType.c` | `TraceHub/Trice` | 从 `TRICE`、`TRICE32_2`、`TRICE_B` 等类型计算 payload 规则 | `generate.go`、`trexDecoder.sprintTrice` |
| `TriceFormatter.h` | `TraceHub/Trice` | 声明 payload 到文本的格式化接口 | `trexDecoder.unSignedOrSignedOut` |
| `TriceFormatter.c` | `TraceHub/Trice` | 按 bit width、format spec 和 endian 生成文本；覆盖 S0/S2/S4 常规参数 | `trexDecoder` formatter functions |
| `TriceSpecial.h` | `TraceHub/Trice` | 声明 S/N/B/F 特殊 Trice 类型处理；阶段二只启用固件实际使用的基础形式 | `triceS`、`triceN`、`trice8B` 等 |
| `TriceSpecial.c` | `TraceHub/Trice` | 处理 dynamic string、dynamic buffer、function style 输出；未支持 special kind 必须报错 | `trexDecoder` special handlers |
| `TriceTag.h` | `TraceHub/Trice` | 定义 tag、log level、pick、ban 过滤结构 | `lineTransformerANSI.go` |
| `TriceTag.c` | `TraceHub/Trice` | 识别 `err:`、`wrn:`、`info:`、`dbg:` 等 tag，映射等级和过滤 | `FindTagName`、`colorize` |
| `TriceDecoder.h` | `TraceHub/Trice` | 声明 Trice runtime decoder 创建、feed、flush、destroy | `trexDecoder.New`、`Read` |
| `TriceDecoder.c` | `TraceHub/Trice` | 串联 framing、record、ID lookup、format、tag filter，输出 `LogEntry_t` | `trexDecoder.Read` |
| `TriceDecoder_internal.h` | `TraceHub/Trice` | 内部状态结构：buffers、cycle、last timestamp、source id、table pointer | `trexDec` |
| `TriceTypeX0.h` | `TraceHub/Trice` | 可延后，声明 selector-0 处理接口 | `decoder/typeX0.go` |
| `TriceTypeX0.c` | `TraceHub/Trice` | 可延后，支持 error、ignore、counted format | `HandleTypeX0` |

#### 复用文件

| 文件 | 使用方式 |
| --- | --- |
| `Trice/src/cobsDecode.c` | 加入 TraceHub 构建或复制到 `TraceHub/Trice`，作为 COBS decode 实现 |
| `Trice/src/cobs.h` | 与 `cobsDecode.c` 同步使用 |
| `Trice/src/tcobsv1Decode.c` | 加入 TraceHub 构建或复制到 `TraceHub/Trice`，作为 TCOBSv1 decode 实现 |
| `Trice/src/tcobs.h`、`Trice/src/tcobsv1Internal.h` | 与 TCOBSv1 decode 同步使用 |

#### 修改文件

| 文件 | 修改内容 | 完成标准 |
| --- | --- | --- |
| `TraceHub/Core/TraceSource.h` | source descriptor 增加 Trice 配置字段：`til_path`、`li_path`、`framing`、`endianness`、`log_level`、`pick`、`ban`、`default_bit_width`、`target_timestamp`、`doubled_16_bit_id`、`single_framing`、`cycle_check` | source 能完整描述一个 Trice core |
| `TraceHub/App/OptionsApi.c`、`TraceHub/App/OptionsParse.c` | `--source` 解析支持 `type=trice`、`til=...`、`li=...`、`framing=...`、`endian=...`、`log-level=...` | CLI 输入能构造 Trice source |
| `TraceHub/App/ServicePlan.c` | 校验 Trice source 必须有 ID 表；校验 framing 和 endian；构造 source log prefix | 错误在 plan 阶段暴露 |
| `TraceHub/Log/TraceDecoder.h` | 增加 Trice decoder factory 注册或明确 switch | collector 通过 decoder type 创建实例 |
| `TraceHub/Log/LogCollectorStorage.c` | 初始化 source 时创建 text 或 Trice decoder；cleanup 时释放 decoder | decoder 生命周期归 source state |
| `TraceHub/Log/LogCollectorRuntime.c` | raw bytes 送入 `TraceDecoder_Feed`，decoder 自己决定何时输出 entry | Trice 二进制不经过 CR/LF 切分 |
| `TraceHub/CMakeLists.txt` | 增加 `TRACEHUB_TRICE_DIR`、Trice decoder sources、COBS/TCOBSv1 C sources、include dir | runtime target 包含 Trice 解码模块 |

#### 阶段二测试文件

| 文件 | 类型 | 覆盖内容 |
| --- | --- | --- |
| `TraceHub/Tests/Unit/TriceFormatTest.c` | 新增 | 格式 spec 计数、C length modifier、bool、pointer、string、float |
| `TraceHub/Tests/Unit/TriceTypeTest.c` | 新增 | `TRICE`、`TRICE32_2`、`TRICE_B`、`TRICE_F` type 解析 |
| `TraceHub/Tests/Unit/TriceFramingTest.c` | 新增 | none、COBS、TCOBSv1 frame decode 和 partial frame |
| `TraceHub/Tests/Unit/TriceRecordTest.c` | 新增 | S0/S2/S4、unknown type、short packet、cycle count |
| `TraceHub/Tests/Unit/TriceIdTableTest.c` | 新增 | `til.json` 和 generated table lookup |
| `TraceHub/Tests/Unit/TriceDecoderTest.c` | 新增 | 给定 bytes + TIL 输出确定文本 |
| `TraceHub/Tests/Unit/TriceTagTest.c` | 新增 | tag 到等级映射、pick、ban、unclassified |

#### 阶段二配置完成标准

1. `TriceConfig` 明确记录每个影响解码的选项，不能依赖散落在 decoder 内部的隐式默认值。
2. `ServicePlan` 对 unsupported 配置集中报错，包括 TCOBSv2、XTEA、X0、未知 timestamp 模式和未知 ID 模式。
3. `default_bit_width` 要么来自 source 配置，要么由 generated C table 固化，不能在 runtime 多处重复推导。
4. `pick` 和 `ban` 同时出现时直接报错，和 Go 工具行为保持一致。
5. decoder 输出不包含终端换行控制，换行和文件输出由 TraceHub 显示与落盘层统一处理。

### 12.3 阶段三：新增独立编译前工具 `tracehub-trice-id`

目标：替代 Go `trice insert`，让固件构建前 ID 管理完全由 C 工具完成。该工具是独立 executable，不进入 `tracehub` runtime CLI。

#### 新增文件

| 文件 | 放置位置 | 功能 | 对应 Go 来源 |
| --- | --- | --- | --- |
| `main.c` | `TraceHub/Tools/TriceIdTool` | `tracehub-trice-id` 入口，分发 `insert`、`clean`、`add`、`generate` | `Trice/cmd/trice/main.go`、`args/handler.go` |
| `TriceIdOptions.h` | `TraceHub/Tools/TriceIdTool` | 定义工具 CLI options：src、exclude、til、li、id range、aliases、saliases、dry-run、verbose、default bit width、stamp size、LI path kind、add param count | `args/init.go` |
| `TriceIdOptions.c` | `TraceHub/Tools/TriceIdTool` | 解析 `insert/clean/add/generate` 的命令行参数，严格拒绝未知选项，明确记录不迁移的 Go 选项 | `flagsRefreshAndUpdate`、`insertIDsInit` |
| `TriceFileWalk.h` | `TraceHub/Tools/TriceIdTool` | 声明 source root 遍历、exclude、source file 后缀过滤 | `id/srcs.go`、`id/Update.go` |
| `TriceFileWalk.c` | `TraceHub/Tools/TriceIdTool` | 遍历 `.c/.h/.cpp/.hpp/.inc` 等源文件，按显式列表处理 | `isSourceFile`、`CompactSrcs` |
| `TriceSourceScan.h` | `TraceHub/Tools/TriceIdTool` | 声明源码扫描器和 `TriceMacroUse_t` | `id/match.go` |
| `TriceSourceScan.c` | `TraceHub/Tools/TriceIdTool` | 解析源码，处理注释、字符串、字符字面量、宏续行、嵌套括号、`TRICE_INSERT_OFF/ON` | `matchTrice`、`maskTriceInsertDisabledRegions` |
| `TriceMacro.h` | `TraceHub/Tools/TriceIdTool` | 定义宏调用信息：type、id kind、id value、format string、line、replace range | `TriceFmt`、`triceIDParse` |
| `TriceMacro.c` | `TraceHub/Tools/TriceIdTool` | 解析 `iD/Id/ID/id`、别名、SAlias、参数计数 | `Update.go`、`helper.go` |
| `TriceIdState.h` | `TraceHub/Tools/TriceIdTool` | 定义 ID 状态：idToTrice、triceToId、idToLocRef、idToLocNew | `idData` |
| `TriceIdState.c` | `TraceHub/Tools/TriceIdTool` | 读写状态、构造 reverse map、合并 LI、事务式更新 | `switchIDs.go`、`manage.go` |
| `TriceIdRange.h` | `TraceHub/Tools/TriceIdTool` | 定义 ID range 和 tag range 数据结构 | `TagEntry` |
| `TriceIdRange.c` | `TraceHub/Tools/TriceIdTool` | 解析 `--id-range tag=min:max`，检查重叠，生成可用 ID 空间 | `EvaluateIDRangeStrings` |
| `TriceIdAlloc.h` | `TraceHub/Tools/TriceIdTool` | 声明 random、upward、downward ID 分配 | `manage.go` |
| `TriceIdAlloc.c` | `TraceHub/Tools/TriceIdTool` | 按 tag 和搜索策略分配 ID，复用 LI 中同文件同位置 ID | `newID`、`newUpwardID`、`newDownwardID` |
| `TriceIdInsert.c` | `TraceHub/Tools/TriceIdTool` | 实现 `insert`，给缺失或冲突 ID 写入 `iD(n)`，更新 TIL/LI | `insertIDs.go` |
| `TriceIdClean.c` | `TraceHub/Tools/TriceIdTool` | 实现 `clean`，移除或清零 ID，更新 TIL/LI | `cleanIDs.go`、`zeroIDs.go` |
| `TriceIdAdd.c` | `TraceHub/Tools/TriceIdTool` | 实现 `add`，读取已有 ID，只更新 TIL/LI，不写源码 | `addIDs.go` |
| `TriceIdGenerate.c` | `TraceHub/Tools/TriceIdTool` | 实现 `generate`，从 TIL 生成 C table 或 header | `generate.go`、`generateTil.go` |
| `TriceIdPath.h` | `TraceHub/Tools/TriceIdTool` | 定义 LI path 策略：base、relative、full | `ToLIPath` |
| `TriceIdPath.c` | `TraceHub/Tools/TriceIdTool` | 计算写入 `li.json` 的路径字符串 | `ToLIPath` |
| `TriceAtomicFile.h` | `TraceHub/Tools/TriceIdTool` | 声明原子写文件接口 | `atomicWriteFile` |
| `TriceAtomicFile.c` | `TraceHub/Tools/TriceIdTool` | 同目录临时文件、写入、flush、chmod、rename；失败不破坏原文件 | `helper.go` |

#### 共享 TraceHub/Trice 文件

| 文件 | 被 runtime 使用 | 被 `tracehub-trice-id` 使用 |
| --- | --- | --- |
| `TriceIdTable.h/c` | 加载 TIL 解码 | 读写 TIL |
| `TriceJson.h/c` | 读取 TIL/LI | 读写 TIL/LI |
| `TriceFormat.h/c` | 运行时格式化前解析 spec | 源码扫描时计算参数数量 |
| `TriceType.h/c` | payload 解码规则 | `generate` 生成 bit width 和 param count |
| `TriceTag.h/c` | runtime level filter | `insert` 的 tag-specific ID range |

#### 修改文件

| 文件 | 修改内容 | 完成标准 |
| --- | --- | --- |
| `TraceHub/CMakeLists.txt` | 增加 `TRACEHUB_TRICE_DIR`、`TRACEHUB_TRICE_ID_TOOL_DIR`；把共享 Trice 源编成 `tracehub_trice_common` object library；新增 executable `tracehub-trice-id`；安装 runtime 和 tool 两个目标 | `tracehub` 和 `tracehub-trice-id` 入口独立 |
| `TraceHub/Tests/CMakeLists.txt` | 新增工具侧 unit test target；将共享 Trice 模块测试加入 unit tests | 工具模块能单独测试 |

#### `insert` 子命令文件流程

```text
TriceIdOptions_Parse
  -> TriceIdState_Load
  -> TriceIdRange_Build
  -> TriceFileWalk_Run
  -> TriceSourceScan_File
  -> TriceIdInsert_ProcessMacro
  -> TriceAtomicFile_Write source when modified
  -> TriceIdState_Save til.json and li.json
```

`TriceIdInsert_ProcessMacro` 必须覆盖以下规则：

1. 源码 ID 非零且 TIL 中同 ID 指向相同 Trice 格式：复用该 ID。
2. 源码 ID 非零但 TIL 中同 ID 指向不同 Trice 格式：报错或分配新 ID，并更新源码。
3. 源码 ID 为 0 或缺失，且 TIL/LI 中有同文件同位置同格式可复用 ID：复用旧 ID。
4. 源码 ID 为 0 或缺失，且没有可复用 ID：按 tag range 分配新 ID。
5. 参数数量和格式字符串不匹配：严格报错，不写源码和表文件。
6. 同一轮处理中出现重复 ID：严格报错。

`insert` 和 `clean` 还必须保留 Go 工具对 `triceConfig.h` 的副作用：`insert` 将 `#define TRICE_CLEAN 1` 切换为 `0`，`clean` 将 `#define TRICE_CLEAN 0` 切换为 `1`。该修改必须通过源码扫描和原子写文件路径完成，不能用脚本式替换绕过上下文校验。

### 12.4 阶段四：补齐 `clean`、`add`、`generate`

目标：覆盖 Trice 常用源码维护工作流，使用户不需要 Go 工具完成 ID 生命周期。

| 子命令 | 实现文件 | 功能 | 必须共享的模块 |
| --- | --- | --- | --- |
| `clean` | `TraceHub/Tools/TriceIdTool/TriceIdClean.c` | 扫描源码中已有 ID，验证 TIL 一致性，将 `iD(n)` 清零或移除，重建 LI | `TriceSourceScan`、`TriceMacro`、`TriceIdState`、`TriceAtomicFile` |
| `add` | `TraceHub/Tools/TriceIdTool/TriceIdAdd.c` | 扫描已有 ID，只更新 TIL/LI，不改写源码 | `TriceSourceScan`、`TriceIdState` |
| `generate` | `TraceHub/Tools/TriceIdTool/TriceIdGenerate.c` | 从 TIL 生成 `trice_id_table.h/.c`，供 TraceHub runtime 或外部工具静态链接 | `TriceIdTable`、`TriceType`、`TriceFormat` |

完成标准：

1. 三个子命令不重复实现源码扫描。
2. 三个子命令不重复实现 JSON 读写。
3. 三个子命令对格式数量、ID 冲突、非法路径执行同一套错误策略。
4. `generate` 输出的 C table 能被 `TriceIdTable` runtime loader 使用。

#### generated C table schema

`generate` 输出的 C table 是运行时的规范化输入，必须包含下列字段。字段名称可以按项目风格调整，但语义必须完整。

| 字段 | 来源 | 用途 |
| --- | --- | --- |
| `id` | `til.json` key | runtime 按 ID 查找记录 |
| `type` | TIL `Type` | 保留原始 Trice type，诊断和一致性检查使用 |
| `format_string` | TIL `Strg` | 输出文本和 tag 提取使用 |
| `bit_width` | `TriceType` 派生 | payload 读数宽度，避免 runtime 重复解析 type |
| `param_count` | `TriceType` 和 `TriceFormat` 派生 | payload 长度校验和源码参数数量校验 |
| `special_kind` | `TriceType` 派生 | 区分常规、S、N、B、F、X0 等处理路径 |
| `tag` | `TriceTag` 派生 | 日志等级和 pick/ban 过滤 |
| `location` | 可选 LI | 诊断输出使用 |

### 12.5 阶段五：高级 Trice 能力

| 能力 | 新增文件 | 修改文件 | 触发条件 |
| --- | --- | --- | --- |
| XTEA 解密 | `TraceHub/Trice/TriceXtea.h`、`TraceHub/Trice/TriceXtea.c`、`TraceHub/Trice/TriceSha1.h`、`TraceHub/Trice/TriceSha1.c` | `TriceDecoder.c`、`App/OptionsApi.c`、`App/OptionsParse.c` | 固件启用 Trice XTEA 加密，且用户传入 password 或 key |
| `TRICE_X0` | `TraceHub/Trice/TriceTypeX0.h`、`TraceHub/Trice/TriceTypeX0.c` | `TriceRecord.c`、`TriceDecoder.c` | 固件输出 selector-0 extension record |
| ANSI 着色 | `TraceHub/Trice/TriceAnsi.h`、`TraceHub/Trice/TriceAnsi.c` | `SwimLaneRenderer.c`、`TriceTag.c` | 需要按 Trice tag 给终端显示上色 |
| tag 统计 | `TraceHub/Trice/TriceTagStats.h`、`TraceHub/Trice/TriceTagStats.c` | `TriceTag.c`、`RunLoop.c` | 需要输出 tag 计数统计 |

## 13. Go 到 C 的迁移映射

本节按计划全部落地后的目标状态描述 Go 工具能力迁移边界。迁移结果分为三类：进入 `tracehub` 运行时、进入独立编译前工具 `tracehub-trice-id`、不迁移。

### 13.1 迁移到 C 的能力

| 原 Go 能力 | 迁移后位置 | 状态 |
| --- | --- | --- |
| `insert` | `TraceHub/Tools/TriceIdTool/TriceIdInsert.c` | 迁移到 `tracehub-trice-id` |
| `clean` / `zeroIDs` | `TraceHub/Tools/TriceIdTool/TriceIdClean.c` | 迁移到 `tracehub-trice-id` |
| `add` | `TraceHub/Tools/TriceIdTool/TriceIdAdd.c`、`TraceHub/Tools/TriceIdTool/TriceIdPath.c` | 迁移到 `tracehub-trice-id` |
| `generate` | `TraceHub/Tools/TriceIdTool/TriceIdGenerate.c` | 迁移到 `tracehub-trice-id` |
| TIL/LI JSON 管理 | `TraceHub/Trice/TriceIdTable.c`、`TraceHub/Trice/TriceJson.c`、`TraceHub/Tools/TriceIdTool/TriceIdState.c` | 迁移为 runtime 和 tool 共享能力 |
| ID 分配、ID range、tag range | `TraceHub/Tools/TriceIdTool/TriceIdAlloc.c`、`TraceHub/Tools/TriceIdTool/TriceIdRange.c` | 迁移到 `tracehub-trice-id` |
| TRICE 宏扫描、format 解析、ID 参数定位 | `TraceHub/Tools/TriceIdTool/TriceSourceScan.c`、`TraceHub/Tools/TriceIdTool/TriceMacro.c` | 迁移到 `tracehub-trice-id` |
| 源文件遍历、exclude、刷新已有 ID | `TraceHub/Tools/TriceIdTool/TriceFileWalk.c`、`TraceHub/Tools/TriceIdTool/TriceSourceScan.c` | 迁移到 `tracehub-trice-id` |
| 原子写回源码 | `TraceHub/Tools/TriceIdTool/TriceAtomicFile.c` | 迁移到 `tracehub-trice-id` |
| C printf format parser | `TraceHub/Trice/TriceFormat.c` | 迁移为 runtime 和 tool 共享能力 |
| TREX 二进制解码 | `TraceHub/Trice/TriceDecoder.c`、`TraceHub/Trice/TriceRecord.c` | 迁移到 `tracehub` 运行时 |
| COBS/TCOBSv1 framing | `TraceHub/Trice/TriceFraming.c`，复用 `Trice/src/cobsDecode.c`、`Trice/src/tcobsv1Decode.c` | 阶段二迁移 none、COBS、TCOBSv1；TCOBSv2 阶段二不迁移 |
| endian、payload value 转换 | `TraceHub/Trice/TriceDecoder.c`、`TraceHub/Trice/TriceFormatter.c` | 迁移到 `tracehub` 运行时 |
| selector-0 / X0 | `TraceHub/Trice/TriceTypeX0.c` | 迁移到 `tracehub` 运行时 |
| tag、logLevel、pick、ban | `TraceHub/Trice/TriceTag.c`，可选 `TraceHub/Trice/TriceAnsi.c` | 迁移到 `tracehub` 运行时 |
| XTEA password 解密 | `TraceHub/Trice/TriceXtea.c`、`TraceHub/Trice/TriceSha1.c` | 阶段五迁移到 `tracehub` 运行时 |

### 13.2 部分迁移的能力

| 原 Go 能力 | 迁移情况 |
| --- | --- |
| `log` | 不迁移 Go 的完整 `log` 命令；只把 Trice 运行时解码参数迁移到 `tracehub --source`，包括 framing、endianness、TIL/LI、logLevel、pick、ban、default bit width、timestamp、cycle check、doubled ID、single framing。password/key 阶段五迁移。RTT/shared-memory 接入仍由 TraceHub 自己负责。 |
| `scan` | 迁移底层扫描能力，用于 `insert/clean/add/generate`；不保留 Go 风格的独立 `scan` 子命令。若后续需要暴露用户命令，应作为 `tracehub-trice-id scan` 单独设计。 |

### 13.3 不迁移的能力

| 原 Go 模块 / 功能 | 不迁移原因 |
| --- | --- |
| `Trice/internal/com` | TraceHub 使用自己的 RTT/shared-memory 输入，不使用 Trice Go 的串口接入 |
| `Trice/internal/receiver` | 接收链路由 TraceHub 负责 |
| `Trice/internal/link` | 不使用 Trice Go 的 J-Link/连接管理 |
| `displayServer` | TraceHub 不需要 Go displayServer |
| `Trice/internal/emitter/remoteDisplay.go`、`Trice/internal/emitter/server.go` | 不迁移远端显示服务 |
| `Trice/internal/charDecoder` | 目标是 Trice TREX 二进制日志，不迁移字符解码路径 |
| `Trice/internal/dumpDecoder` | 目标不是 dump 解码 |
| Go CLI 外壳 | C 版本重新设计为 `tracehub` 和 `tracehub-trice-id` 两个入口，不保留 Go 命令结构 |

### 13.4 Go 文件到 C 文件映射

| Go 文件 | 迁移目标 | 迁移内容 |
| --- | --- | --- |
| `Trice/internal/args/handler.go` | `TraceHub/Tools/TriceIdTool/main.c`、`TriceIdOptions.c`、`TraceHub/App/OptionsApi.c`、`TraceHub/App/OptionsParse.c` | `insert/clean/add/generate` 进入 `tracehub-trice-id`；`log` 的 Trice runtime flags 子集进入 `tracehub --source` |
| `Trice/internal/args/init.go` | `TriceIdOptions.c`、`App/OptionsApi.c`、`App/OptionsParse.c` | ID range、src/exclude、til/li、framing、endianness、logLevel、pick、ban、default bit width、stamp size、target timestamp、doubled ID、single framing、cycle check、LI path kind、aliases、saliases、dry-run |
| `Trice/internal/id/id.go` | `TraceHub/Trice/TriceIdTable.h` | ID、format、LI 数据结构 |
| `Trice/internal/id/manage.go` | `TraceHub/Trice/TriceIdTable.c`、`TriceIdState.c`、`TriceIdAlloc.c` | TIL/LI JSON、reverse lookup、ID 分配、AddFmtCount |
| `Trice/internal/id/switchIDs.go` | `TriceIdState.c`、`TriceIdRange.c`、`TriceIdAlloc.c` | 处理前状态、ID range、tag range、处理后写回 |
| `Trice/internal/id/helper.go` | `TriceSourceScan.c`、`TriceMacro.c`、`TriceAtomicFile.c` | disabled regions、别名、SAlias、参数数量解析、原子写文件 |
| `Trice/internal/id/match.go` | `TriceSourceScan.c` | TRICE 宏定位、format string、ID 参数位置 |
| `Trice/internal/id/Update.go` | `TriceFileWalk.c`、`TriceMacro.c` | source 文件过滤、刷新已有 ID、格式数量校验 |
| `Trice/internal/id/insertIDs.go` | `TriceIdInsert.c` | `insert` 核心算法，包含 `TRICE_CLEAN` 从 1 到 0 的配置切换 |
| `Trice/internal/id/cleanIDs.go`、`zeroIDs.go` | `TriceIdClean.c` | `clean` 和 zero ID 逻辑，包含 `TRICE_CLEAN` 从 0 到 1 的配置切换 |
| `Trice/internal/id/addIDs.go` | `TriceIdAdd.c`、`TriceIdPath.c` | `add` 和 LI path 规则 |
| `Trice/internal/id/generate.go`、`generateTil.go` | `TriceIdGenerate.c`、`TriceType.c` | C table/header 生成、bit width、param count、special kind、tag 派生字段 |
| `Trice/internal/fmtspec/fmtspec.go` | `TraceHub/Trice/TriceFormat.c` | C printf spec parser 和 normalization |
| `Trice/internal/decoder/decoder.go` | `TriceFormat.c`、`TriceFormatter.c`、`TriceDecoder.c` | endian 读数、format spec kind、payload value 转换 |
| `Trice/internal/decoder/typeX0.go` | `TriceTypeX0.c` | selector-0 counted payload |
| `Trice/internal/trexDecoder/trexDecoder.go` | `TriceFraming.c`、`TriceRecord.c`、`TriceDecoder.c`、`TriceFormatter.c`、`TriceSpecial.c` | TREX framing、record parse、ID lookup、payload format、special types |
| `Trice/internal/emitter/lineTransformerANSI.go` | `TriceTag.c`、`TriceAnsi.c` | tag table、log level、pick、ban、可选 ANSI |
| `Trice/pkg/cipher/cipher.go` | `TriceXtea.c`、`TriceSha1.c` | password 到 key、8 字节 swap、XTEA decrypt |
| `Trice/internal/com`、`receiver`、`link` | 不迁移 | TraceHub 使用 RTT/shared-memory，不使用 Trice 串口/J-Link 接入 |
| `Trice/internal/emitter/server.go`、`remoteDisplay.go` | 不迁移 | TraceHub 不需要 Go displayServer |
| `Trice/internal/dumpDecoder`、`charDecoder` | 不迁移 | 当前目标是 Trice TREX 二进制日志 |

## 14. 实施时的硬性边界

1. `tracehub` runtime 入口不能包含源码改写逻辑。
2. `tracehub-trice-id` 不能包含 RTT 采集、LogMerger、SwimLaneRenderer 逻辑。
3. TIL/LI、format spec、tag table 必须是共享模块，不能 runtime 和 tool 各写一套。
4. 源码扫描只能集中在 `TriceSourceScan.c`，禁止在 `insert/clean/add` 中各写一套扫描逻辑。
5. Trice 二进制 raw bytes 不能经过文本清理函数。
6. 所有 source 名称、通道、decoder type 必须在 `TraceSource` 和 `ServicePlan` 阶段解析完成，下游只消费 resolved source。
7. 所有错误信息必须带 source name 或文件路径；runtime 错误带 channel 和 frame/ID，tool 错误带 source file 和 line。
8. `TraceSource` 只能位于公共基础层，`Log`、`Trice` 和 `Tools` 模块不能包含 `App` 头文件。
9. generated C table 必须由 `TriceIdTable` 统一加载，runtime 不能另写一套静态表访问规则。
10. 未支持的 Trice 特性必须在 `ServicePlan` 或 `tracehub-trice-id` 参数解析阶段报错，禁止使用 fallback 掩盖。
11. 每阶段完成后需要用户执行对应构建或测试命令验证，AI 不执行编译和测试命令。
