---
description: ﻿# 辅助命令：阶段8 自动化测试执行（Runtime / Python）
---

﻿# 辅助命令：阶段8 自动化测试执行（Runtime / Python）

说明：仅用于手工补跑自动化验证，主线仍推荐 `/test-loop`。

执行：
1. 读取 `runtime.compile_command/test_command/unit_test_command`。
2. 执行编译、系统测试、单元测试。
3. 输出运行报告与失败日志。

报告输出：`.pipeline/test-auto-report.md`

> **⚠️ [重要配置回写]**：每当系统自动化测试方案跑通并确定了最终的执行命令后，你**必须主动更新** `.env` 文件中的 `TEST_COMMAND` 变量，并同步更新 `CLAUDE.md` 底部项目构建信息中的 `系统测试命令`。这是保证 test-loop 脚本正常运转的核心前提。
