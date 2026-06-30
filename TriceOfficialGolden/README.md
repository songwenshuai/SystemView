# Trice 官方黄金参考测试资料

本目录保存 TraceHub 移植 Trice 主机端 C 实现时需要使用的官方黄金参考资料，用于对比本地 Go 实现和后续 C 实现的行为。

目录与 `Trice` 同级，避免污染当前 `Trice` 源码目录。

## 目录结构

```text
TriceOfficialGolden/
  README.md
  GoldenCaseMatrix.md
  runGoGoldenReport.sh
  official/
    demoTIL.json
    demoLI.json
    til.c
    _test/
    internal/
    examples/
  reports/
  .work/
```

`official/` 下文件保持官方相对路径，方便和 upstream 对照。多数文件来自仓库根目录的 `trice-1.2.5.tar.gz` 官方源码包；`official/til.c` 是因为 1.2.5 包内缺失该文件，按用户要求从 upstream `main` 补充下载。

`runGoGoldenReport.sh` 不从压缩包解压。它只使用同级 `Trice` 目录和本目录 `official/` 下的文件，构造临时 Go 测试工作区并输出报告。脚本内部的 `tar` 只是在没有 `rsync` 时作为目录复制方式使用，不读取 `trice-1.2.5.tar.gz`。

## 提取范围

| 路径 | 作用 |
| --- | --- |
| `official/_test/testdata/triceCheck.c` | 官方 expected 文本来源，`//exp: "..."` 注释是端到端输出真值 |
| `official/_test/testdata/cgoPackage.go` | 官方 CGO 测试驱动，负责执行 C 日志调用、取得 raw bytes、调用 Go decoder |
| `official/_test/testdata/cgoPackage_test.go` | 官方 CGO 测试入口 |
| `official/_test/testdata/cgoTrice.c` | CGO 测试桥接代码 |
| `official/_test/testdata/testTIL.json` | 官方测试 TIL 数据 |
| `official/_test/testdata/testLI.json` | 官方测试 LI 数据 |
| `official/demoTIL.json` | 官方 demo TIL，多个 CGO 测试通过 Go 参数引用 |
| `official/demoLI.json` | 官方 demo LI |
| `official/_test/staticB_di_nopf_rtt32` | NONE framing direct RTT32 参考 |
| `official/_test/dblB_de_multi_cobs_ua` | COBS deferred multi-pack UARTA 参考 |
| `official/_test/dblB_de_single_cobs_ua` | COBS deferred single-pack UARTA 参考 |
| `official/_test/staticB_di_tcobs_rtt32` | TCOBSv1 direct RTT32 参考 |
| `official/_test/dblB_de_multi_tcobs_ua` | TCOBSv1 deferred multi-pack UARTA 参考 |
| `official/_test/be_dblB_de_tcobs_ua` | big-endian + TCOBSv1 参考 |
| `official/_test/staticB_di_xtea_cobs_rtt32` | XTEA + COBS direct RTT32 扩展参考 |
| `official/_test/dblB_de_multi_xtea_cobs_ua` | XTEA + COBS deferred 扩展参考 |
| `official/internal/trexDecoder/trexDecoder_test.go` | 官方 Go decoder 单元测试，错误路径和 starter vectors 来源 |
| `official/internal/trexDecoder/format_specifier_test.go` | format specifier 行为参考 |
| `official/internal/decoder/decoder_test.go` | decoder 通用行为参考 |
| `official/internal/decoder/registry_test.go` | decoder registry 行为参考，同时提供官方测试辅助 `fakeDecoder` |
| `official/internal/fmtspec/fmtspec_test.go` | printf spec parser 行为参考 |
| `official/til.c` | upstream `main` 生成的 C table 参考，用于 `til.json -> generated C table` 对照 |
| `official/examples/exampleData` | 补充 demo 来源，不作为 expected 真值 |

## 版本来源说明

