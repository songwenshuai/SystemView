# SEGGER 编码规范

本文档定义 SEGGER 风格 C 代码的文件组织、命名、类型、注释、头文件声明、源文件结构和代码编写规范。新增、移植或重构 SEGGER 风格代码时，应以本文档作为独立规范执行。

## 1. 总体原则

SEGGER 风格代码强调清晰、稳定、可维护和可移植。新增代码必须像人工维护的正式源码，不能保留反编译痕迹、临时占位命名、编译器优化产物或不清晰的控制流。

核心要求：

- 通用工具库默认一个公共函数对应一个 `.c` 文件。
- 产品组件代码可以按紧密相关的功能域组织 `.c` 文件，但同一文件内的公共函数必须属于同一组件和同一状态上下文。
- 文件名、公共函数名、头文件声明必须使用同一模块前缀和清晰映射关系。
- 公共函数必须使用所属产品、组件或模块前缀，例如 `SEGGER_`、`PRODUCT_MODULE_`、`PRODUCT_MODULE_SUBMODULE_`。
- 模块名、算法名、标准名使用全大写。
- 动作或描述词使用 PascalCase。
- `.h` 头文件只放声明和类型定义，不放函数头注释块。
- 函数说明放在 `.c` 文件中，紧接函数实现之前。
- SEGGER 风格 `.c` 和 `.h` 文件都必须有完整文件头和文件结尾标记。
- 注释、函数文档和代码中的字符串使用英文。
- C 代码使用 2 个空格缩进。
- 错误处理明确，不使用静默 fallback 掩盖问题。

## 2. 文件组织规范

### 2.1 源文件组织

SEGGER 通用库风格采用一个公共函数一个 `.c` 文件的组织方式：

```text
SEGGER_JSON_Parse.c         -> SEGGER_JSON_Parse()
SEGGER_MATH_Cos.c           -> SEGGER_MATH_Cos()
SEGGER_SYS_FILE_Open.c      -> SEGGER_SYS_FILE_Open()
SEGGER_NODE_FreeTreeEx.c    -> SEGGER_NODE_FreeTreeEx()
SEGGER_STREAM_PrintI32.c    -> SEGGER_STREAM_PrintI32()
SEGGER_PRINTF_HexFloat.c    -> SEGGER_PRINTF_HexFloat()
SEGGER_memcpy.c             -> SEGGER_memcpy()
SEGGER_strcpy.c             -> SEGGER_strcpy()
```

每个 `.c` 文件只包含一个主要对外函数，可以包含该函数专用的 `static` helper。禁止在同一个 `.c` 文件中放置多个无关公共函数。

SEGGER 产品组件代码可采用组件级 `.c` 文件组织方式。此时文件名表达组件功能域，文件内可以包含多个紧密相关的公共函数：

```text
PRODUCT_MODULE_Decode.c -> PRODUCT_MODULE_DECODE_Init()
PRODUCT_MODULE_Decode.c -> PRODUCT_MODULE_DECODE_Run()
```

采用组件级组织时必须满足：

- 公共函数共享同一个组件前缀，例如 `PRODUCT_MODULE_DECODE_`。
- 公共函数服务同一个上下文类型或同一个状态机，例如 `PRODUCT_MODULE_DECODE_CONTEXT`。
- `.c` 文件不能混入编码器、解码器、配置、平台适配等无关功能域。
- 头文件声明必须集中在对应公共头文件中，内部共享类型集中在对应内部头文件中。
- 若新增公共函数与现有组件上下文无直接关系，应创建新的功能域文件。

## 3. 命名规范

### 3.1 公共函数和文件命名

公共函数命名格式：

```text
SEGGER_<Function>
SEGGER_<MODULE>_<Function>
SEGGER_<MODULE>_<SUBMODULE>_<Function>
<PRODUCT>_<MODULE>_<Function>
<PRODUCT>_<MODULE>_<SUBMODULE>_<Function>
```

规则：

- `SEGGER_` 是通用 SEGGER 工具函数前缀。
- 产品组件使用产品或组件前缀，例如 `PRODUCT_MODULE_`、`PRODUCT_MODULE_SUBMODULE_`。
- 模块名使用全大写，例如 `JSON`、`MATH`、`SYS`、`NODE`、`STREAM`、`PRINTF`。
- 算法名、标准名和模式名使用全大写，例如 `CRC32`、`AES`、`SHA256`、`GCM`。
- 动作和描述词使用 PascalCase，例如 `Parse`、`Open`、`FreeTree`、`PrintI32`。
- 位宽、版本和数值后缀直接拼接，例如 `CRC8`、`SHA256`、`Ed25519`。
- 通用工具函数文件名必须与公共函数名一致，并以 `.c` 结尾。
- 产品组件源文件名使用产品前缀加功能域名，例如 `PRODUCT_MODULE_Decode.c`。
- 产品组件头文件名使用产品和模块前缀，例如 `PRODUCT_MODULE.h`、`PRODUCT_MODULE_SUBMODULE.h`、`PRODUCT_MODULE_SUBMODULE_Int.h`。

禁止混用模块名大小写：

```text
SEGGER_PrintF_HexFloat.c     Wrong
SEGGER_PRINTF_HexFloat.c     Correct

SEGGER_PRINTFMT_CTX          Wrong
SEGGER_PRINTF_CTX            Correct

PRODUCT_MODULE_Decode_Run    Wrong
PRODUCT_MODULE_DECODE_Run    Correct
```

### 3.2 Static helper 命名

文件内部 helper 使用 `static`，函数名以前导下划线开头，后接 PascalCase：

```c
static void _ValidateContext(SEGGER_PRINTF_CTX *pCtx);
static int  _WriteByte(SEGGER_STREAM_CONTEXT *pCtx, U8 Data);
```

helper 只服务当前 `.c` 文件时保留在该文件中。若多个文件需要相同 helper，应上移到模块内部接口或公共工具层，避免重复实现。

产品组件内部 helper 必须保留前导下划线，并携带完整组件前缀：

```c
static U32 _PRODUCT_MODULE_RdU32LE(const U8 *pData);
static void _PRODUCT_MODULE_DECODE_Step(PRODUCT_MODULE_DECODE_CONTEXT *pSelf, PRODUCT_MODULE_STREAM *pStream, ILEAST16 Flush);
```

