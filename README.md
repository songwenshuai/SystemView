# TraceHub - RTT Trace and Debug Bridge

TraceHub 是一个运行在主机用户空间的 RTT (Real-Time Transfer) trace 和调试桥接工具，专为异构多核系统（Linux A53 + RTOS R5）设计。它通过后端映射共享 RTT 内存，将嵌入式系统的调试日志以多种方式呈现给开发者；Linux 支持 SMEM SharedMem 硬件后端和 MEMSHM 仿真后端，macOS 原生运行支持 MEMSHM 仿真后端。

---

## 目录

- [系统架构](#系统架构)
- [数据流管道](#数据流管道)
- [核心模块](#核心模块)
  - [LogEntry - 日志条目数据模型](#logentry---日志条目数据模型)
  - [LogCollector - 日志采集模块](#logcollector---日志采集模块)
  - [LogMerger - 时序合并模块](#logmerger---时序合并模块)
  - [SwimLaneRenderer - 泳道渲染模块](#swimlanerenderer---泳道渲染模块)
- [服务层](#服务层)
  - [Terminal 服务](#terminal-服务)
  - [核心日志记录](#核心日志记录)
  - [SystemView 服务](#systemview-服务)
- [底层基础设施](#底层基础设施)
  - [RTTBridge - 桥接核心](#rttbridge---桥接核心)
  - [RTTMemory - 内存访问抽象](#rttmemory---内存访问抽象)
  - [RTT 协议](#rtt-协议)
- [使用方法](#使用方法)
  - [命令行参数](#命令行参数)
  - [默认行为与模式选择](#默认行为与模式选择)
  - [使用示例](#使用示例)
  - [服务初始化顺序](#服务初始化顺序)
- [时间同步要求](#时间同步要求)
- [线程安全说明](#线程安全说明)
- [事实约束与验收边界](#事实约束与验收边界)
- [构建说明](#构建说明)
- [自动化 smoke 验证](#自动化-smoke-验证)

---

## 系统架构

```
+------------------------------------------------------------------+
|                    Host (Linux Userspace)                        |
|                                                                  |
|  +------------------------------------------------------------+  |
|  |                       TraceHub                             |  |
|  |                                                            |  |
|  |  +-------------------+     +---------------------------+  |  |
|  |  | LogCollector      |     | RTTBridge                 |  |  |
|  |  |  Linux Thread     |     | (CB Discovery & Validate) |  |  |
|  |  |  RTOS Thread      |     +---------------------------+  |  |
|  |  +--------+----------+                                    |  |
|  |           |                                               |  |
|  |           v                                               |  |
|  |  +-------------------+     +---------------------------+  |  |
|  |  | LogMerger         |     | Terminal Service          |  |  |
|  |  | (Sort by time)    |     | (Telnet / Console /       |  |  |
|  |  +--------+----------+     |  Swimlane)                |  |  |
|  |           |                +---------------------------+  |  |
|  |           v                                               |  |
|  |  +-------------------+     +---------------------------+  |  |
|  |  | SwimLaneRenderer  |     | SystemView Service        |  |  |
|  |  | (Display)         |     | (Network / Local Record)  |  |  |
|  |  +-------------------+     +---------------------------+  |  |
|  +------------------------------------------------------------+  |
|                                                                  |
|                    ^ RTT Read via SMEM or POSIX shm              |
+--------------------+---------------------------------------------+
                     |
+--------------------+---------------------------------------------+
|               Shared Memory (RTT Control Block)                  |
|  +--------------------+         +--------------------+           |
|  | RTT Channel 0      |         | RTT Channel 1      |           |
|  | (Linux A53 logs)   |         | (RTOS R5 logs)     |           |
|  +--------^-----------+         +--------^-----------+           |
+-----------|------------------------------|------------------------+
            |                              |
+-----------+-----------+    +-------------+------------+
|   Linux A53 Core      |    |   RTOS R5 Core           |
|   (Write logs)        |    |   (Write logs)           |
+-----------------------+    +--------------------------+
```

---

## 数据流管道

### 泳道模式（Swimlane Mode）

```
Backend RTT Memory (RTT Control Block)
          |
          v
RTTMemory Backend (SMEM or MEMSHM)
          |
          v
RTTBridge (discovers and validates CB)
          |
          v
LogCollector (consumes CoreLogRecorder queues, 10ms interval)
          |
          v (LogEntry: timestamp_us + source + content)
    _SwimLaneCollectorCallback()
          |
          v
LogMerger_Process()
(buffer + sort by timestamp)
          |
          | (flush: default 1000ms or 4 buffered entries, configurable by CLI)
          v
    _SwimLaneMergerCallback()
          |
    +-----+-----+
    |           |
    v           v
SwimLane    Write to log file
RenderEntry
```

### 普通模式（Normal Mode）

```
RTTBridge
    |
    +----> Terminal Service ---> RTT Channel ---> TCP Client / stdin/stdout
    |
    +----> SystemView Service -> RTT Channel ---> Network Client / Local File
```

---

## 核心模块

### LogEntry - 日志条目数据模型

**文件**: `LogEntry.h` / `LogEntry.c`

**职责**: 提供统一的日志条目数据结构，是泳道显示管道中数据流转的基本单元。

#### 数据模型

每个日志条目包含以下字段：

```
+------------------+----------+------------------------------------------+
| 字段             | 类型     | 说明                                     |
+------------------+----------+------------------------------------------+
| timestamp_us     | uint64_t | 微秒级统一时间戳，来自同步时间基准         |
|                  |          | 范围: 0 ~ 2^64-1 us (约 584,942 年)     |
+------------------+----------+------------------------------------------+
| source           | enum     | 日志来源                                 |
|                  |          |   LOG_SOURCE_LINUX (0) = A53 核心        |
|                  |          |   LOG_SOURCE_RTOS  (1) = R5 核心         |
+------------------+----------+------------------------------------------+
| sequence         | uint64_t | 同一 source 内的交付序号，用于稳定排序    |
+------------------+----------+------------------------------------------+
| content          | char*    | 动态分配的日志内容字符串（null-terminated）|
|                  |          | 单个片段最大长度: LOG_ENTRY_MAX_CONTENT_LEN|
|                  |          |          = 1024 字节                     |
+------------------+----------+------------------------------------------+
| content_len      | size_t   | 实际内容字节数                           |
+------------------+----------+------------------------------------------+
| fragment_*       | bool     | 标记该条目是否为长行片段及是否还有后续片段 |
+------------------+----------+------------------------------------------+
| valid            | bool     | 条目有效性标志                           |
+------------------+----------+------------------------------------------+
```

#### 日志格式解析

LogEntry 通过解析 RTT 日志行创建。推荐每条业务日志使用规范 timestamp prefix：

```
[timestamp_us] log content
```

受支持输入格式：

```
[timestamp_us] log content
[HH:MM:SS.mmm] log content
[HH:MM:SS] log content
log content without timestamp
```

timestamp prefix 满足以下条件时会被剥离并用于排序：
- 前置空白会被忽略
- timestamp prefix 必须使用 `[]` 包裹，裸数字或裸 `HH:MM:SS` 会作为普通日志内容处理
- `timestamp_us` 必须为十进制微秒整数
- `HH:MM:SS.mmm` 中 minutes 和 seconds 必须小于 60，`mmm` 必须正好是 3 位毫秒数字
- 时间戳后面必须有至少一个空格或制表符
- `log content` 不能为空

没有受支持 timestamp prefix 的日志行会作为普通内容保留，并继承同一 source 上一次成功解析到的 timestamp。若该 source 尚未解析到任何 timestamp，这些启动阶段日志会先在 LogCollector 内部暂存，直到同一 source 的首个 timestamp prefix 到达，再统一归属到该 timestamp 后交付给 LogMerger；如果该 source 始终没有 timestamp，刷新阶段会使用确定的 fallback timestamp 输出这些日志，fallback timestamp 位于当前已观察到的最大 timestamp 之后。原始日志文件始终保持完整。超长日志行按 `LOG_ENTRY_MAX_CONTENT_LEN` 分片生成多个 LogEntry，片段使用 source-local sequence 保持原始顺序。

解析示例：

```
输入: "[10002500] [ISP] Stream On CMD Sent"
      "  probe banner"
      "[10002510] [IRQ] IPC Interrupt Received"

输出 (条目 1): timestamp_us = 10002500
               content      = "[ISP] Stream On CMD Sent"

输出 (条目 2): timestamp_us = 10002500
               content      = "probe banner"

输出 (条目 3): timestamp_us = 10002510
               content      = "[IRQ] IPC Interrupt Received"
```

#### 比较与排序

`LogEntry_Compare()` 提供全序关系，用于 LogMerger 的排序：

1. 主键：`timestamp_us`（升序）
2. 平局决胜：`source`（LINUX < RTOS）
3. 同 source 同 timestamp：`sequence`（升序）

确保同时发生的事件具有确定性顺序。

#### 内存生命周期

所有权沿管道单向传递，每个阶段负责销毁不再持有的条目：

```
LogCollector       LogMerger           SwimLaneRenderer
    |                  |                       |
    +-- Create() ----->|                       |
    |                  +-- Insert (sort)       |
    |                  +-- Flush() ----------->|
    |                  |                       +-- Render
    |                  |<------ Destroy() -----+
    |                  |                       |
```

传递路径：创建者 -> 采集器 -> 合并器 -> 渲染器 -> 销毁者。

#### 公开 API

```
LogEntry_Create()       - 创建日志条目（分配 entry 和 content 缓冲区）
LogEntry_CreateEx()     - 创建带 sequence 和片段标记的日志条目
LogEntry_Clone()        - 克隆条目（深拷贝）
LogEntry_Destroy()      - 销毁条目（释放 entry 和 content）
LogEntry_GetTimestamp() - 获取微秒时间戳
LogEntry_GetSource()    - 获取来源（LINUX / RTOS）
LogEntry_GetSequence()  - 获取 source-local sequence
LogEntry_GetContent()   - 获取内容字符串
LogEntry_Compare()      - 比较两个条目（用于排序）
LogEntry_IsValid()      - 验证条目有效性（非 NULL、valid 标志、content 非 NULL、source 合法）
```

---

### LogCollector - 日志采集模块

**文件**: `LogCollector.h` / `LogCollector.c`

**职责**: 泳道显示管道中的数据采集层。持续轮询来自 Linux 和 RTOS 核心的 RTT 通道，解析可选 timestamp prefix，并为下游处理创建 LogEntry 对象。

#### 系统上下文

LogCollector 运行在宿主机用户空间，通过 `CoreLogRecorder` 消费已落盘的核心日志字节，独立异步地处理两个核心的日志流：

```
+------------------------------------------------------------+
|                       LogCollector                         |
|  +--------------------------+  +--------------------------+|
|  | Linux Collection Thread  |  | RTOS Collection Thread   ||
|  | 消费 Linux 记录器队列     |  | 消费 RTOS 记录器队列      ||
|  | (linux_channel)          |  | (rtos_channel)           ||
|  | LOG_SOURCE_LINUX         |  | LOG_SOURCE_RTOS          ||
|  +----------+--------------+  +-----------+--------------+|
|             |                             |                |
|             +-------------+--------------+                |
|                           |                               |
|                           v                               |
|                 Callback (to LogMerger)                   |
+------------------------------------------------------------+
```

使用独立线程的原因：
- 每个通道的数据可用性相互独立
- 避免一个通道阻塞时延误另一个通道
- 最大化日志采集吞吐量

#### 采集工作流

每个采集线程执行以下循环：

```
1. Read RTT Channel
   CoreLogRecorder_ReadChannel(channel, buffer, size)
   从核心日志记录器的消费队列读取已落盘字节，无数据时返回 0

2. Trim Whitespace
   去除末尾空格、制表符、换行符
   跳过空行

3. Parse Timestamp
   从受支持的 timestamp prefix 提取时间戳
   没有 timestamp prefix 的日志行继承同一 source 的当前 timestamp
   当前 source 尚无 timestamp 时先暂存，首个 timestamp 到达后再交付
   如果刷新阶段仍无 timestamp，则使用 fallback timestamp 交付

4. Create LogEntry
   LogEntry_Create(timestamp, source, content, length)
   分配条目和内容缓冲区

5. Deliver via Callback
   callback(entry, user_data)
   回调接管所有权，无论返回值如何必须调用 LogEntry_Destroy()

6. Sleep and Repeat
   SYS_Sleep(poll_interval_ms)  // 默认 10ms
```

内部缓冲区片段大小：`LOG_COLLECTOR_MAX_LINE_LEN` = 2048 字节。单个 LogEntry 内容片段最大长度由 `LOG_ENTRY_MAX_CONTENT_LEN` 统一定义，当前为 1024 字节；超过该限制的日志行会被拆分为多个 LogEntry 片段交付，不截断、不停止泳道。只有首个 timestamp 之前的无时间戳启动日志超过 pending 暂存区时，采集器才会进入 fatal 状态，因为此时无法继续无损保存待排序内容。

#### 统一时间基准要求

带 timestamp prefix 的日志行必须：
- 使用 `[]` 包裹 timestamp，推荐规范格式为 `[timestamp_us]`
- 与另一个核心使用相同单位和相同时间基准
- 来自与另一个核心**相同**的硬件定时器（详见[时间同步要求](#时间同步要求)）

无 timestamp 日志行不会停止采集，适合 boot banner、第三方库输出和 continuation line。此类日志继承同一 source 的当前 timestamp；首个 timestamp 前的启动日志会等到同一 source 的首个 timestamp 后再输出，避免晚启动 source 产生倒序条目。若该 source 始终没有 timestamp，刷新阶段会使用位于全局已观察最大 timestamp 之后的 fallback timestamp 输出。

#### 运行模式

```
+-------------------+-----------------------------------------------+
| 模式              | 说明                                          |
+-------------------+-----------------------------------------------+
| 后台模式          | LogCollector_Start() 启动两个后台线程         |
| (Background Mode) | 持续轮询，通过回调持续交付 LogEntry 条目      |
|                   | 调用 LogCollector_Stop() 终止线程             |
+-------------------+-----------------------------------------------+
| 前台模式          | LogCollector_Poll() 对两个通道执行单次轮询    |
| (Foreground Mode) | 立即处理当前可用条目，返回采集到的条目数量    |
|                   | 调用方控制轮询频率                            |
+-------------------+-----------------------------------------------+
```

#### 配置

```c
typedef struct {
    unsigned linux_channel;      // Linux 日志的 RTT 通道号（默认 0）
    unsigned rtos_channel;       // RTOS 日志的 RTT 通道号（默认 1）
    unsigned poll_interval_ms;   // 轮询间隔，单位毫秒（默认 10ms）
} LogCollector_Config_t;
```

#### 内存管理

- 采集器为每条日志行分配 LogEntry
- 所有权转移给回调函数
- 回调**必须**调用 `LogEntry_Destroy()` 销毁条目
- 内部缓冲区（`linux_buffer`、`rtos_buffer`）复用，不重复分配

#### 公开 API

```
LogCollector_Init()           - 初始化采集器
LogCollector_Start()          - 启动后台采集（指定回调函数）
LogCollector_Stop()           - 停止后台采集线程
LogCollector_Poll()           - 手动单次轮询两个通道
LogCollector_IsRunning()      - 查询运行状态
LogCollector_HasFatalError()  - 查询 fatal 状态
```

#### 在泳道管道中的位置

```
LogCollector --> LogMerger --> SwimLaneRenderer --> Terminal/File
  (Collect)       (Sort)         (Format)
```

---

### LogMerger - 时序合并模块

**文件**: `LogMerger.h` / `LogMerger.c`

**职责**: 实现多个异步日志流的时序合并。确保来自不同核心的日志按严格时间顺序呈现，即使它们由独立的采集线程异步收集。

#### 解决的问题

在异构系统中，各核心独立写入 RTT 通道，LogCollector 的两个线程各自独立轮询。若不合并，日志将以到达顺序（而非时间顺序）呈现：

```
错误顺序（按到达顺序）:
  [Linux 100ms] -> [RTOS 50ms] -> [Linux 120ms] -> [RTOS 60ms]

正确顺序（按时间戳）:
  [RTOS 50ms] -> [RTOS 60ms] -> [Linux 100ms] -> [Linux 120ms]
```

产生乱序的原因：
- 轮询时序偏差（Polling timing skew）
- RTT 缓冲区读取延迟
- 各核心事件频率不同

#### 合并策略

合并器使用带来源水位线的有序缓冲区：

```
1. 插入 (Insertion)
   - 新 LogEntry 插入内部有序缓冲区
   - 缓冲区始终按 timestamp_us 排序
   - 插入复杂度 O(n)，n 通常较小（泳道模式默认刷新阈值为 4 条）

2. 来源水位线 (Source Watermark)
   - 合并器通过 required_source[] 明确记录参与排序的来源集合
   - 未出现的 required source 会阻塞输出，直到最早缓冲条目等待超过 flush_timeout_ms
   - 已出现但静默超过 flush_timeout_ms 的 required source 不再阻塞其他活跃来源继续输出
   - flush_timeout_ms 为 0 时禁用超时刷新，只按来源水位线或关闭刷新
   - 若新条目早于已经输出的位置，合并器直接报错，避免继续产生错误顺序

3. 刷新触发 (Flush Triggers)
   - 缓冲区达到 flush_threshold
   - 最早缓冲条目等待超过 flush_timeout_ms（超时刷新，flush_timeout_ms 为 0 时禁用）
   - 缓冲区满时只刷新已经满足水位线条件的条目；若仍无可刷新条目则报错停止
   - 显式调用 LogMerger_FlushReady()（运行期水位线刷新）
   - 显式调用 LogMerger_Flush()（关闭时强制输出剩余条目）

4. 刷新行为 (Flush Behavior)
   - 运行期 ready 条目按 timestamp_us 顺序通过回调逐条交付
   - 运行期不会为了腾出容量而强制交付未满足水位线条件的条目
   - 关闭时剩余条目按当前缓冲排序强制交付
   - 回调获得所有权并负责销毁条目；只有回调成功返回后，条目才会被记录为已交付
```

#### 缓冲时间窗口

```
Timeline:
-------------------+--------------------+-------------------------->
                   ^                    ^                    ^
                   |                    |                    |
                Oldest               Current              Newest
                Buffered             Time (now)           Buffered
```

- 新条目时间戳 < 最旧缓冲条目：若不早于已输出位置，条目仍被插入并排序
- 新条目早于已输出位置：直接报错停止，防止破坏全局时间序
- 缓冲区达到 flush_threshold：触发 ready 刷新
- 最早缓冲条目等待超过 flush_timeout_ms：触发 ready 刷新；设置为 0 时禁用超时刷新，适合需要严格等待双 source 水位线的场景
- 缓冲区满且没有 ready 条目：等待 flush_timeout_ms 使现有条目进入 ready 状态，再释放排序上不晚于待插入条目的最旧条目；如果超时刷新被禁用或等待后仍没有可释放条目，则报错停止

#### 配置

```c
typedef struct {
    unsigned    buffer_size;        // 内部排序缓冲区容量（默认 8192 条）
    unsigned    flush_threshold;    // 缓冲条目数达到此值时刷新
                                    // 泳道模式默认 4，可用 CLI 调整
    unsigned    flush_timeout_ms;   // 最早缓冲条目等待超过此时长则刷新（默认 1000ms，0 禁用）
    bool        required_source[LOG_SOURCE_MAX];
                                    // 参与排序且必须提供水位线的来源集合
    bool        log_enabled;        // 是否将合并结果写入日志文件
    const char *log_prefix;         // 日志文件名前缀
} LogMerger_Config_t;
```

#### 公开 API

```
LogMerger_Init()             - 初始化合并器
LogMerger_Insert()           - 插入单条日志条目到有序缓冲区
LogMerger_FlushReady()       - 刷新水位线以内的 ready 条目
LogMerger_Flush()            - 强制刷新所有缓冲条目（关闭时使用）
LogMerger_Process()          - 插入并触发刷新检查（组合操作）
LogMerger_GetBufferedCount() - 获取当前缓冲区中的条目数量
LogMerger_WriteEntry()       - 将已合并条目写入合并器拥有的日志文件
```

> **注意**: 本模块内部使用 `SYS_Mutex` 保护缓冲区。输出回调在锁外执行，回调函数仍需自行保证其访问的外部资源线程安全。

---

### SwimLaneRenderer - 泳道渲染模块

**文件**: `SwimLaneRenderer.h` / `SwimLaneRenderer.c`

**职责**: 将合并后的日志条目格式化为泳道表格，利用两个正交的视觉维度显示异构系统日志：

```
+------------------+---------------------------------------------+
| 视觉维度         | 编码的信息                                  |
+------------------+---------------------------------------------+
| 垂直轴           | 时间进展（时间顺序，从上到下）               |
| 水平轴           | 系统分区（左: Linux A53, 右: RTOS R5）      |
+------------------+---------------------------------------------+
```

#### 视觉格式

渲染器生成三列表格输出：

```
+--------------+------------------------------+-----------------------------+
|  TIME (us)   |  LINUX CORE (A53)            |  RTOS CORE (R5)            |
+--------------+------------------------------+-----------------------------+
|  10002500    |  [ISP] Stream On CMD Sent    |                            |
|  10002510    |                              |  [IRQ] IPC Interrupt Recv  |
|  10002525    |                              |  [DRV] Sensor stream_on()  |
|  10002530    |                              |  [IPC] ACK Sent            |
|  10002550    |  [IPC] Recv ACK (Lat: 50us)  |                            |
|  10002600    |  [BUF] Enqueue Buffer 01     |                            |
|  10002700    |                              |  [3A ] AE Converged        |
+--------------+------------------------------+-----------------------------+
```

#### 泳道显示的核心优势

```
+--------------------+--------------------------------------------------+
| 优势               | 说明                                             |
+--------------------+--------------------------------------------------+
| 时间连续性         | 所有事件严格按时间顺序显示，时间自上而下流动     |
+--------------------+--------------------------------------------------+
| 空间分离           | 左列始终显示 Linux A53 事件                      |
|                    | 右列始终显示 RTOS R5 事件                        |
|                    | 空单元格表示该核心在此时段空闲                   |
+--------------------+--------------------------------------------------+
| 交互可视化         | 跨核交互形成"阶梯"模式，请求-响应对视觉配对    |
|                    | 示例：                                           |
|                    |   CMD Sent (左) --> Interrupt Recv (右)          |
|                    |   --> ACK Sent (右) --> Recv ACK (左)            |
|                    | 延迟可通过条目间垂直距离直观感知                 |
+--------------------+--------------------------------------------------+
| 认知效率           | 人眼并行处理水平位置与内容                       |
|                    | 无需解析每行的 [LINUX] 或 [RTOS] 来源前缀       |
|                    | 可专注扫描单列跟踪单个子系统逻辑                 |
+--------------------+--------------------------------------------------+
```

#### 配置

```c
typedef struct {
    unsigned timestamp_width;    // 时间列宽，默认 SWIMLANE_DEFAULT_TIMESTAMP_WIDTH = 12
    unsigned linux_width;        // Linux 列宽，默认 SWIMLANE_DEFAULT_LINUX_WIDTH    = 128
    unsigned rtos_width;         // RTOS 列宽， 默认 SWIMLANE_DEFAULT_RTOS_WIDTH     = 128
    bool     show_header;        // 显示列标题（默认 true）
    bool     show_separator;     // 显示行分隔线（默认 true）
    bool     color_enabled;      // 启用 ANSI 颜色（默认 true）
    FILE    *output_stream;      // 输出流（默认 stdout）
} SwimLane_Config_t;
```

#### 性能说明

- 每条条目立即刷新到 `output_stream`，保证实时显示
- 列宽固定，避免动态宽度计算开销
- 内容超出列宽时按列宽换行显示
- ANSI escape sequence 按 0 显示宽度处理，`color_enabled=true` 时保留目标日志原始颜色，`color_enabled=false` 时过滤 ANSI 输出

#### 公开 API

```
SwimLane_Init()            - 初始化渲染器
SwimLane_RenderHeader()    - 渲染列标题行
SwimLane_RenderSeparator() - 渲染行分隔线
SwimLane_RenderEntry()     - 渲染单条日志条目（路由到对应列）
SwimLane_GetState()        - 获取只读状态
SwimLane_GetRowCount()     - 获取已渲染总行数
```

> **注意**: 本模块内部使用 `SYS_Mutex` 保护渲染输出，可被多个采集线程调用。

---

## 服务层

### Terminal 服务

**文件**: `Terminal.h` / `Terminal.c`

Terminal 服务提供三种互斥的显示模式：

```
+----------+----------------------------------------+-------------------+
|   模式   |              参数要求                  |     显示位置      |
+----------+----------------------------------------+-------------------+
| Console  | --console [可选: 通道参数]             | 终端 stdin/stdout |
|          | 不指定通道 -> 默认 linux_channel=0     |                   |
+----------+----------------------------------------+-------------------+
| Swimlane | --swimlane [可选: 通道参数]            | 终端双列显示      |
|          | 不指定通道 -> linux=0, rtos=1           |                   |
+----------+----------------------------------------+-------------------+
| Telnet   | --telnet-port <port> [可选: 通道参数]  | Telnet TCP 服务   |
|          | 不指定通道 -> 默认 linux_channel=0     |                   |
+----------+----------------------------------------+-------------------+
```

默认端口：`RTT_BRIDGE_DEFAULT_TERMINAL_PORT` = **19021**

Console 和 Telnet 是单通道 Terminal 模式，只消费一个核心 source：

- 指定 `--linux-channel <n>` 时消费 Linux source，通道为 `<n>`
- 指定 `--rtos-channel <n>` 时消费 RTOS source，通道为 `<n>`
- 未指定 source 通道时消费 Linux source，通道为默认 `linux_channel=0`

Console 和 Telnet 模式不允许同时指定 `--linux-channel` 与 `--rtos-channel`；需要双核心显示时应使用 Swimlane。Terminal 启动前必须校验所选通道的 RTT up-buffer，up-buffer 未配置会立即失败。down-buffer 只决定目标输入能力：配置存在时支持 stdin/Telnet 输入写回目标；配置不存在时进入只读日志模式并输出 warning，不阻断日志查看。Console 模式下 stdout 只承载目标输出，工具状态、banner 和诊断信息写入 stderr，便于脚本采集虚拟串口数据；`run-console.sh` 的 runner 状态输出也写入 stderr。

#### 配置

```c
typedef struct {
    unsigned     port;          // TCP 端口（默认 19021）
    unsigned     channel;       // RTT 通道号（默认 0）
    bool         enabled;       // 启用/禁用标志
    bool         console_mode;  // true = stdin/stdout, false = TCP
    bool         log_enabled;   // 启用文件日志记录
    const char  *log_prefix;    // 日志文件名前缀
    size_t       network_queue_size; // TCP backlog 字节数，0 使用默认 1 MiB
} Terminal_Config_t;
```

#### 公开 API

```
Terminal_Init()      - 初始化服务
Terminal_Start()     - 启动排空和网络服务线程
Terminal_Stop()      - 停止服务
Terminal_Status()    - 打印服务当前状态
Terminal_IsEnabled() - 检查是否已启用
```

TCP 模式下，核心日志 RTT up-buffer 的排空由 `CoreLogRecorder` 保证；Terminal 使用独立线程持续消费记录器队列，并把目标输出写入内部网络队列。
客户端未连接、Telnet 协商中或客户端不可写时，Terminal 线程仍会继续取数并保留网络 backlog。网络 backlog 是 live TCP 投递缓存，不是权威日志；容量不足时会丢弃 backlog 并关闭当前客户端，服务继续接受新连接。启用 `--telnet-port` 时，默认 backlog 为 1 MiB，可用 `--terminal-queue-size <bytes>` 调整，0 表示使用默认值。核心日志文件由 `CoreLogRecorder` 持续记录，是目标输出的权威来源。socket 接收、发送失败或客户端主动断开只关闭当前客户端，不会停止 Terminal 服务。

### 核心日志记录

启用 Console、Telnet、Swimlane 或默认核心日志消费模式时，程序会按当前模式选择明确的 source 集合记录日志：

- Console/Telnet 单通道模式只记录所选 source：Linux 或 RTOS
- Swimlane 模式记录 Linux 与 RTOS 两个 source
- 默认模式等同于 Swimlane，记录 Linux 与 RTOS 两个 source
- SystemView-only 模式不启动 `CoreLogRecorder`

日志文件按启用 source 生成：

- `linux_<timestamp>.log`: Linux source RTT 日志原始字节流
- `rtos_<timestamp>.log`: RTOS source RTT 日志原始字节流

默认写入当前工作目录。使用 `--log-dir <dir>` 可指定所有自动生成日志和记录文件的目录，该目录必须已经存在；程序会在创建任何日志文件前验证该路径存在且是目录。

`main_<timestamp>.log` 是常驻诊断日志，记录 error/warning 级别的启动、RTT 映射、RTTCB 搜索、通道校验、socket/listen/thread、文件持久化等失败原因，并同步输出到 `stderr`。`Log_Print` 仅用于可选 debug trace，只有定义 `_LOG_DEBUG` 时才输出；常驻错误诊断不依赖 `_LOG_DEBUG`。

`CoreLogRecorder` 是已启用核心日志 RTT up-buffer 的唯一读取者和唯一核心日志文件写入者。记录器线程启动后持续读取对应通道，先写入对应文件，再把已记录字节放入内部消费队列。Swimlane、Console 和 Telnet 只消费记录器队列中的数据，不直接读取核心日志 RTT up-buffer，因此不会和文件记录互相抢读。

每个启用 source 独立使用锁保护文件和消费队列，Linux 与 RTOS 记录路径不会因对方写盘或 flush 卡顿而互相阻塞。每个 source 的消费队列默认容量为 1 MiB，可用 `--core-log-queue-size <bytes>` 调整，0 表示使用默认值；非零值不能小于 8192 字节。核心日志文件按时间间隔刷新并在停止时强制刷新；若核心日志文件写入、刷新失败，或消费队列容量不足以无损交付给 Swimlane、Console、Telnet，记录器进入 fatal 状态，主循环退出并返回错误，避免显示/转发路径出现不完整日志流。
运行期 target reset 或 RTTCB 重新初始化导致 source up-buffer 不可用时，`CoreLogRecorder` 会在记录器读取边界等待对应 channel 重新配置；恢复后继续记录同一 source。若配置了 RTTCB 搜索超时且恢复超时，记录器进入 fatal 状态。

SystemView-only 模式只读取配置的 SystemView RTT 通道并生成 `sysview_<timestamp>.SVDat`。核心日志消费模式与 SystemView 同时启用时，`--systemview-channel` 不能与任何已启用核心日志 source 的通道复用。

---

### SystemView 服务

**文件**: `SystemView.h` / `SystemView.c`

SystemView 服务读取 RTT 二进制 trace 数据，支持两种工作模式：

```
+----------+------------------------------------------+-------------------------+
|   模式   |                 参数要求                 |            功能         |
+----------+------------------------------------------+-------------------------+
| 本地记录 | --systemview-channel <n>                 | 从 RTT 读取，记录到文件 |
|          | 单独指定时不启动 Terminal                |                         |
+----------+------------------------------------------+-------------------------+
| 网络服务 | --systemview-port <port>                 | TCP 网络服务 + 本地记录 |
|          | --systemview-channel <n> 可选            |                         |
+----------+------------------------------------------+-------------------------+
```

默认端口：`RTT_BRIDGE_DEFAULT_SYSVIEW_PORT` = **19111**

协议版本：`SYSVIEW_VERSION_MAJOR.MINOR` = **3.60**，握手包大小 `SYSVIEW_HELLO_SIZE` = 32 字节。

网络服务模式下，SystemView trace 由记录线程持续从 RTT 读取并写入本地 `.SVDat` 记录文件。
内部网络队列保存已读取但尚未发送给 TCP 客户端的 trace 数据；客户端尚未完成握手时，早期 trace 会在队列容量范围内等待首个客户端接收。网络队列是 live TCP 投递缓存，不是权威记录；容量不足时会丢弃 backlog 并关闭当前客户端，本地 `.SVDat` 记录继续运行。启用 `--systemview-port` 时，默认 backlog 为 1 MiB，可用 `--systemview-queue-size <bytes>` 调整，0 表示使用默认值。socket 接收、发送失败或客户端主动断开只关闭当前客户端，不会停止 SystemView 记录服务。
SystemView 记录文件保存 RTT 通道上的原始二进制 SVDat 数据。记录线程按时间和字节水位刷新文件，并在线程退出时刷新剩余数据；写入、刷新或关闭失败会使服务进入 fatal 状态，主循环退出并返回错误，避免 trace 静默丢失。
服务启动前会校验 SystemView up-buffer；网络服务模式还会校验同一 channel 的 down-buffer。运行期 RTT region 校验、up-buffer 读取、down-buffer 校验或 down-buffer 写入失败不会被当作无数据处理，服务会进入 recovery wait 并周期重试；RTT 访问恢复后继续记录和服务。记录文件写入、刷新或关闭失败仍会进入 fatal 状态。

#### 配置

```c
typedef struct {
    unsigned     port;              // TCP 端口（默认 19111）
    unsigned     channel;           // RTT 通道号（默认 2）
    bool         enabled;           // 启用/禁用标志
    bool         network_enabled;   // 启用 TCP 网络服务
    bool         record_enabled;    // 启用本地文件记录
    const char  *record_prefix;     // 记录文件名前缀
    size_t       network_queue_size; // TCP backlog 字节数，0 使用默认 1 MiB
} SystemView_Config_t;
```

#### 公开 API

```
SystemView_Init()          - 初始化服务
SystemView_Start()         - 启动记录线程和可选网络服务线程
SystemView_Stop()          - 停止服务
SystemView_Status()        - 打印服务当前状态
SystemView_IsEnabled()     - 检查是否已启用
SystemView_HasFatalError() - 检查是否进入 fatal 状态
```

---

## 底层基础设施

### RTTBridge - 桥接核心

**文件**: `RTTBridge.h` / `RTTBridge.c`

负责 RTT 控制块的发现、验证和整体生命周期管理，是所有上层服务的基础。

#### 关键常量

```
RTT_BRIDGE_DEFAULT_POLL_INTERVAL_MS  = 10    ms  (主 RTT 轮询间隔)
RTT_BRIDGE_DEFAULT_TERMINAL_PORT     = 19021      (Terminal 默认 Telnet 端口)
RTT_BRIDGE_DEFAULT_SYSVIEW_PORT      = 19111      (SystemView 默认端口)
RTT_BRIDGE_DEFAULT_TERMINAL_CHANNEL  = 0          (Terminal 通道)
RTT_BRIDGE_DEFAULT_RTOS_CHANNEL      = 1          (RTOS 日志通道)
RTT_BRIDGE_DEFAULT_SYSVIEW_CHANNEL   = 2          (SystemView 默认通道)
```

#### 配置

```c
typedef struct {
    const char *device_path;        // 设备路径或 POSIX 共享内存名
    uint64_t    rtt_address;        // RTT region 后端地址
    size_t      rtt_region_size;    // 用于映射、搜索和校验的 RTT region 大小
    unsigned    poll_interval_ms;   // 轮询间隔（默认 10ms）
    const char *log_file;           // 可选日志文件路径（已废弃）
    FILE       *log_file_handle;    // 日志文件句柄
    volatile sig_atomic_t *run_flag; // 可选运行标志，置零后终止初始化等待
    unsigned    rtt_search_timeout_ms; // RTTCB 搜索超时，0 表示一直等待直到退出
    bool        reset_memory;       // 后端支持时在映射前重置内存
    bool        debug;              // 调试模式标志
} RTTBridge_Config_t;
```

`rtt_search_timeout_ms=0` 表示保持原有等待语义，适合真实芯片晚启动场景；设置为非零值时，RTTCB 在指定毫秒数内未发现会让 `RTTBridge_Init()` 失败并返回错误，适合地址、size、驱动或共享内存配置需要快速暴露错误的场景。

内部状态还维护 `rtt_cb_address`（发现的 CB 本地映射地址）、`rtt_region_size`（从 CB 起始处计算的剩余映射大小）、`initialized`/`running` 标志，以及 `polls_count` 和 `errors_count` 统计计数。模块状态由进程生命周期锁保护，清理时不会销毁该锁。

#### 公开 API

```
RTTBridge_Init()                  - 初始化桥接器（内存映射、发现并验证 RTT CB）
RTTBridge_Cleanup()               - 清理资源并取消内存映射
RTTBridge_GetState()              - 获取桥接器只读状态
RTTBridge_GetValidatedRTTRegion() - 获取并校验 RTT CB 本地映射地址和 region size；旧地址失效时重新搜索配置映射区
RTTBridge_CheckUpBufferChannel()  - 校验指定上行通道是否存在且已配置
RTTBridge_CheckDownBufferChannel() - 校验指定下行通道是否存在且已配置
RTTBridge_GetBytesInBuffer()      - 校验后查询上行缓冲区待读字节数
RTTBridge_ReadUpBufferNoLock()    - 校验后非加锁读取上行缓冲区
RTTBridge_WriteDownBufferNoLock() - 校验后非加锁写入下行缓冲区
RTTBridge_EnsureRTTInitialized()  - 调用 SEGGER RTT 初始化或复用模拟 RTTCB
RTTBridge_WaitForRTTInitialized() - 等待模拟 RTTCB 初始化
RTTBridge_WaitForRTTUpChannelReady() - 等待 RTT CB 有效且指定上行通道已配置
RTTBridge_IsRunning()             - 查询运行状态
RTTBridge_SetRunning()            - 设置运行标志（用于优雅关闭）
RTTBridge_GetLogFileHandle()      - 获取日志文件句柄
RTTBridge_IncrementPolls()        - 递增轮询计数
RTTBridge_IncrementErrors()   - 递增错误计数
RTTBridge_Status()            - 打印当前状态信息
```

---

### RTTMemory - 内存访问抽象

**文件**: `RTTMemory.h` / `RTTMemory.c`

提供可插拔的 RTT 内存访问后端抽象层，将上层代码与底层内存映射机制解耦。后端地址统一使用 `uint64_t`，本进程可直接解引用的本地映射地址统一使用 `uintptr_t`。

#### 后端接口

```c
typedef struct RTTMem_Backend {
    const char *name;
    int       (*init)(const char *path, uint64_t base, size_t size);
    void      (*cleanup)(void);
    uint64_t  (*get_base)(void);
    size_t    (*get_size)(void);
    uintptr_t (*to_local_address)(uint64_t addr, size_t size);
} RTTMem_Backend_t;
```

#### 内置后端

```
+----------+------------------------------------------------------+
| 后端     | 说明                                                 |
+----------+------------------------------------------------------+
| SMEM     | SharedMem 硬件后端 RTTMem_Backend_SMEM()              |
|          | 通过 /dev/xxx 设备访问物理内存                       |
+----------+------------------------------------------------------+
| MEMSHM   | POSIX 共享内存后端 RTTMem_Backend_MEMSHM()           |
|          | 通过 shm_open() 访问命名共享内存段                   |
+----------+------------------------------------------------------+
```

MEMSHM 后端在正常 cleanup 时不会自动 `shm_unlink()`，因此独立仿真进程重启后仍会解析到同一个共享内存对象。需要清空仿真状态时，可使用 `--memshm-reset` 在初始化时先 unlink 指定 POSIX 共享内存对象，再创建并清零新的映射。不要在其他仿真进程仍使用同名对象时执行 reset；需要跨进程复用旧状态时不要启用该参数。

#### 公开 API

```
RTTMem_InstallBackend()        - 安装自定义后端
RTTMem_GetBackend()            - 获取当前已安装的后端
RTTMem_InstallDefaultBackend() - 安装默认后端
RTTMem_SetResetOnInit()        - 设置后端初始化时是否重置内存
RTTMem_GetResetOnInit()        - 查询后端初始化 reset 配置
RTTMem_Init()                  - 初始化（通过设备路径）
RTTMem_InitEx()                - 初始化（显式指定后端地址和大小）
RTTMem_GetBackendBase()        - 获取后端基地址
RTTMem_GetMappedSize()         - 获取映射区域大小
RTTMem_ToLocalAddress()        - 将后端地址转换为本进程可直接访问的映射地址
```

---

### RTT 协议

**文件**: `../RTT/RTT/SEGGER_RTT.h` / `../RTT/RTT/SEGGER_RTT.c`

TraceHub 使用 RTT 目录中的共享内存 RTT 实现作为唯一 RTT 协议来源。RTT API 接收本进程可直接访问的本地映射地址，descriptor 中的 name/buffer 地址保存为相对 RTT control block 的固定 64 位 offset。owner 侧初始化和额外 up/down buffer pair 配置由 `SEGGER_RTT_EnsureInitEx()` 完成，`RTTBridge_EnsureRTTInitialized()` 只保留 TraceHub 模拟程序侧包装；consumer 侧等待策略仍在 `RTTBridge.c`。

#### 关键常量

```
SEGGER_RTT_MAX_NUM_UP_BUFFERS    = 3        (MEMSHM 模拟默认上行缓冲区数量)
SEGGER_RTT_MAX_NUM_DOWN_BUFFERS  = 3        (MEMSHM 模拟默认下行缓冲区数量)
BUFFER_SIZE_UP                    = 1 MB     (默认上行缓冲区大小)
BUFFER_SIZE_DOWN                  = 1 MB     (默认下行缓冲区大小)
SEGGER_RTT_SPINLOCK_MAX_CORES     = 2        (默认共享软件 spinlock 核心数)
SEGGER_RTT_SPINLOCK_SW_SIZE       = 24 字节  (2 核默认共享软件 spinlock 对象大小)
RTT_COMM_POLL_INTERVAL            = 2 ms     (通信轮询间隔)
RTT_SEND_THRESHOLD                = 512 字节 (发送触发阈值)
```

MEMSHM 默认映射大小包含 RTT control block、所有配置的 up/down buffer pair，以及一个 `SEGGER_RTT_SPINLOCK_SW_SIZE` 大小的共享软件 spinlock 区域。模拟程序把该锁区域放在 `SEGGER_RTT_GetRequiredMemSize(SIM_NUM_CHANNELS)` 之后，Linux 模拟器负责创建，RTOS 模拟器等待该锁就绪后再使用。

#### 缓冲区工作模式

```
SEGGER_RTT_MODE_NO_BLOCK_SKIP       = 0  (无阻塞，FIFO 满时跳过数据)
SEGGER_RTT_MODE_NO_BLOCK_TRIM       = 1  (无阻塞，FIFO 满时截断数据)
SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL  = 2  (FIFO 满时阻塞等待)
```

#### 主要 API

```
SEGGER_RTT_InitEx()                - 按映射大小初始化 RTT control block
SEGGER_RTT_EnsureInitEx()          - 初始化或复用 RTTCB 并配置额外 up/down buffer pair
SEGGER_RTT_CheckInit()             - 检查 RTT ID 是否完整
SEGGER_RTT_CheckRegion()           - 校验 control block 和 descriptor 是否位于映射区域内
SEGGER_RTT_CheckUpBuffer()         - 校验指定 up-buffer descriptor 是否有效且已配置
SEGGER_RTT_CheckDownBuffer()       - 校验指定 down-buffer descriptor 是否有效且已配置
SEGGER_RTT_REQUIRED_MEM_SIZE_FOR_BUFFER_PAIRS() - 编译期计算有效 buffer pair 数需要的 RTT 共享内存大小，0 返回 0
SEGGER_RTT_GetRequiredMemSize()    - 计算指定 buffer pair 数需要的 RTT 共享内存大小
SEGGER_RTT_FindControlBlock()      - 在映射区域中搜索 RTT control block
SEGGER_RTT_FindValidControlBlock() - 在映射区域中搜索有效 RTT control block
SEGGER_RTT_SPINLOCK_SW_Create()    - 在共享内存中创建软件 spinlock
SEGGER_RTT_SPINLOCK_SW_Check()     - 校验共享软件 spinlock 是否有效
SEGGER_RTT_SPINLOCK_SW_Lock()      - 使用 core id 获取共享软件 spinlock
SEGGER_RTT_SPINLOCK_SW_Unlock()    - 使用 core id 释放共享软件 spinlock
RTTBridge_ReadUpBufferNoLock()     - 校验后非加锁读取上行缓冲区
RTTBridge_WriteDownBufferNoLock()  - 校验后非加锁写入下行缓冲区
RTTBridge_GetBytesInBuffer()       - 校验后获取缓冲区中的字节数
RTTBridge_EnsureRTTInitialized()   - TraceHub 模拟程序 owner 侧包装
RTTBridge_WaitForRTTInitialized()  - 模拟程序 consumer 侧等待 RTTCB 初始化
RTTBridge_WaitForRTTUpChannelReady() - 模拟程序 consumer 侧等待指定上行通道配置完成
```

---

## 使用方法

### 命令行参数

#### 必须参数

```
--addr <hex_address>    RTT region 后端地址（十六进制）；0 表示使用后端基地址
--size <hex_size>       用于映射、搜索和校验的 RTT region 大小（十六进制）；0 表示使用后端映射大小
--shm <name>            POSIX 共享内存名称
  或
--device <path>         设备文件路径（与 --shm 二选一）
```

#### Terminal 服务参数（可选）

```
--console               启用 Console 模式（使用 stdin/stdout）
--swimlane              启用 Swimlane 模式（双列终端显示）
--telnet-port <port>    启用 Telnet 模式并指定 TCP 端口
--linux-channel <n>     Linux 日志 RTT 通道号（默认 0）
--rtos-channel <n>      RTOS 日志 RTT 通道号（默认 1）
--terminal-queue-size <bytes>  Telnet TCP backlog 字节数，仅用于 --telnet-port，0 使用默认 1 MiB
--core-log-queue-size <bytes>  每个核心日志 source 的消费队列字节数，0 使用默认 1 MiB，非零值最小 8192
--swimlane-flush-threshold <count>  Swimlane ready 刷新阈值（默认 4，最大 8192）
--swimlane-flush-timeout-ms <ms>    Swimlane ready 刷新超时毫秒数（默认 1000，0 禁用超时刷新）
```

**约束**: `--console` 和 `--swimlane` 互斥，不能同时使用。Console 和 Telnet 是单通道模式，不能同时指定 `--linux-channel` 与 `--rtos-channel`；Swimlane 才启用双通道显示和双 source 记录。`--terminal-queue-size` 只用于 `--telnet-port` TCP 模式；`--swimlane-flush-threshold` 与 `--swimlane-flush-timeout-ms` 只用于 Swimlane 模式；`--core-log-queue-size` 要求至少启用一个核心日志 source。

#### SystemView 服务参数（可选）

```
--systemview-channel <n>      SystemView RTT 通道号（默认 2）
--systemview-port <port>      SystemView TCP 服务端口（默认 19111）
--systemview-queue-size <bytes>  SystemView TCP backlog 字节数，仅用于 --systemview-port，0 使用默认 1 MiB
```

未指定 `--systemview-channel` 时，SystemView 使用默认通道 2。

**约束**: 与 Terminal、Console、Telnet 或 Swimlane 同时启用时，`--systemview-channel` 不能与任何已启用核心日志 source 的通道相同。`--systemview-queue-size` 只用于 `--systemview-port` TCP 模式。SystemView-only 模式只占用 SystemView 通道。

#### 其他参数

```
-v, --version             显示版本信息
-h, --help                显示帮助信息
--rtt-timeout-ms <ms>     RTTCB 搜索超时，0 表示一直等待直到退出（默认 0）
--memshm-reset            MEMSHM 后端初始化时重置指定 POSIX 共享内存对象
--log-dir <dir>           自动生成日志和记录文件的目录（默认当前工作目录）
```

---

### 默认行为与模式选择

**默认行为**: 只指定必须参数时，默认启用 **Swimlane 模式**（linux_channel=0, rtos_channel=1）。

**模式选择决策流程**:

```
用户输入命令
      |
      v
是否只指定了 SystemView 参数（无任何 Terminal 参数）?
      |
      +--- 是 ---> 只运行 SystemView（不启动 Terminal）
      |
      +--- 否 ---> 启用 Terminal，执行模式判断：
                        |
                        +--- 指定了 --telnet-port ? --> Telnet 模式
                        +--- 指定了 --console ?     --> Console 模式
                        +--- 指定了 --swimlane ?    --> Swimlane 模式
                        +--- 以上都未指定           --> 默认 Swimlane 模式

通道选择：
      +--- Console/Telnet 指定 --linux-channel --> 只启用 Linux source
      +--- Console/Telnet 指定 --rtos-channel  --> 只启用 RTOS source
      +--- Console/Telnet 未指定 source        --> 只启用 Linux source
      +--- Swimlane/default                    --> 启用 Linux 与 RTOS source
```

---

### 使用示例

#### 基础用法

```bash
# 1. 默认 Swimlane 模式（最简单，使用默认通道 linux=0, rtos=1）
tracehub --addr 0x10000000 --size 0 --shm /rtt_sim

# 2. Console 模式（使用默认通道 linux=0）
tracehub --addr 0x10000000 --size 0 --shm /rtt_sim --console

# 3. Console 模式（指定 RTOS 通道）
tracehub --addr 0x10000000 --size 0 --shm /rtt_sim \
  --console --rtos-channel 1

# 4. Swimlane 模式（显式指定，使用默认通道）
tracehub --addr 0x10000000 --size 0 --shm /rtt_sim --swimlane

# 5. Swimlane 模式（自定义通道）
tracehub --addr 0x10000000 --size 0 --shm /rtt_sim \
  --swimlane --linux-channel 0 --rtos-channel 1
```

#### MEMSHM 仿真复现

```bash
# 重置同名共享内存对象，并在 30 秒内未发现 RTTCB 时失败
tracehub --addr 0x10000000 --size 0 --shm /rtt_sim \
  --memshm-reset --rtt-timeout-ms 30000 --swimlane
```

#### Telnet 网络服务

```bash
# 6. Telnet 模式（使用默认通道 linux=0）
tracehub --addr 0x10000000 --size 0 --shm /rtt_sim \
  --telnet-port 2323

# 7. Telnet 模式（指定 RTOS 通道）
tracehub --addr 0x10000000 --size 0 --shm /rtt_sim \
  --telnet-port 2323 --rtos-channel 1
```

#### SystemView 服务

```bash
# 8. 只运行 SystemView 本地记录（不启动 Terminal）
tracehub --addr 0x10000000 --size 0 --shm /rtt_sim \
  --systemview-channel 2

# 9. 运行 SystemView 网络服务并本地记录 SVDat（不启动 Terminal）
tracehub --addr 0x10000000 --size 0 --shm /rtt_sim \
  --systemview-port 19111
```

#### 组合用法

```bash
# 10. Swimlane + SystemView 本地记录
tracehub --addr 0x10000000 --size 0 --shm /rtt_sim \
  --swimlane --systemview-channel 2

# 11. Swimlane + SystemView 网络服务和本地 SVDat 记录
tracehub --addr 0x10000000 --size 0 --shm /rtt_sim \
  --swimlane --systemview-port 19111

# 12. Telnet + SystemView 网络服务和本地 SVDat 记录
tracehub --addr 0x10000000 --size 0 --shm /rtt_sim \
  --telnet-port 2323 --systemview-port 19111
```

---

### 服务初始化顺序

```
1. LOG_Init()              主日志文件初始化
2. _SignalInit()           信号处理器注册（SIGINT, SIGTERM, SIGILL 等）
3. RTTBridge_Init()        RTT 内存映射和控制块发现
4. CoreLogRecorder_Init()  已启用 source 的核心日志文件初始化（核心日志模式）
5. Terminal_Init()         Terminal 服务初始化
6. SystemView_Init()       SystemView 服务初始化
7. RTTBridge_SetRunning()  设置运行状态为 true
8. CoreLogRecorder_Start() 启动已启用 source 的读取和落盘线程（核心日志模式）
9a. _RunSwimLaneMode()     泳道主循环（Swimlane 模式）
  或
9b. _RunNormalMode()       普通服务主循环（Terminal/SystemView 模式）
```

**信号处理**: `SIGINT` / `SIGTERM` 触发优雅关闭（设置 `_running = 0`）；`SIGILL` / `SIGFPE` / `SIGSEGV` / `SIGABRT` 只执行 async-signal-safe 的错误输出并立即退出。

---

## 时间同步要求

**关键要求**: Linux A53 和 RTOS R5 两个核心必须使用**同一个**硬件计数器作为时间基准。若使用不同时间基准，LogMerger 将无法按正确时序合并日志，泳道显示将产生错误的时序排列。

macOS/Linux 主机仿真程序使用 `SYS_GetMonotonicTimeUs()` 生成日志时间戳，该函数基于 `CLOCK_MONOTONIC` 返回微秒级单调时间，不受午夜跨越或系统时间调整影响。仿真程序的周期业务日志使用 `[timestamp_us] log content`，启动阶段同时覆盖无 timestamp banner 和 ANSI 颜色日志，匹配真实芯片常见输出。

**推荐实现**（ARM 平台）：

```
两个核心均从 CNTPCT_EL0（ARM Generic Timer 物理计数器）读取

转换公式:
  timestamp_us = (CNTPCT_EL0 * 1000000) / CNTFRQ_EL0

计算示例:
  计数器频率: CNTFRQ_EL0 = 62,500,000 (62.5 MHz)
  计数器值:   CNTPCT_EL0 = 625,000
  时间戳:     (625,000 * 1,000,000) / 62,500,000 = 10,000 us (10 ms)
```

---

## 线程安全说明

```
+--------------------+----------+------------------------------------------+
| 模块               | 线程安全 | 说明                                     |
+--------------------+----------+------------------------------------------+
| LogEntry           | N/A      | 不可变数据，采用所有权转移模式            |
+--------------------+----------+------------------------------------------+
| LogCollector       | 是       | 内部维护独立线程和各自的缓冲区            |
+--------------------+----------+------------------------------------------+
| LogMerger          | 是       | 内部使用 SYS_Mutex 保护缓冲区            |
+--------------------+----------+------------------------------------------+
| SwimLaneRenderer   | 是       | 内部使用 SYS_Mutex 保护渲染输出          |
+--------------------+----------+------------------------------------------+
| RTTMemory          | 否       | 底层内存访问，由上层调用方保证串行       |
+--------------------+----------+------------------------------------------+
| RTTBridge          | 是       | 内部维护 SYS_Mutex 保护状态             |
+--------------------+----------+------------------------------------------+
| CoreLogRecorder    | 是       | 每个核心独立 SYS_Mutex 保护文件和队列    |
+--------------------+----------+------------------------------------------+
| Terminal           | 是       | 内部管理排空线程和网络服务线程           |
+--------------------+----------+------------------------------------------+
| SystemView         | 是       | 内部管理接受线程和轮询线程               |
+--------------------+----------+------------------------------------------+
```

SystemView target 侧多核同时写同一 RTT up-buffer 时必须使用跨核共享锁保护 `SEGGER_SYSVIEW_LOCK()` / `SEGGER_SYSVIEW_UNLOCK()`。模拟程序使用 RTT 公共 `SEGGER_RTT_SPINLOCK_SW_*` API 在 MEMSHM 共享内存中保护 channel 2；真实芯片环境应在共享内存或硬件互斥资源中放置同一类锁对象，并为每个 core 分配稳定唯一的 core id。

---

## 事实约束与验收边界

### 真实芯片集成约束

TraceHub 是主机侧 RTT 读取和展示工具，不是目标端共享内存、cache 属性、RTTCB 初始化或 SystemView 锁实现的证明者。SMEM 后端只负责打开 `SharedMem` 设备、查询后端地址和大小、执行 `mmap`，并通过 RTTBridge 搜索和校验 RTTCB。真实芯片链路成立必须同时满足以下外部集成契约：

- Linux SharedMem 驱动必须暴露正确的物理共享内存窗口，`phys_addrs`、`mem_sizes`、设备节点和 UAPI 必须与目标端实际布局一致。
- 目标端必须使用本仓库同一套共享 RTT fork。该 fork 的 RTT descriptor 使用共享内存相对 offset，不兼容标准 SEGGER RTT/J-Link reader。
- 共享 RTT 内存必须对所有参与核心保持 uncache 或具备等价一致性保证。TraceHub 运行在用户空间，无法证明目标端 MMU、MPU、cache maintenance 或总线属性配置正确。
- RTTCB 初始化方必须唯一。MEMSHM 仿真中 Linux simulator 负责初始化；真实芯片环境必须由目标端明确指定唯一 owner，其他核心只能等待或复用已初始化 RTTCB。
- Linux、RTOS 和 SystemView 的 RTT channel 分配必须稳定，且不能把 SystemView channel 与已启用的文本日志 source channel 复用。
- SystemView 多核写同一 RTT up-buffer 时必须使用跨核共享锁保护，且每个 core id 必须稳定唯一。
- Linux A53 和 RTOS R5 必须使用同一个硬件时间基准生成文本日志 timestamp，不能使用各自独立的系统时间或启动后相对时间。

### 验收边界

MEMSHM smoke 验证的是主机仿真链路。它能覆盖 tracehub 进程启动、POSIX shared memory 后端、模拟 RTTCB、双 source 文本日志、泳道排序、SystemView 本地 SVDat 和 TCP trace 投递，但不能证明真实芯片 SharedMem 驱动、目标端 cache 属性或真实 SystemView GUI 兼容性。

SMEM preflight 验证的是真实芯片 SMEM 启动和有限数据链路。默认验收要求 `main_*.log`、`linux_*.log`、`rtos_*.log` 和 `sysview_*.SVDat` 均在本次运行中生成且非空。显式使用 `--startup-only` 时，只验证 SharedMem 设备访问、mmap、RTTCB 发现、channel layout、进程存活和主日志创建，不代表 Linux/RTOS/SystemView 业务数据链路通过。

SEGGER SystemView GUI 兼容性必须用真实 GUI 连接 `--systemview-port` 验收。仓库 smoke client 只验证 tracehub TCP 服务使用同一 32 字节 Hello 语义完成握手并投递 trace 字节。

### 泳道日志约束

泳道模式是严格时序视图。每条可排序业务日志应输出 `[timestamp_us] log content`，其中 `timestamp_us` 是同一硬件时间基准下的十进制微秒值。timestamp prefix 必须使用 `[]` 包裹；裸数字、PID、错误码或裸 `HH:MM:SS` 开头的日志会作为普通内容处理。无 timestamp 的日志行只适合 boot banner、第三方库输出和 continuation line；它们会继承同一 source 上一次成功解析到的 timestamp。首个 timestamp 到达前的无 timestamp 启动日志会暂存；若同一 source 始终没有 timestamp，刷新阶段使用 fallback timestamp 输出；超过暂存容量会进入 fatal 状态，因为此时无法继续无损保存待排序内容。

LogMerger 只保证未输出条目的排序。条目一旦输出，后续到达且时间戳早于已输出位置的 entry 会被拒绝，避免终端泳道和 `swimlane_*.log` 出现不可恢复的倒序历史。真实业务必须保证两个核心的 timestamp 单调推进，并保证长期运行中不会生成比已经输出位置更早的日志。

### 平台与工具链约束

macOS 原生运行只支持 MEMSHM 仿真后端，不能运行 SMEM SharedMem 硬件后端，也不能使用 `run-console.sh`、`run-terminal.sh`、`run-sysview.sh` 或 `run_smem_preflight.sh` 进行真实芯片路径验证。Linux 支持 MEMSHM 仿真后端和 SMEM SharedMem 硬件后端。

主工程 `CMakeLists.txt` 要求 CMake 3.28 或更高版本；`build.sh` 使用 Ninja generator。真实芯片 SDK、旧 Linux 发行版或 CI 镜像必须提供对应版本的 CMake 和 Ninja，或显式使用满足同等构建语义的 CMake 入口。

### 模块职责边界

当前抽象层服务于实际运行路径：RTTMemory 区分 MEMSHM 和 SMEM 后端，CoreLogRecorder 保证核心文本日志只有一个 RTT reader，LogCollector/LogMerger/SwimLaneRenderer 保证泳道采集、排序和渲染分层，SystemView 独立读取二进制 trace 通道。新增功能必须遵守这些所有权边界：核心文本日志 RTT up-buffer 只能由 CoreLogRecorder 读取；SystemView trace 只能由 SystemView 服务读取；网络 backlog 不是权威日志，文件记录才是持久化来源。

---

## 构建说明

```bash
cd TraceHub
./build.sh
```

构建系统使用本目录的独立 CMake 配置，生成可执行文件 `tracehub`。构建过程不加载仓库外的 CMake 模块或环境脚本。

常用编译和测试命令：

```bash
# 主机 MEMSHM Debug 构建，包含模拟程序和单元测试目标
./build.sh --backend memshm

# 运行单元测试
ctest --test-dir build/HOST_Debug/test --output-on-failure

# 运行 MEMSHM 主机仿真 smoke 验证
bash test/run_memshm_smoke.sh build/HOST_Debug

# Linux SharedMem 硬件后端构建
./build.sh --backend smem

# Linux SharedMem 真实芯片 preflight 验证
sudo bash test/run_smem_preflight.sh --build-dir build/HOST_Debug -k /path/to/SharedMem.ko
```

直接使用 CMake 时使用以下命令：

```bash
# 配置并编译 MEMSHM 后端
cmake -S . -B build/HOST_Debug -GNinja -DCMAKE_BUILD_TYPE=Debug -DUSE_MEMSHM_BACKEND=ON -DBUILD_SIM_TESTS=ON
cmake --build build/HOST_Debug
ctest --test-dir build/HOST_Debug/test --output-on-failure

# 配置并编译 SMEM 后端
cmake -S . -B build/HOST_Debug -GNinja -DCMAKE_BUILD_TYPE=Debug -DUSE_MEMSHM_BACKEND=OFF -DBUILD_SIM_TESTS=OFF
cmake --build build/HOST_Debug
```

后端选择：

```bash
# 原生主机构建默认使用 MEMSHM 后端，用于 macOS/Linux 仿真
./build.sh

# 显式选择 MEMSHM 后端
./build.sh --backend memshm

# 显式选择原生主机构建目标
./build.sh -a HOST --backend memshm

# 直接使用 CMake 时默认也是 MEMSHM
cmake -S . -B build -DUSE_MEMSHM_BACKEND=ON

# Linux SharedMem 硬件后端
./build.sh --backend smem

# 直接使用 CMake 选择 SMEM
cmake -S . -B build-smem -DUSE_MEMSHM_BACKEND=OFF

# 非 ARM64 主机交叉构建真实芯片 ARM64 版本
./build.sh -a ARM64 --backend smem --toolchain /path/to/toolchain.cmake
```

macOS 原生运行只支持 MEMSHM 仿真后端。Linux 同时支持 MEMSHM 仿真后端和 SMEM SharedMem 硬件后端；SMEM 后端使用仓库 `SharedMem/UAPI/SharedMem.h` 作为唯一 UAPI 来源，默认设备节点为 `/dev/shared_mem0`，运行脚本加载 `SharedMem.ko` 时使用 `phys_addrs` 和 `mem_sizes` 参数。三个 `run-*.sh` 脚本只适用于 Linux SMEM 硬件路径，并支持 `--rtt-timeout-ms` 控制 RTTCB 发现等待时间，支持 `--log-dir` 指定自动生成日志和记录文件目录。脚本发现 `SharedMem` 已加载时默认拒绝覆盖，必须显式传入 `--force-reload` 才会卸载并重新加载模块；脚本退出时只卸载本次运行加载的模块。`run-console.sh` 和 `run-terminal.sh` 支持 `--source linux|rtos` 与 `--channel <n>` 选择单通道 source。`BUILD_SIM_TESTS` 默认只在 MEMSHM 后端开启；模拟程序固定使用 MEMSHM，因此 SMEM 后端不会构建模拟程序，显式启用该组合会在 CMake 配置阶段报错。Linux 模拟程序负责初始化 RTTCB、写入 channel 0 文本日志、使用 SEGGER_SYSVIEW 在 channel 2 写入 A53 事件，并读取 Linux/SystemView down-buffer；RTOS 模拟程序写入 channel 1 文本日志、等待 channel 2 和 SystemView spinlock 就绪后使用 SEGGER_SYSVIEW 写入 R5 事件，并读取 RTOS down-buffer。两个模拟程序共同形成多核 SystemView 二进制 SVDat 事件流，覆盖启动同步、系统描述、任务创建/切换、中断、timer、marker 和 target printf 场景。
如果已有旧版 `/rtt_sim` POSIX 共享内存对象小于当前默认布局，MEMSHM 会直接报错而不是继续映射；重新创建共享内存对象或使用 `--memshm-reset` 后再运行模拟程序。

---

## 自动化 smoke 验证

MEMSHM smoke 脚本用于验证主机仿真链路，不替代单元测试。它按 `tracehub -> rtt_sim_linux -> rtt_sim_rtos` 顺序启动进程，并使用 `rtt_sim_sysview_tcp_client` 连接 SystemView TCP 服务，断言以下结果：

- `linux_*.log` 包含 Linux A53 日志
- `rtos_*.log` 包含 RTOS R5 日志
- `swimlane_*.log` 同时包含 LINUX 和 RTOS 行，且时间戳单调不下降
- `sysview_*.SVDat` 已生成并非空
- SystemView TCP 服务完成 32 字节 Hello 握手并向客户端投递 trace 字节

使用方式：

```bash
cd TraceHub
./build.sh --backend memshm
bash test/run_memshm_smoke.sh
```

也可以显式指定构建目录：

```bash
bash test/run_memshm_smoke.sh build/HOST_Debug
```

手动 MEMSHM 仿真联调可按 `tracehub -> rtt_sim_linux -> rtt_sim_rtos` 顺序在不同终端启动。冷启动整套仿真时，第一个终端启动 TraceHub 并重置同名共享内存对象：

```bash
cd TraceHub
./build/HOST_Debug/tracehub \
  --addr 0x10000000 \
  --size 0 \
  --shm /rtt_sim \
  --memshm-reset \
  --rtt-timeout-ms 10000 \
  --swimlane
```

`--memshm-reset` 会删除并重建 `/rtt_sim` POSIX 共享内存对象。已经运行的 simulator 仍映射旧对象，不会自动切换到新对象；因此带 `--memshm-reset` 重启 TraceHub 后，必须重新启动 Linux 和 RTOS simulator。

只重启 TraceHub、并保留已经运行的 Linux/RTOS simulator 时，不要使用 `--memshm-reset`：

```bash
cd TraceHub
./build/HOST_Debug/tracehub \
  --addr 0x10000000 \
  --size 0 \
  --shm /rtt_sim \
  --rtt-timeout-ms 10000 \
  --swimlane
```

第二个终端启动 Linux A53 simulator：

```bash
cd TraceHub
./build/HOST_Debug/test/rtt_sim_linux
```

第三个终端启动 RTOS R5 simulator：

```bash
cd TraceHub
./build/HOST_Debug/test/rtt_sim_rtos
```

需要同时验证 SystemView TCP 服务时，第一个终端使用带端口的命令：

```bash
cd TraceHub
./build/HOST_Debug/tracehub \
  --addr 0x10000000 \
  --size 0 \
  --shm /rtt_sim \
  --memshm-reset \
  --rtt-timeout-ms 10000 \
  --swimlane \
  --systemview-port 19112
```

Linux 和 RTOS simulator 均启动后，再运行 SystemView TCP smoke client：

```bash
cd TraceHub
./build/HOST_Debug/test/rtt_sim_sysview_tcp_client 127.0.0.1 19112 1 10000
```

SMEM preflight 脚本用于验证 Linux SharedMem 真实芯片启动路径。它会加载或复用 `SharedMem` 模块，启动 SMEM 后端的 `tracehub`，并使用有限 `--rtt-timeout-ms` 验证 SharedMem 设备访问、mmap、RTTCB 发现、Linux/RTOS/SystemView channel layout 以及 `main_<timestamp>.log` 创建。该脚本要求目标端已经完成共享 RTT layout 集成，包括 uncache 共享内存、双核一致时间戳、SystemView 跨核锁和稳定 channel 分配。

使用方式：

```bash
cd TraceHub
./build.sh --backend smem
sudo bash test/run_smem_preflight.sh --build-dir build/HOST_Debug -k /path/to/SharedMem.ko
```

如果 `SharedMem` 已由外部流程加载且参数确认正确，可复用现有模块：

```bash
bash test/run_smem_preflight.sh --use-loaded-module --device /dev/shared_mem0 --log-dir ./logs
```

预检默认要求 `linux_*.log`、`rtos_*.log` 和 `sysview_*.SVDat` 均非空，避免只验证启动路径却误判为真机数据链路已通过。只验证启动路径时必须显式增加 `--startup-only`。需要提升业务数据链路覆盖时，可增加 `--duration-sec <sec>` 让 `tracehub` 在启动验证后继续运行指定时间；也可用 `--linux-marker <regex>` 或 `--rtos-marker <regex>` 要求日志包含目标端已知 marker。

预检脚本不替代长时间真实业务压力测试；它覆盖的是真实芯片路径能否启动、能否发现 RTTCB、所需 RTT channels 是否存在、配置时长内进程是否保持运行，并默认验证文本日志和 SystemView 记录文件是否有真实数据。使用 `--startup-only` 时只覆盖启动路径。SEGGER SystemView GUI 兼容性仍需要在真实 GUI 环境中连接 `--systemview-port` 进行外部验收；仓库内自动 smoke 覆盖的是 tracehub TCP 服务自身的握手和 trace 投递路径。
