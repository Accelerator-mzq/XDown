# 自动化测试执行

> **路径规则**：所有文档路径以 `smartroute.config.json` 中的 `documents` 配置为准，不要使用硬编码路径。

## 第一步：读取配置
1. 读取 `smartroute.config.json` 中 `documents` 配置，确定以下路径：
   - `system_test_cases`：系统测试用例文档路径
   - `automation_plan`：自动化测试方案路径

## 第二步：逐条执行
1. 读取系统测试用例和自动化测试方案
2. 按照测试项逐条执行自动化测试

执行规则：
- 每条测试执行前先确认自动化方案可行
- 如果自动化方案实施失败，尝试修复（最多 3 轮）
- 如果 3 轮后仍失败，在报告中标注"建议手工测试"并说明原因
- 自动化测试发现的 Bug，先尝试定位根因
- 如果定位困难（经过多轮尝试无果），明确说明"需要升级到 opus 模型进行深度分析"
- 定位到根因后给出修改方案，然后执行修改
- 每次修改后必须编译通过
- 修改后重新运行相关测试确认修复

## 第三步：输出报告
保存到 `.pipeline/test-auto-report.md`：
- 测试例编号、名称、结果（通过/失败/需手工测试）
- 失败项的根因分析和修复记录
- 最终通过率统计

## Run-Test.ps1 系统测试技能 - 强制约束

当修改了底层业务逻辑（如网络、数据库、文件 IO）后，**必须**调用此脚本进行无头自动化测试，严禁要求用户手动点击 UI 验证。

**调用方式 (PowerShell)：**
`.\Run-Test.ps1 -Module <模块名> -Param "<参数>"`
例如测试下载：`.\Run-Test.ps1 -Module net -Param "https://www.w3.org/WAI/ER/tests/xhtml/testfiles/resources/pdf/dummy.pdf"`

**反馈分析规则：**
1. 脚本运行结束后，仔细阅读终端输出的日志
2. 日志末尾包含 `[TEST_RESULT: SUCCESS]` → 测试通过，向用户汇报成果
3. 日志末尾包含 `[TEST_RESULT: FAILED]` → 翻阅终端的 `qDebug` 错误信息，自主修改 C++ 代码，重新编译 (`cmake --build build`)，并**再次调用该脚本**，不断循环直到看到 SUCCESS
