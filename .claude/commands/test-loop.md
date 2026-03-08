---
description: ﻿# 阶段5-9：兵工厂自动执行（Python Orchestrator）
---

﻿# 阶段5-9：兵工厂自动执行（Python Orchestrator）

此命令是主线入口，严格覆盖阶段5-9：
- 5 Planner
- 6 Coder
- 7 Test Coder
- 8 Runtime
- 9 Fixer / Debug Expert

执行：
```bash
cd $PROJECT_DIR
python3 .pipeline/test_loop.py \
  --project-dir . \
  --task .smartroute/task.md \
  --max-retries 3
```

若需漏测反思：
```bash
python3 .pipeline/test_loop.py \
  --project-dir . \
  --task .smartroute/task.md \
  --max-retries 3 \
  --bug-report "$ARGUMENTS"
```

执行后检查：
- `.smartroute/Execution_Plan.json`
- `.pipeline/test-loop-report.md`
- `.pipeline/logs/trace.jsonl`
- `.smartroute/artifacts/execution_xxx/`

约束：阶段5-9不应被CLI手工拆散替代。

> **⚠️ [重要配置回写规则（AI 必读）]**：
> 如果你在上述任何阶段（如编码、补充测试）摸索跑通了新的编译命令、系统测试命令或单元测试命令，你**必须主动更新**项目根目录下的 `.env` 文件以及本 `CLAUDE.md` 文件中的"项目构建信息"区块。这是为了确保 `test_loop.py` 自动化测试流转时能拉起正确的最新指令。