| 内容 | 原因 |
| --- | --- |
| `trice.bin` | 官方 1.2.5 包中该文件为空，不适合作为黄金输入 |
| `official/til.c` | 官方 1.2.5 包中不存在该文件；当前文件于 2026-06-30 从 `https://raw.githubusercontent.com/rokath/trice/main/til.c` 补充下载，严格对比时需要记录它与 `demoTIL.json` 的版本关系 |
| 完整 `_test` 目录 | 目录很大且包含大量阶段五或非当前主路径组合；当前只提取移植 runtime decoder 和 table loader 需要的代表性目录 |

## Go 运行报告脚本

### 运行环境要求

`runGoGoldenReport.sh` 只负责生成 Go 黄金运行报告，不负责安装系统工具。运行前需要手动准备以下命令和环境：

| 依赖 | 必需性 | 检查命令 | 说明 |
| --- | --- | --- | --- |
| `bash` | 必需 | `bash --version` | 脚本使用 Bash 执行 |
| `go` | 必需 | `go version` | `Trice/go.mod` 声明 Go `1.24.0`，建议安装 Go 1.24 或更高版本 |
| C 编译器 | 必需 | `cc --version` 或 `clang --version` | CGO 测试会编译官方 C runtime；macOS 可通过 `xcode-select --install` 安装 Command Line Tools |
| `CGO_ENABLED=1` | 必需 | `go env CGO_ENABLED` | 脚本默认导出 `CGO_ENABLED=1`，但 Go 环境和 C 编译器必须可用 |
| `tar` | 必需 | `tar --version` 或 `tar --help` | 当系统没有 `rsync` 时，脚本使用 `tar` 复制临时工作区 |
| Go module 网络或缓存 | 必需 | `go env GOPATH GOMODCACHE` | 首次运行 `go test` 可能需要下载 `go.sum` 中的依赖；离线环境需要提前准备 Go module cache |
| `rsync` | 可选 | `rsync --version` | 存在时脚本优先用它复制工作区；缺失时使用 `tar` |
| `git` | 可选 | `git --version` | 存在时报告会记录本地 `Trice` 的 commit 和工作区状态 |
| `shasum` 或 `sha256sum` | 可选 | `shasum -a 256 --version` 或 `sha256sum --version` | 存在时报告会记录关键参考文件 hash |

macOS 上最小准备通常是：

```sh
xcode-select --install
brew install go
```

安装完成后先确认：

```sh
go version
cc --version
go env CGO_ENABLED
```

### 执行命令

执行脚本：

```sh
./TriceOfficialGolden/runGoGoldenReport.sh
```

脚本输入：

| 输入 | 作用 |
| --- | --- |
| `../Trice` | 本地 Go 实现和 C runtime 源码；`Trice/src` 是 CGO 黄金用例使用的 C runtime 输入 |
| `official/` | 官方测试、TIL/LI、CGO case、`til.c` 参考文件 |

脚本构造 `.work/go-golden-report/src` 的顺序是：

1. 删除并重建临时工作区。
2. 将同级 `Trice/` 复制到临时工作区，提供本地 Go module、`cmd`、`pkg`、`internal`、`src` 等实现代码。
3. 将 `TriceOfficialGolden/official/` 覆盖到同一个临时工作区，补入官方 `_test`、官方测试数据、`demoTIL.json`、`demoLI.json` 和 `til.c`。

覆盖后的来源规则：

| 临时工作区路径 | 来源 |
| --- | --- |
| `cmd/`、`pkg/`、`scripts/`、`go.mod`、`go.sum` | 默认来自同级 `Trice/` |
| `internal/` | 来自同级 `Trice/`，再叠加 `official/internal` 下的官方测试文件 |
| `_test/`、`demoTIL.json`、`demoLI.json`、`til.c` | 来自 `TriceOfficialGolden/official/` |
| `src/` | 来自同级 `Trice/src`；生成稳定黄金报告时应保持该目录不变 |