当 helper 属于内部子系统时，函数名中应体现子系统边界，例如 `RANGE_DECODE`、`BITTREE_DECODE`、`LENGTH_DECODE`。多个 helper 若共享同一内部上下文类型，应使用同一内部子系统前缀。

### 3.3 类型、宏和常量命名

公共类型和宏必须带模块前缀：

```c
typedef struct SEGGER_FORMAT_CONTEXT_s SEGGER_FORMAT_CONTEXT;

#define SEGGER_PRINTF_STATUS_OVERFLOW    1
#define SEGGER_FORMAT_FLAG_SIGNED        (1u << 7)

#define PRODUCT_MODULE_STATUS_USAGE_ERROR        -101
#define PRODUCT_MODULE_CONFIG_DECODE_LEVEL_MAX   8
```

规则：

- 类型名使用全大写模块前缀，例如 `SEGGER_FORMAT_CONTEXT`。
- 结构体 tag 可使用 `_s` 后缀，例如 `SEGGER_FORMAT_CONTEXT_s`。
- 函数指针类型使用 `_FUNC` 后缀，例如 `SEGGER_FORMAT_FLUSH_FUNC`。
- 宏和枚举值使用全大写。
- 同一模块内的类型、宏、状态码前缀必须一致。
- 配置宏使用 `CONFIG` 或组件配置前缀，例如 `PRODUCT_MODULE_CONFIG_DECODE_WINDOW_MAX`。
- 状态码宏应包含 `STATUS`，错误码取值和返回值语义必须在公共 API 注释中说明。
- 内部派生尺寸和表大小使用集中宏定义，例如 `PRODUCT_MODULE_DECODE_TABLE_SIZE`，禁止在多个调用点重复写同一表达式。

### 3.4 变量命名

变量名必须表达语义，禁止使用反编译命名或无意义临时名：

```text
a1, a2, v1, v2, result      Forbidden
tmp, buf, data              Avoid when the semantic is unclear
```

推荐命名：

```text
pCtx        Pointer to context.
pSelf       Pointer to the current module context.
pBuffer     Pointer to buffer.
pData       Pointer to data.
pStream     Pointer to streaming I/O state.
pEnd        Pointer to end position.
pfFlush     Pointer to flush function.
NumBytes    Number of bytes.
DataLen     Data length.
BufferSize  Buffer capacity.
AvailIn     Number of input bytes available.
AvailOut    Number of output bytes available.
Offset      Offset in bytes.
Index       Loop or table index.
HexValue    Decoded hexadecimal value.
```

指针参数通常使用 `p` 前缀，函数指针使用 `pf` 前缀，数量和长度使用 `Num`、`Len`、`Size` 等明确后缀。

## 4. 类型使用规范

整数类型优先遵循 SEGGER 类型约定：

```text
U8, U16, U32, U64    Fixed-width unsigned integers.
I8, I16, I32, I64    Fixed-width signed integers.
ULEAST16, ILEAST16   Least-width integer types used by SEGGER components.
int                  Status, error code, index, or character-compatible return.
unsigned             Count, size, length, and capacity.
char                 Character data.
void *               Generic pointer.
const void *         Read-only input data.
```

要求：

- 同一语义不能混用 `int`、`unsigned`、`size_t` 和 SEGGER 定宽类型。
- 输入数据指针必须按语义添加 `const`。
- 计数、长度、容量优先使用 `unsigned`，除非现有接口已有明确类型。
- 错误码和成功失败结果优先使用 `int`。
- 组件既有接口使用 `ILEAST16`、`ULEAST16` 时，新增代码必须沿用对应语义，不改写为 `int` 或 `unsigned`。
- 字节缓冲区使用 `U8 *` 或 `const U8 *`。

## 5. 头文件风格

### 5.1 头文件划分

SEGGER 产品组件头文件按职责划分为公共模块头、内部头、用户配置头和默认配置头。通用结构如下：

```text
PRODUCT_MODULE.h                         Common public module header.
PRODUCT_MODULE_SUBMODULE.h               Submodule public API header.
PRODUCT_MODULE_Int.h                     Common internal header.
PRODUCT_MODULE_SUBMODULE_Int.h           Submodule internal header.
PRODUCT_MODULE_SUBMODULE_Conf.h          User or project configuration wrapper.
PRODUCT_MODULE_SUBMODULE_ConfDefaults.h  Default configuration and validation header.
```

职责边界：

- `module.h` 放公共 API、公共状态码、公共流结构、版本宏和外部调用方需要直接使用的公共类型。
- `module_Int.h` 放模块内部共享宏、内部类型、派生尺寸、内部上下文和包内共享工具定义。
- `module_Conf.h` 是用户或工程配置入口，只包含工程配置头或用户配置定义，不放默认值、不放算法逻辑、不放内部类型。
- `module_ConfDefaults.h` 放配置默认值、默认钩子和配置合法性检查。默认配置必须用 `#ifndef` 包裹，错误配置必须用 `#error` 明确停止。
- 公共 API 若要求调用方按值分配 context，公共 API 头可以包含对应 `module_Int.h` 以暴露 context 完整定义；这种暴露只服务内存分配和 API 调用，不表示内部字段可由调用方直接修改。
- 配置依赖顺序必须保持单一入口：用户配置从 `Conf.h` 进入，默认值从 `ConfDefaults.h` 补齐，内部头只消费已经确定的配置值。

如果已有同类头文件，新增内容必须继承其 include guard、段落顺序、空行和对齐风格。若没有同类文件，include guard 使用文件名派生的全大写宏：

```c
#ifndef SEGGER_FORMAT_H
#define SEGGER_FORMAT_H

...

#endif
```

SEGGER 原生文件的 include guard 不使用双下划线前缀。工程集成包装头若来自外部生成体系，应保持该体系的既有文件头、include guard 和结尾注释风格，但包装头的职责仍必须限制在配置承接范围内。

### 5.2 头文件内容

头文件负责承载接口语义和共享事实：

- 公共函数声明。
- 公共类型定义。
- 公共宏、状态码、枚举值。
- 模块内部共享定义。
- 配置项声明、默认值和配置校验。

