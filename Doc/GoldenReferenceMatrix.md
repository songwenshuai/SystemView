# Trice C 单元测试黄金参考对照

## 1. 目标

本文件定义 TraceHub 侧 C 版 Trice runtime decoder 和 ID table loader 的黄金参考来源、覆盖矩阵和验收标准。目标是把官方 Go/CGO 测试中已经验证过的行为转化为 C 单元测试依据，避免 C 侧重新发明语义规则。

本文件只描述测试事实和提取规则，不接入构建系统，不要求当前目录存在实际 raw bytes fixture。后续生成 fixture 时，必须以本文件为单一入口维护来源、覆盖和状态。

## 2. 黄金参考来源优先级

| 优先级 | 官方来源 | 用途 | 结论 |
| --- | --- | --- | --- |
| 1 | `rokath/trice/_test/testdata/triceCheck.c` | 每个被测试 C 日志调用的 expected 文本，来自 `//exp: "..."` 注释 | 端到端 expected 的权威来源 |
| 1 | `rokath/trice/_test/testdata/cgoPackage.go` | 执行 C 测试行，取得 raw bytes，再调用 Go `trice log -p BUFFER -args ...` 对比 expected | raw bytes 提取流程的权威来源 |
| 1 | `rokath/trice/_test/*/cgo_test.go` | 每种目标端配置对应的 Go decoder 参数 | 端到端配置的权威来源 |
| 2 | `rokath/trice/internal/trexDecoder/trexDecoder_test.go` | decoder 级成功路径和错误路径 | C decoder 单元测试的权威参考 |
| 2 | `rokath/trice/demoTIL.json` | 官方 demo ID table | JSON table loader 和运行时查表参考 |
| 2 | `rokath/trice/demoLI.json` | 官方 demo location table | 可选源码位置诊断参考 |
| 2 | `rokath/trice/til.c` | 官方 generated C table | `til.json -> generated C table -> runtime decode` 闭环参考 |
| 3 | `rokath/trice/examples/exampleData` | demo 源码 | 只作为补充覆盖，不作为 expected 权威来源 |
| 排除 | `rokath/trice/trice.bin` | 根目录二进制文件 | 官方文件为空，不能作为黄金输入 |

## 3. 端到端 Go-vs-C 黄金样例矩阵

端到端样例必须从官方 CGO 测试中提取并固化为：

```text
case name
  official directory
  triceCheck.c line
  raw bytes
  expected text
  til source
  li source, optional
  framing
  endian
  Go decoder args
```

| Case | 官方目录 | 关键配置 | Go 参数事实 | 覆盖能力 | 状态 |
| --- | --- | --- | --- | --- | --- |
| `golden_none_direct_rtt32` | `_test/staticB_di_nopf_rtt32` | direct mode, NONE framing, RTT32, doubled 16-bit ID | `-pf=NONE -d16 -hs=off -prefix=off -li=off -color=off` | NONE framing、direct output、timestamp S0/S2/S4、普通 TRICE 参数 | 需要提取 raw bytes |
| `golden_cobs_deferred_multi_uart` | `_test/dblB_de_multi_cobs_ua` | deferred multi-pack, COBS framing, UARTA | `-pf=COBS -hs=off -prefix=off -li=off -color=off` | COBS framing、deferred output、多包流、普通参数和特殊类型 | 需要提取 raw bytes |
| `golden_tcobsv1_direct_rtt32` | `_test/staticB_di_tcobs_rtt32` | direct mode, TCOBSv1 framing, RTT32 | `-d16 -hs=off -prefix=off -li=off -color=off` | TCOBSv1 framing、direct output、doubled ID | 需要提取 raw bytes |
| `golden_tcobsv1_big_endian` | `_test/be_dblB_de_tcobs_ua` | big-endian transfer order, single framing | `-triceEndianness=bigEndian -singleFraming -hs=off -prefix=off -li=off -color=off` | big-endian payload、single framing、TCOBSv1 | 需要提取 raw bytes |

端到端样例的 expected 只能来自 `triceCheck.c` 中对应行的 `//exp:` 注释。不得从 C decoder 当前输出反向生成 expected。

## 4. Decoder 级 starter vectors

这些用例来自官方 `internal/trexDecoder/trexDecoder_test.go`，不依赖 CGO 运行，适合先转成 C 单元测试的固定 byte vectors。

| Case | 官方测试 | 输入阶段 | Framing | Endian | 输入 bytes | ID table | 期望 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `none_known_trice8_1` | `TestReadNoneFramingKnownTrice` | stream | none | little | `01 40 c0 01 2a 00 00 00` | `1 -> { Type: "TRICE8_1", Strg: "v=%d" }` | exact `v=42` |
| `cobs_unknown_id` | `TestReadCOBSFramingUnknownID` | stream | COBS | little | `06 01 40 c0 01 2a 00` | empty | contains `unknown ID` |
| `decoded_package_short_one_byte` | `TestReadFramedUnsupportedShortPackets` | decoded package | COBS | little | `aa` | unused | contains `unsupported short packet size 1` |
| `decoded_package_short_regular_selector` | `TestReadFramedUnsupportedShortPackets` | decoded package | COBS | little | `01 40` | unused | contains `unsupported short non-X0 packet size 2` |
| `none_cycle_error` | `TestReadCycleErrorMessage` | stream | none | little | `01 40 10 01 2a` | `1 -> { Type: "TRICE8_1", Strg: "v=%d" }` | contains `CYCLE_ERROR`; previous cycle forced to `0x11` |
| `tcobsv2_unsupported_config` | `TestNewSelectsPackageFraming` | config | TCOBSv2 | any | none | unused | config error, contains `unsupported framing` |