每次运行的报告目录中会生成 `workspace-provenance.txt`，记录本次临时工作区的实际来源。

脚本输出：

| 路径 | 内容 |
| --- | --- |
| `reports/go-golden-report-<timestamp>/report.md` | 人类可读的 Go 黄金运行报告 |
| `reports/go-golden-report-<timestamp>/summary.tsv` | 每个 case 的 PASS/FAIL、退出码、耗时和日志文件索引 |
| `reports/go-golden-report-<timestamp>/*.jsonl` | `go test -json` 原始事件流，可用于后续和 C 报告做机器对比 |
| `reports/go-golden-report-<timestamp>/*.stderr` | 每个 case 的 stderr |
| `reports/go-golden-report-<timestamp>/prepare-triceCheck.log` | 临时工作区中给 `triceCheck.c` 插入 ID 的准备阶段日志 |
| `reports/go-golden-report-<timestamp>/workspace-provenance.txt` | 临时工作区来源和叠加顺序 |
| `reports/go-golden-report-<timestamp>/go-version.txt` | Go 版本 |
| `reports/go-golden-report-<timestamp>/go-env.txt` | Go 运行环境关键信息 |
| `reports/go-golden-report-<timestamp>/reference-hashes.sha256` | 关键官方参考文件和本地 `Trice/src` 头文件 hash |

脚本会先在 `.work/go-golden-report/src` 临时工作区中用 `demoTIL.json` 和 `demoLI.json` 给 `_test/testdata/triceCheck.c` 插入 ID。这个准备步骤只修改临时工作区，不修改 `official/` 下的官方参考文件。

官方 CGO runtime 用例会遍历 `triceCheck.c` 中 1682 条 `//exp:` 参考行，部分 case 会逐行执行 C 代码并调用 Go decoder。单个 runtime case 分钟级耗时属于正常现象；脚本会在每个 case 结束时打印耗时，并在报告中记录 `elapsed_seconds`。

脚本执行的官方用例包括：

| Case | 覆盖场景 |
| --- | --- |
| `internal_decoder` | decoder 通用行为 |
| `internal_fmtspec` | printf format spec parser |
| `internal_trex_decoder` | decoder 成功路径、错误路径、unsupported framing |
| `runtime_none_direct_rtt32` | NONE framing、direct output、RTT32 |
| `runtime_cobs_deferred_multi_uart` | COBS、deferred multi-pack、UARTA |
| `runtime_cobs_deferred_single_uart` | COBS、deferred single-pack、UARTA |
| `runtime_tcobsv1_direct_rtt32` | TCOBSv1、direct output、RTT32 |
| `runtime_tcobsv1_deferred_multi_uart` | TCOBSv1、deferred multi-pack、UARTA |
| `runtime_tcobsv1_big_endian` | big-endian payload、single framing、TCOBSv1 |
| `runtime_xtea_cobs_direct_rtt32` | XTEA、COBS、direct RTT32 |
| `runtime_xtea_cobs_deferred_multi_uart` | XTEA、COBS、deferred multi-pack |

脚本会编译并运行 Go/CGO 测试。本次修改没有执行该脚本，也没有执行任何编译或测试命令。

## 使用方式

1. 先查看 [GoldenCaseMatrix.md](GoldenCaseMatrix.md)，确认要验证的场景和官方来源。
2. 执行 `runGoGoldenReport.sh` 生成本地 Go 实现的官方测试运行报告。
3. 端到端对比时，从对应 `official/_test/<case>/cgo_test.go` 读取 Go decoder 参数。
4. 从 `official/_test/testdata/triceCheck.c` 的 `//exp:` 注释读取 expected 文本。
5. 用官方 CGO 测试流程提取 raw bytes 后，同时喂给 Go decoder 和 C decoder。
6. C decoder 输出必须与官方 expected 一致；错误路径必须匹配官方 Go 单元测试中的诊断语义。

本目录只保存官方参考资料、报告脚本和索引，不接入当前 `Trice` 构建系统。