禁止在头文件中放置函数实现。函数头注释块只放在 `.c` 文件中，头文件中只放裸函数声明。

### 5.3 C++ 兼容声明

需要被 C++ 编译单元包含的 C 头文件必须在 include guard 内使用 `extern "C"` 包裹声明区域：

```c
#ifdef __cplusplus
extern "C" {
#endif

...

#ifdef __cplusplus
}
#endif
```

`extern "C"` 放在必需 include 之后、公共声明之前。配置包装头若只转发工程配置头且不声明 C 符号，可以不添加 `extern "C"`。

### 5.4 函数声明格式

头文件中的函数声明必须满足：

- 每条声明写在一行内。
- 同一组声明中，函数名后的 `(` 对齐到同一列。
- 参数名必须保留，不能只写参数类型。
- 不在声明前添加函数头注释块。

正确示例：

```c
void SEGGER_PRINTF_HexFloat         (SEGGER_PRINTF_CTX *pCtx, char *pBuffer, char *pEnd, float  Value, int Precision);
void SEGGER_PRINTF_HexFloatShortest (SEGGER_PRINTF_CTX *pCtx, char *pBuffer, char *pEnd, float  Value);
void SEGGER_PRINTF_HexDouble        (SEGGER_PRINTF_CTX *pCtx, char *pBuffer, char *pEnd, double Value, int Precision);

void     PRODUCT_MODULE_DECODE_Init (PRODUCT_MODULE_DECODE_CONTEXT *pSelf);
ILEAST16 PRODUCT_MODULE_DECODE_Run  (PRODUCT_MODULE_DECODE_CONTEXT *pSelf, PRODUCT_MODULE_STREAM *pStream, ILEAST16 Flush);
```

错误示例：

```c
void SEGGER_PRINTF_HexFloat(SEGGER_PRINTF_CTX *pCtx,
                            char              *pBuffer,
                            char              *pEnd,
                            float              Value,
                            int                Precision);
```

同一组声明中，取 `返回类型 + 空格 + 函数名` 最长者的长度再加 1，作为 `(` 所在列。

### 5.5 类型定义格式

结构体成员按列对齐，指针星号贴近变量名或函数指针名的局部风格应与相邻代码一致。SEGGER 示例中的对齐方式如下：

```c
typedef struct SEGGER_FORMAT_CONTEXT_s {
  SEGGER_FORMAT_USER_CONTEXT *pUserCtx;
  char                       *pData;
  unsigned                    DataLen;
  unsigned                    Cnt;
  unsigned                    MaxLen;
  unsigned                    TrueCnt;
  SEGGER_FORMAT_GET_STR_FUNC *pfGetStr;
  SEGGER_FORMAT_GET_VAL_FUNC *pfGetVal;
  SEGGER_FORMAT_FLUSH_FUNC   *pfFlush;
} SEGGER_FORMAT_CONTEXT;
```

函数指针 typedef 可以按参数列换行对齐：

```c
typedef int (SEGGER_FORMAT_FLUSH_FUNC)(SEGGER_FORMAT_USER_CONTEXT *pCtx,
                                       const char                 *pData,
                                       unsigned                    DataLen);
```

### 5.6 Internal header 的 `EXTERN` 模式

内部头文件可以使用一个编译单元自标识宏来区分声明和定义。拥有模块全局对象的 `.c` 文件在包含内部头之前定义该宏，内部头根据该宏把 `EXTERN` 展开为空或 `extern`。

```c
#ifdef PRODUCT_MODULE_CORE_C
  #define EXTERN
#else
  #define EXTERN extern
#endif
```

要求：

- 自标识宏只表达当前拥有者 `.c` 文件身份，不能作为功能开关使用。
- 只有一个 `.c` 文件可以定义同一个拥有者宏。
- `EXTERN` 只用于模块全局对象或内部共享对象声明，不用于公共 API 声明。
- 模块全局状态必须集中在语义化结构体或明确分组中，避免散落多个无归属变量。
- 内部头必须仍然保持 include guard 和 `extern "C"` 结构完整。

优点：

- 全局对象只有一个定义位置，声明和定义保持单一真相源。
- 头文件可以同时服务 owner 编译单元和其他使用方，减少重复声明。
- 模块私有全局状态的所有权清晰，便于审查初始化和生命周期。

## 6. C 文件风格

### 6.1 文件头

每个 SEGGER 风格 `.c` 和 `.h` 文件必须包含完整文件头。`File` 和 `Purpose` 行不带 `*` 前缀：

```c
/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2003 - 2024  SEGGER Microcontroller GmbH                 *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File        : SEGGER_Example.c
Purpose     : Brief description of the file purpose.
              Additional details on subsequent lines.
-------------------------- END-OF-HEADER -----------------------------
*/
```

授权产品组件必须保留完整 SEGGER 产品文件头，包括产品名、版本、授权信息、`END-OF-HEADER` 分隔线、`File` 和 `Purpose` 字段。此类文件中 `File` 和 `Purpose` 可以位于 `END-OF-HEADER` 之后、注释块结束之前，必须继承同组件既有格式。工程集成包装头可以使用工程自身文件头格式，但不得替换 SEGGER 原生文件头。

### 6.2 段落顺序

`.c` 文件使用标准段落注释块分隔代码区域：

```c
/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/
```

推荐段落顺序：

```text
#include Section
Defines, configurable
Defines, fixed
Types
Local data types
Function prototypes
Static const data
Static data
Static code
Public code
Global functions
```

`#include Section` 和公共函数段落必须存在。公共函数段落可以按同组件既有风格命名为 `Public code` 或 `Global functions`。其他段落只有在实际需要时保留。

不同产品组件可能使用 `Types`、`Local data types`、`Static prototypes`、`Function prototypes`、`Function prototypes, required` 等局部段落名。新增代码必须继承同文件和同组件的段落命名、大小写、空行和顺序。禁止在同一文件中混用多个同义段落名。

### 6.3 Include section

`#include Section` 只放当前 `.c` 文件实际需要的头文件。新增 include 必须继承同目录或同模块已有排序风格。

```c
/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "SEGGER_FORMAT.h"
#include "SEGGER_FORMAT_Int.h"
```

