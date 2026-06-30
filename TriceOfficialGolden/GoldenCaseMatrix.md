# 黄金用例矩阵

## 1. 端到端 Go-vs-C 对比用例

端到端用例来自官方 CGO 测试目录。每个用例都应提取为固定 fixture：

```text
case name
official source directory
triceCheck.c line
raw bytes
expected text
til source
li source
framing
endianness
Go decoder args
```

| Case | 官方目录 | 关键文件 | Go 参数事实 | 覆盖场景 | 当前状态 |
| --- | --- | --- | --- | --- | --- |
| `runtime_none_direct_rtt32` | `official/_test/staticB_di_nopf_rtt32` | `cgo_test.go`, `triceConfig.h`, `SEGGER_RTT_Conf.h` | `-pf=NONE -d16 -hs=off -prefix=off -li=off -color=off` | NONE framing、direct output、RTT32、doubled 16-bit ID、S0/S2/S4 timestamp | 官方文件已提取，raw bytes 待生成 |
| `runtime_cobs_deferred_multi_uart` | `official/_test/dblB_de_multi_cobs_ua` | `cgo_test.go`, `triceConfig.h`, `triceUart.h` | `-pf=COBS -hs=off -prefix=off -li=off -color=off` | COBS framing、deferred output、multi-pack、UARTA | 官方文件已提取，raw bytes 待生成 |
| `runtime_cobs_deferred_single_uart` | `official/_test/dblB_de_single_cobs_ua` | `cgo_test.go`, `triceConfig.h`, `triceUart.h` | `-pf=COBS -hs=off -prefix=off -li=off -color=off` | COBS framing、deferred output、single-pack、UARTA | 官方文件已提取，raw bytes 待生成 |
| `runtime_tcobsv1_direct_rtt32` | `official/_test/staticB_di_tcobs_rtt32` | `cgo_test.go`, `triceConfig.h`, `SEGGER_RTT_Conf.h` | `-d16 -hs=off -prefix=off -li=off -color=off` | TCOBSv1 framing、direct output、RTT32、doubled 16-bit ID | 官方文件已提取，raw bytes 待生成 |
| `runtime_tcobsv1_deferred_multi_uart` | `official/_test/dblB_de_multi_tcobs_ua` | `cgo_test.go`, `triceConfig.h`, `triceUart.h` | `-hs=off -prefix=off -li=off -color=off` | TCOBSv1 framing、deferred output、multi-pack、UARTA | 官方文件已提取，raw bytes 待生成 |
| `runtime_tcobsv1_big_endian` | `official/_test/be_dblB_de_tcobs_ua` | `cgo_test.go`, `triceConfig.h`, `triceUart.h` | `-triceEndianness=bigEndian -singleFraming -hs=off -prefix=off -li=off -color=off` | big-endian payload、single framing、TCOBSv1 | 官方文件已提取，raw bytes 待生成 |
| `runtime_xtea_cobs_direct_rtt32` | `official/_test/staticB_di_xtea_cobs_rtt32` | `cgo_test.go`, `triceConfig.h`, `SEGGER_RTT_Conf.h` | 以目录内 `cgo_test.go` 为准 | XTEA、COBS、direct RTT32，阶段五参考 | 官方文件已提取，阶段五使用 |
| `runtime_xtea_cobs_deferred_multi_uart` | `official/_test/dblB_de_multi_xtea_cobs_ua` | `cgo_test.go`, `triceConfig.h`, `triceUart.h` | 以目录内 `cgo_test.go` 为准 | XTEA、COBS、deferred multi-pack，阶段五参考 | 官方文件已提取，阶段五使用 |

端到端 expected 文本必须来自 `official/_test/testdata/triceCheck.c` 中同一测试行的 `//exp:` 注释。不得从当前 C decoder 输出反向生成 expected。

可先执行 `runGoGoldenReport.sh` 生成 Go 侧运行报告。该脚本会把本地 `../Trice` 与 `official/` 叠加到临时工作区，并运行上表对应的官方 CGO 测试包。报告中的 `*.jsonl` 是 Go 官方测试事件流，后续 C 实现的报告应按同一 case name 输出，便于逐项对比。

## 2. Decoder 级 starter vectors

这些用例来自 `official/internal/trexDecoder/trexDecoder_test.go`，不需要 CGO raw bytes 提取，适合先手动转成 C decoder 单元测试。

