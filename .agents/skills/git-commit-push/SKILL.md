---
name: git-commit-push
description: 仅基于已暂存变更创建 Conventional Commit 并推送到远程。当用户要求提交并推送代码时使用，可带提交信息，也可不带提交信息。禁止额外暂存文件。
---

# Git 提交并推送

使用此技能基于当前已暂存变更创建 Git 提交，并推送当前分支。

## 硬性规则

- 只处理已暂存文件。
- 不运行 `git add`。
- 不暂存、恢复、stash、reset 或覆盖工作区变更。
- 提交信息中不包含 AI 归因或 generated-by 页脚。
- 使用 Conventional Commits。
- 提交并推送后的只读复核如果发现新的已暂存变更，只报告它们不在本次已推送提交中；不继续提交，不改动暂存区。

## 工作流程

1. 并行收集仓库状态。
   - `pwd`
   - `git remote -v`
   - `git status`
   - `git diff --cached --name-status`
   - `git log -5 --oneline`

2. 如果没有已暂存变更，则停止操作。
   - 告知用户没有可提交的已暂存变更。
   - 不运行 `git add`。

3. 检查已暂存变更。
   - 阅读 `git diff --cached`。
   - 根据暂存 diff、暂存文件列表和近期提交历史推断提交类型、作用域和描述。
   - 如果用户提供了提交信息，将其作为主要意图，同时验证它是否匹配暂存 diff。

4. 编写提交信息。
   - 格式：

```text
<type>(<scope>): <description>

<optional body>
```

   - 使用以下类型之一：`feat`、`fix`、`docs`、`style`、`refactor`、`test`、`chore`、`perf`、`ci` 或 `build`。
   - 根据主要暂存路径推断 scope。
   - description 使用祈使动词开头。
   - subject 保持简洁，尽量不超过 72 个字符。
   - 仅在需要说明多项变更或非显而易见意图时添加正文。

5. 提交并推送。
   - 使用生成的提交信息运行 `git commit`。
   - 提交成功后运行 `git push`。
   - 如果任一命令失败，报告失败原因和相关输出。除非用户明确要求，否则不要通过改变暂存区或工作区状态来恢复。

6. 推送后只读复核状态。
   - 运行 `git status` 和 `git diff --cached --name-status`。
   - 如果发现新的已暂存变更，明确告知用户这些文件不在本次已推送提交中。
   - 不继续创建第二个提交。
   - 不暂存、取消暂存、恢复、stash、reset 或覆盖这些变更。

7. 报告完成结果。
   - 包含提交 subject。
   - 如果命令输出中包含分支或远程信息，一并说明。