### 6.4 Defines 和 Types

共享宏、枚举、结构体和 typedef 必须放入公共头文件或内部头文件。单个 `.c` 文件内部专用的固定常量，优先使用 `static const`；若局部风格要求宏，则放入 `Defines, fixed` 段落。

```c
/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#define HEX_BASE    10u
```

类型定义只有在必须局限于当前 `.c` 文件时才放入 `Types` 段落。若多个文件需要同一类型，应移动到对应头文件，保持单一真相源。

### 6.5 Static code 和 Public code

内部 helper 放在 `Static code` 段落，公共函数放在 `Public code` 段落。通用工具库源文件的公共函数必须是文件中唯一的对外函数；产品组件源文件可以在 `Public code` 段落放置多个同一功能域的公共函数。

```c
/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

static int _IsHexDigit(char c) {
  ...
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

int SEGGER_HexChar2Nibble(char c) {
  ...
}
```

### 6.6 文件结尾

每个 `.c` 文件最后一行必须是：

```c
/*************************** End of file ****************************/
```

### 6.7 编译单元自标识

拥有模块全局对象或需要触发内部头特殊声明模式的 `.c` 文件，可以在 include section 前定义编译单元自标识宏：

```c
#define PRODUCT_MODULE_CORE_C

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "PRODUCT_MODULE_Int.h"
```

要求：

- 自标识宏必须放在包含内部头之前。
- 自标识宏名称必须来自当前文件或当前模块拥有者角色，保持全大写模块前缀。
- 自标识宏不能控制算法行为、平台行为或配置行为。
- 自标识宏不能被多个 `.c` 文件重复定义。
- 若该文件不拥有模块全局对象，也不需要触发内部头声明差异，不应添加自标识宏。

优点：

- 编译单元身份显式，内部头中的声明模式可预测。
- 全局状态定义位置清楚，链接层面的重复定义风险降低。
- 配置宏和编译单元身份宏职责分离，避免把文件身份误用成功能开关。

## 7. 注释风格

### 7.1 总体注释原则

代码注释必须使用英文，只描述当前代码行为，不记录整理过程、历史原因或替代方案。

允许的注释内容：

- 当前函数做什么。
- 参数和返回值语义。
- 当前代码块执行什么步骤。
- 必要的边界条件、格式限制和算法说明。

禁止的注释内容：

- 反编译来源记录。
- 为什么不用其他方案。
- 临时说明。
- 与当前代码行为不一致的过时描述。

### 7.2 函数头注释格式

函数头注释只放在 `.c` 文件中，紧接函数实现之前。公共函数和复杂 `static` helper 都应包含函数头注释。

```c
/*********************************************************************
*
*       SEGGER_FunctionName()
*
*  Function description
*    Brief English description of the function.
*
*  Parameters
*    pData      Pointer to input data.
*    NumBytes   Number of bytes to process.
*
*  Return value
*    == 0    Success.
*    != 0    Error.
*
*  Additional information
*    Optional details about limits, ownership, or algorithm behavior.
*
*  Notes
*    (1) Optional numbered note.
*/
```

字段使用规则：

| 字段 | 使用条件 |
| ---- | -------- |
| `Function description` | 所有函数必须包含 |
| `Parameters` | 函数有参数时必须包含 |
| `Return value` | 函数有返回值时必须包含 |
| `Additional information` | 需要补充说明时使用 |
| `Notes` | 需要编号说明时使用 |

参数说明要描述方向、长度单位、是否允许为空、所有权和生命周期。返回值说明使用明确比较形式，例如 `== 0`、`!= 0`、`> 0`、`< 0`。

### 7.3 函数体内注释

函数体内使用 `//` 注释块分隔关键步骤：

```c
//
// Validate parameters.
//
if (pData == 0) {
  return -1;
}

//
// Process input bytes.
//
...
```

简单自解释语句不需要注释。注释用于分隔阶段，不用于重复解释单行代码。

### 7.4 格式、协议和布局说明

实现固定二进制格式、编码规则、packet 布局、共享内存布局或外部可见协议时，必须把格式说明放在靠近实现的位置。说明可以位于文件头的 `Additional information`，也可以位于负责该格式的函数注释中。

要求：

- 写清楚字段顺序、长度单位、编码方式、边界值和保留值。
- 写清楚主机字节序无关的读写规则，例如显式小端或显式大端。
- 对变长编码、压缩 ID、timestamp delta、offset、padding 等规则，应给出可核对的文字说明。
- 若结构体布局必须与外部工具、共享内存或协议字段一致，必须同时使用集中 offset 宏和编译期断言验证。
- 注释只描述当前格式和当前代码行为，不记录历史迁移过程。

优点：

- 读者可以直接用注释对照协议和代码，不需要从分散分支反推格式。
- 外部可见布局发生变化时，文档、宏和断言会一起暴露不一致。
- 新增编码函数时能复用已有规则，避免公共 API 重复写格式细节。

## 8. 代码编写风格

### 8.1 缩进、花括号和空行

基本格式：

- 使用 2 个空格缩进。
- 左花括号放在函数、`if`、`for`、`while` 同一行。
- 操作符两侧保留空格。
- 逻辑段之间用空行分隔，避免过度空行。
- 对齐应服务可读性，不做无意义的大面积格式调整。

```c
int SEGGER_Example(const U8 *pData, unsigned NumBytes) {
  if (pData == 0) {
    return -1;
  }
  return (int)NumBytes;
}
```

### 8.2 参数检查和错误处理

入口处使用 guard clause 检查前置条件：

```c
if (pCtx == 0) {
  return -1;
}
if ((pData == 0) && (NumBytes != 0u)) {
  return -1;
}
```

错误处理要求：

- 不合法输入必须明确返回错误或明确停止。
- 不静默忽略错误。
- 不通过 fallback 掩盖不确定状态。
- 返回值语义必须在函数头注释中写清楚。
- 底层库不猜测调用者意图。

### 8.3 控制流

控制流应自然、扁平、可读：

- 优先使用 guard clause 降低嵌套。
- 使用 `for`、`while`、`if` 表达算法意图。
- 避免不必要的 `goto`。
- 复杂分支规则优先集中为表驱动数据或 helper。
- 多处需要同一派生信息时，上移到权威字段、查询函数或规范化结果。