| Case | 官方测试 | 输入阶段 | Framing | Endian | 输入 bytes | ID table | 期望 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `decoder_none_known_trice8_1` | `TestReadNoneFramingKnownTrice` | stream | none | little | `01 40 c0 01 2a 00 00 00` | `1 -> { Type: "TRICE8_1", Strg: "v=%d" }` | exact `v=42` |
| `decoder_cobs_unknown_id` | `TestReadCOBSFramingUnknownID` | stream | COBS | little | `06 01 40 c0 01 2a 00` | empty | contains `unknown ID` |
| `decoder_short_one_byte` | `TestReadFramedUnsupportedShortPackets` | decoded package | COBS | little | `aa` | unused | contains `unsupported short packet size 1` |
| `decoder_short_regular_selector` | `TestReadFramedUnsupportedShortPackets` | decoded package | COBS | little | `01 40` | unused | contains `unsupported short non-X0 packet size 2` |
| `decoder_cycle_error` | `TestReadCycleErrorMessage` | stream | none | little | `01 40 10 01 2a` | `1 -> { Type: "TRICE8_1", Strg: "v=%d" }` | contains `CYCLE_ERROR`, previous cycle forced to `0x11` |
| `decoder_tcobsv2_unsupported` | `TestNewSelectsPackageFraming` | config | TCOBSv2 | any | none | unused | config error, contains `unsupported framing` |

## 3. 错误路径覆盖

| 场景 | 官方参考 | C 移植要求 |
| --- | --- | --- |
| unknown ID | `TestReadCOBSFramingUnknownID` | 必须输出清晰诊断，至少包含 `unknown ID` 和 ID 值 |
| short packet, 1 byte | `TestReadFramedUnsupportedShortPackets` | 不得崩溃，不得输出伪日志 |
| short packet, selector only | `TestReadFramedUnsupportedShortPackets` | 不得把残缺 selector 当作有效 record |
| cycle mismatch | `TestReadCycleErrorMessage` | cycle check 开启时必须输出 `CYCLE_ERROR` |
| partial frame | `TestNextPackageConsumesInputUntilDelimiter` | feed 半帧时不能输出日志，必须保留等待后续 bytes |
| malformed COBS | `TestNextPackageCOBS` | 解帧错误必须显式诊断 |
| malformed TCOBSv1 | `TestNextPackageTCOBSPaths` | 解帧错误必须显式诊断 |
| TCOBSv2 | `TestNewSelectsPackageFraming` | 阶段二作为 unsupported 配置处理，不能静默 fallback |

## 4. Table loader 对照

| 对照项 | 官方文件 | C 移植要求 |
| --- | --- | --- |
| JSON TIL loader | `official/demoTIL.json`、`official/_test/testdata/testTIL.json` | ID、Type、Strg 必须严格解析 |
| JSON LI loader | `official/demoLI.json`、`official/_test/testdata/testLI.json` | 可选位置诊断必须保持路径和行号语义 |
| generated C table | `official/til.c` | 当前 `til.c` 从 upstream `main` 补充下载，不来自 1.2.5 包；用于字段结构、`bitWidth`、`paramCount` 和 format string 参考，严格黄金对比前必须记录版本关系 |
| format parser | `official/internal/fmtspec/fmtspec_test.go` | C printf spec 解析必须与 Go 行为一致 |
| decoder format output | `official/internal/trexDecoder/format_specifier_test.go` | 格式化输出必须覆盖 width、length modifier、string、buffer、float |
| decoder registry | `official/internal/decoder/registry_test.go` | decoder 注册、编码名规范化和构造函数错误路径必须与 Go 行为一致 |

## 5. 阶段划分

| 阶段 | 必须使用的官方资料 | 目标 |
| --- | --- | --- |
| Phase 2 runtime decoder | `trexDecoder_test.go`、`staticB_di_nopf_rtt32`、`dblB_de_multi_cobs_ua`、`staticB_di_tcobs_rtt32`、`be_dblB_de_tcobs_ua`、`demoTIL.json` | 覆盖 none、COBS、TCOBSv1、big-endian、基础错误路径 |
| Phase 3/4 ID table 和 generate | `demoTIL.json`、`demoLI.json`、`testTIL.json`、`testLI.json`、`fmtspec_test.go` | 覆盖 TIL/LI 解析、格式 spec 和 generated table 派生字段 |
| Phase 5 advanced | `staticB_di_xtea_cobs_rtt32`、`dblB_de_multi_xtea_cobs_ua`、X0 相关 Go 测试 | 覆盖 XTEA、X0 和高级 Trice 特性 |

## 6. 使用约束

1. `official/` 下除 `til.c` 外的文件是官方源码包提取结果，后续不要直接修改。
2. `official/til.c` 是补充下载文件，不属于 1.2.5 包内提取内容；更新时必须记录 upstream URL 和日期。
3. CGO runtime case 的测试入口、配置和 expected 来源来自 `official/_test`，C runtime 输入来自同级 `Trice/src`；生成稳定黄金报告前应保持 `Trice/src` 不变。
4. 若需要生成固定 raw bytes fixture，应放到新目录 `TriceOfficialGolden/generated/`，并记录生成来源。
5. 成功路径 expected 必须来自官方 `//exp:` 注释或官方 Go 单元测试断言。
6. 错误路径 expected 可以使用官方 Go 测试中的 required substring。
7. TCOBSv2、XTEA、X0 在未实现阶段必须测试为配置拒绝或明确 unsupported，不能使用 fallback。
8. `runGoGoldenReport.sh` 只生成 Go 侧运行报告，不生成 C 侧结论；C 侧报告应复用相同 case name。