这些 starter vectors 覆盖最小正向路径、未知 ID、坏包长度、错误 cycle 和 unsupported framing。它们不替代端到端 Go-vs-C 样例，作用是先把 C decoder 的基础行为固定住。

## 5. 错误路径覆盖要求

| 场景 | 来源 | C 测试要求 |
| --- | --- | --- |
| unknown ID | `TestReadCOBSFramingUnknownID` | 输出诊断必须包含 ID 信息和 `unknown ID` |
| short packet: 1 byte | `TestReadFramedUnsupportedShortPackets` | 不得崩溃，诊断必须包含 packet size |
| short packet: regular selector only | `TestReadFramedUnsupportedShortPackets` | 不得把残缺 selector 当作有效 record |
| cycle mismatch | `TestReadCycleErrorMessage` | cycle check 开启时必须输出 `CYCLE_ERROR` |
| unsupported TCOBSv2 | `TestNewSelectsPackageFraming` | 阶段二必须在配置阶段拒绝 |
| partial frame | `TestNextPackageConsumesInputUntilDelimiter` 相关路径 | feed 半帧时不能输出日志，必须保留等待后续 bytes |
| malformed COBS/TCOBSv1 | `TestNextPackageCOBS`、`TestNextPackageTCOBSPaths` 相关路径 | 解帧错误必须显式诊断，不能静默丢弃后继续输出伪日志 |

## 6. Generated table 闭环

官方 `til.c` 是由 `demoTIL.json` 生成的 C 表，包含 runtime decode 需要的派生字段。C 侧必须验证 JSON table loader 和 generated table loader 得到一致事实。

| 字段 | 来源 | C 侧用途 |
| --- | --- | --- |
| `id` | `demoTIL.json` key / `til.c` entry | runtime 按 ID 查找 |
| `type` | TIL `Type` / generated table comment or metadata | 诊断和一致性检查 |
| `format_string` | TIL `Strg` / generated table string | 文本格式化和 tag 提取 |
| `bitWidth` | generated table 派生字段 | payload 读数宽度 |
| `paramCount` | generated table 派生字段 | payload 长度和参数数量校验 |

闭环测试要求：

1. 端到端黄金样例使用的每个 ID，在 JSON loader 和 generated table loader 中解析出的 format string 必须一致。
2. `bitWidth` 和 `paramCount` 必须来自统一派生逻辑，runtime 不允许多个调用点重复推导。
3. JSON table 和 generated table 任一侧缺失 ID，都必须让测试失败。

## 7. 不作为黄金样例的内容

| 内容 | 原因 |
| --- | --- |
| `trice.bin` | 官方根目录文件为空，不能证明 decoder 行为 |
| `examples/exampleData/triceExamples.c` | 没有 `//exp:` expected 注释，不能作为权威 expected |
| `examples/exampleData/triceLogDiagData.c` | 适合补充 demo，不适合作为主黄金样例 |
| TCOBSv2 success decode | 官方 Go 测试只证明配置识别，不证明完整 C 解码路径；阶段二必须作为 unsupported |
| XTEA success decode | 属于扩展阶段；阶段二只验证配置拒绝 |
| X0 success decode | 属于扩展阶段；阶段二只验证配置拒绝或明确 unsupported |

## 8. Fixture 命名建议

后续生成实际文件时，推荐使用单一目录：

```text
Trice/c_unit_tests/golden/
  README.md
  sources.json
  runtime_cases.json
  decoder_vectors.json
  generated_table_closure.json
  bytes/
  expected/
```

命名规则：

1. `runtime_cases.json` 记录端到端 Go-vs-C 样例。
2. `decoder_vectors.json` 记录不依赖 CGO 的 decoder 级 starter vectors。
3. `bytes/<case>.txt` 使用空格分隔十六进制 bytes。
4. `expected/<case>.txt` 保存 expected 文本或 required diagnostic substring。
5. 文件中必须记录 upstream path 和 upstream test name。

## 9. 准入标准

新增 C 单元测试前，必须满足以下条件：

1. 每个成功解码样例都有官方 expected 来源。
2. 每个错误路径样例都有官方 Go 单元测试来源，或在本文件中明确说明为 TraceHub 自有严格性补充。
3. 每个 runtime 样例都显式记录 framing、endianness、TIL、LI、Go args 和 source directory。
4. 未支持能力必须以 config error 测试表达，不能通过 fallback 或默认值继续运行。
5. C decoder 输出与 Go decoder 输出不一致时，先确认官方 expected，再判断是 C bug、Go 行为差异还是 TraceHub 明确收紧的行为。