### 8.4 魔法数字和常量

裸露数字必须命名。局部常量优先 `static const`；模块共享常量放入头文件。

```c
#define ASCII_DIGIT_0    48u
#define HEX_BASE         10u
```

如果数字本身有标准语义，应在常量名中表达该语义，而不是在多个调用点重复注释。

### 8.5 标准库和内部函数调用

若项目已有 SEGGER 等价实现，标准库调用应替换为 SEGGER 前缀函数：

```text
memcpy  -> SEGGER_memcpy
memset  -> SEGGER_memset
strlen  -> SEGGER_strlen
strcmp  -> SEGGER_strcmp
```

调用前必须确认目标函数存在且已有头文件声明。未找到依赖时，不保留反编译占位函数名，应设计正式 SEGGER API 或记录缺失依赖。

### 8.6 反编译痕迹清除

整理反编译代码时必须清除：

- `a1`、`a2`、`v1`、`v2`、`result` 等无语义命名。
- 无意义临时变量和寄存器搬运变量。
- 过度强制类型转换。
- 编译器生成的循环展开、尾调用跳转和除法魔法常量。
- 不自然的 `goto` 和跳转式控制流。

正式源码应使用清晰变量名、自然控制流和标准 C 表达算法意图。

### 8.7 编译器、CPU 和静态分析适配

SEGGER 风格代码允许为不同编译器、CPU、section placement、cache line、inline 行为和静态分析工具提供适配层，但这些适配必须集中、可读、可验证。

要求：

- 编译器关键字、attribute、pragma、section placement 和 inline 差异必须封装为模块宏或公共工具宏。
- 平台或编译器条件必须显式列出已支持分支，不支持分支使用 `#error` 明确停止。
- 静态分析或 lint 抑制必须靠近触发点，并说明规则编号或具体原因。
- 禁止用宽泛的全文件抑制掩盖普通代码问题。
- 指针转换、地址转换、未对齐访问、volatile 访问等敏感操作应通过集中宏表达，宏名必须暴露语义。
- 运行时业务逻辑不能直接散落编译器私有语法；私有语法应归入配置层、平台层或公共工具层。

优点：

- 可移植差异有固定归属，核心代码保持一致。
- 静态分析例外可审查，不会扩散成全局豁免。
- 新增编译器或 CPU 支持时，修改范围集中，风险边界清楚。

## 9. 代码设计风格

SEGGER 产品组件代码不仅要求格式一致，还要求算法结构、状态所有权、错误传播和平台边界清晰。下列小节描述通用设计风格，适用于流式算法组件、共享内存传输组件、事件记录组件和平台适配组件。

### 9.1 Context 作为单一状态源

组件运行状态必须集中保存在模块 context 中，例如 `PRODUCT_MODULE_CONTEXT`。公共 API 接收 context 指针，内部 helper 只通过 context 读取和更新模块状态。

要求：

- 不使用文件级全局变量保存可实例化组件的运行状态。
- 不把同一状态拆散到多个无权威来源的位置。
- 流式处理需要暂停和恢复时，所有恢复所需状态都必须在 context 中。
- 局部变量可以缓存 context 字段以表达算法步骤，但函数退出前必须把更新后的状态写回 context。

优点：

- 支持调用方按实例管理多个组件对象。
- 支持流式输入输出和中断后继续运行。
- 状态所有权明确，便于验证重入性和资源生命周期。

### 9.2 公共 API 薄封装，内部阶段分层

公共 API 负责初始化、参数校验、驱动主流程和返回状态。算法细节拆分到 `static` helper 中，每个 helper 只处理一个明确阶段：

```text
PRODUCT_MODULE_Init          Initialize all context fields and tables.
PRODUCT_MODULE_Run           Validate API usage, move stream data, drive processing steps.
_PRODUCT_MODULE_Step         Advance the high-level state machine.
_PRODUCT_MODULE_ProcessNext  Process one logical unit.
_PRODUCT_MODULE_INPUT_*      Input reader operations.
_PRODUCT_MODULE_TABLE_*      Table lookup operations.
_PRODUCT_MODULE_OUTPUT_*     Output writer operations.
```

要求：

- 公共 API 不内联复杂算法细节。
- 内部 helper 名称必须表达所属子系统和具体动作。
- 子系统 helper 使用对应子系统 context 或主 context，不直接操作无关状态。
- 初始化函数必须显式初始化完整 context，不依赖调用方清零内存。

优点：

- 主流程可读，细节边界清晰。
- 算法阶段可以逐个验证，错误定位更直接。
- 同类初始化、读取、查表、写入和状态推进逻辑集中在权威 helper 中，避免重复实现。

### 9.3 显式状态机驱动流程

流式算法使用显式状态枚举和 `switch` 驱动，例如 `INIT`、`NEXT`、`COPY`、`DONE`、`ERROR`。状态转换必须集中在负责推进状态机的 helper 中。

要求：

- 每个状态分支只处理该状态的职责。
- 终止状态不继续转换。
- 进入错误后必须转入错误终止状态。
- 状态枚举值必须有模块前缀，状态语义必须可从名称直接判断。

优点：

- 控制流确定，暂停、继续和终止行为清晰。
- 流式接口不会依赖隐含调用顺序。
- 错误状态和完成状态不会混淆。

### 9.4 表驱动表达固定规则

协议或算法中的固定状态转换、固定尺寸和派生表应集中定义。典型状态转换可以使用 `static const` 数组表达，配置派生尺寸使用集中宏表达。

要求：

- 固定状态转换优先使用表驱动数据，不在多个分支重复编码。
- 表名必须包含完整模块或子系统前缀。
- 表索引范围必须来自集中常量或已验证状态。
- 派生尺寸、容量和表大小必须在头文件或局部权威定义中集中表达。

优点：

- 固定规则一处可查，降低分支逻辑出错概率。
- 状态转换与算法主流程分离，便于与规格对照。
- 修改配置上限时，相关数组和循环边界从同一事实派生。

### 9.5 错误记录与状态返回分离

内部算法可以把错误记录到 context 的错误寄存器中，由顶层公共 API 统一转换为返回值。公共 API 的返回值必须表达明确状态：

