// V3.0 Pipeline Unit Test
// 单元测试 - 各个节点的独立功能

#include <gtest/gtest.h>
#include <memory>
#include <string>

// ========== Test Context ==========

struct TestContext {
    std::string input;
    std::string output;
    bool success;
    std::string error;
    
    TestContext() : success(true), error("") {}
};

// ========== Mock Nodes ==========

class UnitValidator {
public:
    static bool validate(const std::string& data, TestContext& ctx) {
        if (data.empty()) {
            ctx.success = false;
            ctx.error = "Empty input";
            return false;
        }
        if (data.length() > 1000) {
            ctx.success = false;
            ctx.error = "Input too long";
            return false;
        }
        return true;
    }
};

class UnitProcessor {
public:
    static std::string process(const std::string& data, TestContext& ctx) {
        if (!ctx.success) return "";
        return "[processed]" + data;
    }
};

class UnitFormatter {
public:
    static std::string format(const std::string& data, TestContext& ctx) {
        if (!ctx.success) return "";
        return "<formatted>" + data + "</formatted>";
    }
};

// ========== Unit Tests ==========

// Validator 正常路径
TEST(UnitValidatorTest, Normal_ValidInput) {
    TestContext ctx;
    bool result = UnitValidator::validate("valid_data", ctx);
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(ctx.success);
    EXPECT_EQ(ctx.error, "");
}

// Validator 异常路径 - 空输入
TEST(UnitValidatorTest, Abnormal_EmptyInput) {
    TestContext ctx;
    bool result = UnitValidator::validate("", ctx);
    
    EXPECT_FALSE(result);
    EXPECT_FALSE(ctx.success);
    EXPECT_EQ(ctx.error, "Empty input");
}

// Validator 异常路径 - 超长输入
TEST(UnitValidatorTest, Abnormal_InputTooLong) {
    TestContext ctx;
    std::string longInput(1001, 'a');
    bool result = UnitValidator::validate(longInput, ctx);
    
    EXPECT_FALSE(result);
    EXPECT_FALSE(ctx.success);
    EXPECT_EQ(ctx.error, "Input too long");
}

// Processor 正常路径
TEST(UnitProcessorTest, Normal_ProcessData) {
    TestContext ctx;
    ctx.success = true;
    
    std::string result = UnitProcessor::process("test", ctx);
    
    EXPECT_EQ(result, "[processed]test");
    EXPECT_TRUE(ctx.success);
}

// Processor 异常路径 - 失败状态
TEST(UnitProcessorTest, Abnormal_ProcessOnFailedContext) {
    TestContext ctx;
    ctx.success = false;
    
    std::string result = UnitProcessor::process("test", ctx);
    
    EXPECT_EQ(result, "");  // 不应处理失败的数据
}

// Formatter 正常路径
TEST(UnitFormatterTest, Normal_FormatData) {
    TestContext ctx;
    ctx.success = true;
    
    std::string result = UnitFormatter::format("data", ctx);
    
    EXPECT_EQ(result, "<formatted>data</formatted>");
    EXPECT_TRUE(ctx.success);
}

// Formatter 异常路径 - 失败状态
TEST(UnitFormatterTest, Abnormal_FormatOnFailedContext) {
    TestContext ctx;
    ctx.success = false;
    
    std::string result = UnitFormatter::format("data", ctx);
    
    EXPECT_EQ(result, "");  // 不应格式化失败的数据
}

// 集成测试 - 完整流程
TEST(IntegrationTest, Normal_FullPipeline) {
    TestContext ctx;
    std::string input = "hello";
    
    // Step 1: Validate
    EXPECT_TRUE(UnitValidator::validate(input, ctx));
    
    // Step 2: Process
    std::string processed = UnitProcessor::process(input, ctx);
    
    // Step 3: Format
    std::string formatted = UnitFormatter::format(processed, ctx);
    
    EXPECT_EQ(formatted, "<formatted>[processed]hello</formatted>");
}

// 集成测试 - 异常流程
TEST(IntegrationTest, Abnormal_FailedAtValidation) {
    TestContext ctx;
    std::string input = "";
    
    // Step 1: Validate - should fail
    EXPECT_FALSE(UnitValidator::validate(input, ctx));
    
    // Step 2: Process - should skip due to failure
    std::string processed = UnitProcessor::process(input, ctx);
    EXPECT_EQ(processed, "");  // 失败状态不处理
    
    // Step 3: Format - should skip
    std::string formatted = UnitFormatter::format(processed, ctx);
    EXPECT_EQ(formatted, "");  // 失败状态不格式化
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