```text
<  0  Processing error or data error.
== 0  Call again to continue processing.
>  0  Processing complete.
```

要求：

- API 使用错误在公共入口处检查并立即返回明确错误码。
- 输入数据、协议或算法错误记录到模块错误状态中，再由顶层统一返回。
- 一旦记录错误，不得静默清除或继续报告成功。
- 为了让状态机收敛而继续执行内部步骤时，必须已经记录错误，并且最终进入错误终止状态。

优点：

- 调用方只需要检查一个返回通道。
- 内部 helper 不需要层层传递重复状态码。
- 错误传播路径固定，便于审查不会被吞掉。

### 9.6 流式接口设计

流式 API 使用输入输出 stream 结构承载缓冲区指针和剩余长度，例如 `pIn`、`AvailIn`、`pOut`、`AvailOut`。公共 API 在消费输入或产生输出时同步推进指针并减少可用长度。

要求：

- 输入输出缓冲区所有权属于调用方，组件只消费当前调用提供的范围。
- 公共入口必须校验非空输出缓冲区；存在输入字节时必须校验输入指针非空。
- 没有输入且未 flush 时应返回使用错误。
- 输出缓冲区满、内部输入不足或尚未完成时，返回继续状态。

优点：

- 适合受限内存环境，不要求一次性持有完整输入输出。
- 调用方可以精确控制缓冲区生命周期。
- 组件内部不需要动态分配内存。

### 9.7 内部前置条件写入注释

内部 helper 可以依赖调用方已经保证的前置条件，但必须在函数注释中写清楚。例如某些 helper 要求 `AvailIn > 0`，某些处理步骤要求仍有字节需要消费。

要求：

- 公共 API 负责外部参数合法性。
- 内部 helper 不重复做全部公共校验，但必须说明调用前置条件。
- 前置条件被破坏会导致越界或错误状态时，必须在上层调用路径显式保证。

优点：

- 内部代码保持专注，避免重复防御逻辑掩盖调用关系。
- 关键约束可从函数注释直接审查。
- 性能敏感路径减少无意义重复检查。

### 9.8 可移植性与可观测性钩子

底层算法代码使用 SEGGER 整数类型、显式字节序读写和配置钩子。耗时循环中调用 watchdog 宏，lint 抑制必须局部说明原因。

要求：

- 多字节数据按协议显式读取，不依赖主机字节序或未对齐访问。
- 循环中需要满足外部监控要求时使用配置宏钩子，例如 `PRODUCT_MODULE_WATCHDOG_TICK(I)`。
- lint 抑制只允许针对明确规则和明确符号，不做宽泛屏蔽。
- 宏钩子默认实现必须在 `ConfDefaults.h` 中提供，用户覆盖必须从 `Conf.h` 进入。

优点：

- 代码跨平台行为稳定。
- 长循环可接入系统 watchdog 或进度监控。
- 静态分析例外范围可控，审查成本低。

### 9.9 共享内存控制块与固定二进制布局

共享内存传输组件的运行状态不一定由普通进程内对象持有，也可以由调用方提供的共享内存控制块承载。此类组件必须把共享协议布局作为权威事实集中定义，例如 control block、up/down buffer descriptor、字段 offset、对齐和总尺寸。

要求：

- 共享内存入口必须显式传入基地址，例如 `uintptr_t Address`，不能在模块内部隐式分配默认全局控制块。
- 共享描述符中的指针或名称引用应使用相对基地址的固定宽度 offset，避免跨进程、跨核、跨映射地址空间时失效。
- 字段 offset、结构体尺寸、对齐、padding 和运行时索引计算必须集中定义，不能在各调用点重复手写偏移表达式。
- 结构体定义必须用编译期断言绑定到 offset 宏，确保 C 布局与协议布局一致。
- 被 host、debug probe 或其他 core 异步修改的字段必须显式使用 `volatile` 或统一的 volatile 读写宏访问。
- 跨共享内存发布数据时，必须在写入数据和更新发布指针之间使用明确的内存屏障或顺序约束。
- 初始化共享控制块时，必须先写完整描述符和缓冲区，再发布可被外部扫描器识别的 ID 或 magic。
- 共享内存范围、对齐、buffer offset、buffer size 和读写指针必须在访问前验证；验证失败应返回明确错误或不执行访问。

优点：

- 共享协议布局一处可查，便于与外部工具和其他 core 对接。
- 编译期断言能在布局漂移时立即失败，避免运行时协议损坏。
- 相对 offset 让同一共享内存可被不同地址空间映射。
- 显式发布顺序避免 host 或其他 core 看到半初始化状态。

### 9.10 环形缓冲区语义集中化

共享内存传输组件的 up/down buffer 通常使用环形缓冲区，并通过模式标志明确规定空间不足时的行为。此类代码应把 ring buffer 的容量规则、读写指针推进和写入策略集中到 helper 或同一功能域中。

要求：

- 环形缓冲区必须明确读写方向，例如 up buffer 表示 target 到 host，down buffer 表示 host 到 target。
- 必须保留一个不可用字节或采用等价机制区分 full 与 empty，不能依赖隐含状态猜测。
- 写入路径必须显式处理 wrap-around，不能让调用方分散处理分段写入。
- 空间不足策略必须由集中模式表达，例如 skip、trim、block、overwrite，每个模式的返回语义必须在 API 注释中写清楚。
- 固定模式值必须定义为公共宏，公共入口校验 flags 时必须拒绝 reserved bits 或未知模式。
- 读取、写入、查询可用空间必须共用同一套读写指针语义，禁止每个函数重新发明环形缓冲计算。
- 被外部消费者更新的读写指针必须按异步共享状态处理，读取后需要验证是否仍在 buffer 范围内。

优点：

- 空间不足行为可预测，调用方可以根据模式选择丢弃、截断或阻塞。
- wrap-around 和 full/empty 判定集中，降低 off-by-one 错误风险。
- 同一 buffer 描述符可服务多种传输场景，减少重复实现。

### 9.11 Lock/NoLock 双层 API

共享内存传输组件和事件记录组件都应把同步边界显式放到 API 设计中：有锁公共入口负责初始化、锁定和解锁；无锁入口负责核心操作，并在注释中说明调用前置条件。

要求：

- 需要并发保护的组件应提供清晰的锁边界，公共有锁 API 负责调用 `LOCK`/`UNLOCK`。
- 性能敏感或已由调用方持锁的路径可以提供 `NoLock` API，但名称必须明确包含 `NoLock`。
- `NoLock` API 必须在函数注释中说明不会加锁、是否会跳过初始化、调用前必须满足哪些条件。
- 有锁 API 应薄封装同名无锁核心函数，避免复制核心逻辑。
- 锁宏必须来自配置层，例如 `PRODUCT_LOCK()`、`PRODUCT_UNLOCK()`；默认锁实现只作为配置默认值，不应散落在业务逻辑中。
- 锁保护范围必须覆盖共享状态检查和修改之间的 TOCTOU 窗口；不应把检查和写入拆到不同锁区间。

优点：

- 调用方能清楚选择同步成本和责任边界。
- 核心读写逻辑只有一个实现，减少有锁/无锁路径行为分叉。
- 适合 ISR、任务、多核和 host 交互等不同运行环境。

### 9.12 运行时基础设施允许集中全局状态

算法型组件优先使用调用方传入的 context 管理实例状态。但运行时基础设施组件可能天然只有一个系统级服务实例，允许使用模块私有全局状态承载 recorder、module registry、main context 或固定通信缓冲区。

要求：

- 全局状态必须是模块私有 `static`，对外只能通过公共 API 或查询函数访问。
- 全局状态必须集中在一个语义化结构体中，例如 `PRODUCT_GLOBALS`，禁止散落大量无归属变量。
- 若存在主 context 和额外 core context，主 context 应有唯一权威位置；额外 context 由调用方显式传入并初始化。
- 全局状态初始化必须由明确的 init API 完成，不能依赖调用方知道内部字段。
- 对外暴露全局 context 指针时，只能用于明确的扩展场景，并应避免让调用方直接维护内部不变量。
- 全局运行状态和配置默认值必须分离：运行状态在 `.c` 文件中，配置默认值在 `ConfDefaults.h` 中。

优点：

- 系统级 tracing 或 transport 服务的所有权明确。
- 主流程不需要在每个事件 API 中重复传入同一 recorder 状态。
- 扩展多核或附加 buffer 时仍能保持主 context 的单一真相源。

### 9.13 事件记录流水线

事件记录组件的公共记录函数应遵循固定流水线：准备 packet buffer、编码 payload、统一补齐 event id、length 和 timestamp delta，再交给统一发送函数处理 dropped packet、overflow packet 和 host command。

要求：

- 事件记录 API 应是薄格式化层，只负责把语义参数编码成 payload。
- packet header、payload length、event id、timestamp delta 和发送策略必须由统一 helper 处理。
- 变长整数、字符串、数据块和 ID 压缩必须由集中 encode 函数或宏负责，公共 API 不重复编码规则。
- packet buffer 的生命周期策略必须集中表达，例如 static buffer 需要入口加锁，stack buffer 可以推迟到发送阶段加锁。
- dropped packet、overflow packet、post-mortem overwrite、sync packet 等传输状态必须集中在发送层处理，记录函数不各自维护。
- host command 或 back-channel 处理必须在统一发送后检查，且必须防止递归失控。
- 事件禁用 mask、restart、start/stop、post-mortem 等模式开关必须集中在发送或控制层处理，不能散布到每个事件 API。

优点：

- 新增事件时只需定义 payload 语义，不需要重新理解传输协议。
- 事件包格式一致，host 解析行为稳定。
- 丢包、溢出、同步和命令处理路径固定，审查重点集中。

### 9.14 OS 与平台适配边界

事件记录组件应使用 `PRODUCT_OS_API`、系统描述 callback 和各 OS 适配文件隔离 OS 细节。平台配置文件只负责时间源、设备名、RAM base、OS API 绑定和启动策略，核心 recorder 不直接依赖具体 OS。

要求：

- 核心组件只能依赖抽象回调结构或配置宏，不直接包含具体 OS 内部头文件。
- 每个 OS 适配文件负责把 OS task、timer、ISR、system time 等事实转换为组件公共事件。
- 配置文件只设置应用名、设备名、timestamp 频率、CPU 频率、RAM base 和 init callback，不实现核心记录逻辑。
- 系统描述字符串应由系统描述 callback 统一发送，记录函数不拼接平台描述。
- OS 适配层维护的 task 列表、资源列表等状态必须属于该适配层，不反向污染核心 recorder。
- callback 表必须由 init API 注入，核心 recorder 只在需要时调用已注册 callback。

优点：

- 核心 recorder 可跨多种 OS 和无 OS 环境复用。
- 平台差异集中在 sample、config 或 adapter 文件中，移植范围清楚。
- OS 信息转换为统一事件语义后，host 端协议保持稳定。

### 9.15 配置宏作为可移植性边界

运行时传输组件和事件记录组件通常使用 `Conf.h`/`ConfDefaults.h` 提供配置入口、默认值、锁、timestamp、buffer placement、cache line alignment、post-mortem 模式和 recorder 通知钩子。此类宏是可移植性边界，不应变成业务逻辑的隐式分支补丁。

要求：

- 用户可覆盖配置只从 `Conf.h` 进入，默认值只在 `ConfDefaults.h` 中补齐。
- 默认配置必须完整文档化用途、默认值和约束；非法组合应使用 `#error` 或明确返回错误。
- 平台特定 section、alignment、inline assembly、interrupt mask 等实现必须集中在配置层或平台隔离段。
- 可选钩子必须有空实现或明确默认实现，例如 event recorded hook、lock/unlock、timestamp 获取。
- 配置宏只表达平台能力、资源尺寸和行为策略，不能用于掩盖核心算法缺陷。
- 新增配置项前必须确认不能通过已有配置、context 字段或 callback 表达，避免配置面膨胀。

优点：

- 同一核心源码可以在不同编译器、CPU 和 OS 下复用。
- 默认值降低接入成本，明确校验避免错误配置进入运行期。
- 平台相关代码有固定归属，核心逻辑保持稳定。

## 10. 平台相关代码

平台相关实现必须显式隔离。同一函数的多平台实现可以放在同一个 `.c` 文件中，用条件编译分隔：

```c
#if defined(__linux__)
  ...
#elif defined(_WIN32) || defined(_WIN64)
  ...
#elif defined(__APPLE__) && defined(__MACH__)
  ...
#else
  #error Unsupported platform.
#endif
```

要求：

- 所有支持的平台必须明确列出。
- 不支持的平台使用 `#error` 明确报错。
- 平台特定头文件只在对应条件分支中包含。
- 平台特定魔法数字同样需要命名。
- 即使调用系统 API，也要保持 SEGGER 类型和命名风格一致。

## 11. 完整 C 文件骨架

```c
/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2003 - 2024  SEGGER Microcontroller GmbH                 *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File        : SEGGER_Example.c
Purpose     : Brief description of the file purpose.
-------------------------- END-OF-HEADER -----------------------------
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "SEGGER_EXAMPLE.h"

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#define EXAMPLE_LIMIT    16u

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _IsValidSize()
*
*  Function description
*    Checks whether the requested size is supported.
*
*  Parameters
*    NumBytes   Number of bytes requested.
*
*  Return value
*    == 0    Size is not supported.
*    != 0    Size is supported.
*/
static int _IsValidSize(unsigned NumBytes) {
  return NumBytes <= EXAMPLE_LIMIT;
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       SEGGER_Example()
*
*  Function description
*    Processes a byte buffer.
*
*  Parameters
*    pData      Pointer to input data.
*    NumBytes   Number of bytes to process.
*
*  Return value
*    == 0    Success.
*    != 0    Error.
*/
int SEGGER_Example(const U8 *pData, unsigned NumBytes) {
  //
  // Validate parameters.
  //
  if ((pData == 0) && (NumBytes != 0u)) {
    return -1;
  }
  if (_IsValidSize(NumBytes) == 0) {
    return -1;
  }
  //
  // Process data.
  //
  return 0;
}

/*************************** End of file ****************************/
```

## 12. 检查清单

新增或整理 SEGGER 风格代码前后，逐项检查：

- [ ] 文件名、公共函数名、头文件声明使用同一模块前缀并保持清晰映射。
- [ ] 通用工具库 `.c` 文件只有一个公共函数；产品组件 `.c` 文件内的公共函数属于同一功能域和同一上下文。
- [ ] `.c` 和 `.h` 文件包含完整 SEGGER 文件头。
- [ ] `.c` 文件包含 `#include Section` 和公共函数段落，段落命名继承同组件既有风格。
- [ ] `.c` 和 `.h` 文件最后一行是 End of file 标记。
- [ ] 函数头注释只放在 `.c` 文件，不放在 `.h` 文件。
- [ ] 头文件函数声明一行一个，`(` 按列对齐。
- [ ] 公共函数使用所属产品、组件或模块前缀。
- [ ] `module.h`、`module_Int.h`、`module_Conf.h`、`module_ConfDefaults.h` 职责边界清晰。
- [ ] 配置默认值只在 `ConfDefaults.h` 中定义，用户配置入口只在 `Conf.h` 中承接。
- [ ] 内部头的 `EXTERN` 模式只用于模块全局对象声明，拥有者编译单元唯一。
- [ ] `.c` 文件自标识宏只表达编译单元身份，不承担功能配置职责。
- [ ] 模块名、类型名、宏名前缀一致。
- [ ] 运行状态集中在模块 context 中，没有隐藏全局运行状态。
- [ ] 公共 API 负责参数校验和状态返回，复杂算法拆分到命名清晰的内部 helper。
- [ ] 流式算法使用显式状态机，完成状态和错误状态不会继续转换。
- [ ] 固定状态转换、派生尺寸和容量规则集中定义，没有分散重复编码。
- [ ] 内部错误记录后由公共 API 统一转换为返回值，不静默清除错误。
- [ ] 流式接口正确推进 `pIn`、`AvailIn`、`pOut`、`AvailOut`。
- [ ] 内部 helper 依赖的前置条件已在函数注释或上层调用路径中明确。
- [ ] 字节序处理、watchdog 钩子和 lint 抑制符合可移植性与可观测性要求。
- [ ] 固定格式、协议、packet 或共享布局说明靠近实现，并与集中宏和断言保持一致。
- [ ] 编译器、CPU、section、inline 和 pragma 差异集中在配置层、平台层或公共工具层。
- [ ] 静态分析例外靠近触发点，范围明确，没有宽泛全局抑制。
- [ ] 共享内存控制块的 offset、尺寸、对齐和 padding 有集中定义，并通过编译期断言验证。
- [ ] 共享描述符使用固定宽度字段和相对 offset，异步修改字段使用 volatile 语义或统一访问宏。
- [ ] 共享内存发布顺序明确，数据写入和指针/ID 发布之间有必要的内存屏障。
- [ ] 环形缓冲区的 full/empty 判定、wrap-around 和空间不足策略集中实现。
- [ ] buffer flags 只允许已定义模式，未知模式和 reserved bits 会被拒绝。
- [ ] 有锁 API 和 `NoLock` API 分层清晰，核心逻辑没有在两条路径重复实现。
- [ ] 系统级全局状态只存在于运行时基础设施组件中，并集中在模块私有状态结构体。
- [ ] 事件记录函数只负责编码 payload，packet header、timestamp、丢包和 overflow 由统一发送层处理。
- [ ] OS/平台适配通过 callback、配置宏或独立 adapter 文件隔离，核心组件不直接依赖具体 RTOS。
- [ ] 配置宏只表达平台能力、资源尺寸和行为策略，默认值和非法配置校验集中在 `ConfDefaults.h`。
- [ ] 参数名和变量名具有明确语义。
- [ ] 指针参数按语义使用 `p` 前缀和 `const`。
- [ ] 类型使用符合 SEGGER 约定，没有同一语义混用类型。
- [ ] 魔法数字已命名。
- [ ] 参数检查和错误返回语义完整。
- [ ] 没有反编译占位命名和编译器优化产物。
- [ ] 没有静默 fallback。
- [ ] 共享宏、类型和派生事实只有一个权威定义位置。
- [ ] 代码注释为英文，只描述当前行为。
- [ ] 构建文件已同步更新。
