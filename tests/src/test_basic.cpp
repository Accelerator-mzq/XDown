/**
 * Basic Unit Tests - V3.0 Framework
 * 基础单元测试验证
 */

#include <gtest/gtest.h>
#include <string>
#include <vector>

// ==================== Context Tests ====================

TEST(ContextTest, DefaultConstruction) {
    v3::PipelineContext ctx;
    EXPECT_EQ(ctx.GetData(), "");
    EXPECT_EQ(ctx.GetStatus(), 0);
}

TEST(ContextTest, DataSettingAndGetting) {
    v3::PipelineContext ctx;
    ctx.SetData("test_data");
    EXPECT_EQ(ctx.GetData(), "test_data");
    
    ctx.SetData("");
    EXPECT_EQ(ctx.GetData(), "");
}

TEST(ContextTest, StatusSettingAndGetting) {
    v3::PipelineContext ctx;
    ctx.SetStatus(100);
    EXPECT_EQ(ctx.GetStatus(), 100);
    
    ctx.SetStatus(-1);
    EXPECT_EQ(ctx.GetStatus(), -1);
}

// ==================== Node Tests ====================

TEST(NodeTest, InputNodeExecution) {
    v3::InputNode node;
    v3::PipelineContext ctx;
    
    bool result = node.Execute(ctx);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(node.GetName(), "InputNode");
    EXPECT_EQ(ctx.GetData(), "input_data");
    EXPECT_EQ(ctx.GetStatus(), 1);
}

TEST(NodeTest, ProcessNodeNormal) {
    v3::ProcessNode node;
    v3::PipelineContext ctx;
    ctx.SetData("test");
    
    bool result = node.Execute(ctx);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(ctx.GetData(), "test_processed");
    EXPECT_EQ(ctx.GetStatus(), 2);
}

TEST(NodeTest, ProcessNodeAbnormal) {
    v3::ProcessNode node;
    v3::PipelineContext ctx;
    // 不设置数据
    
    bool result = node.Execute(ctx);
    
    EXPECT_FALSE(result);
}

TEST(NodeTest, OutputNodeNormal) {
    v3::OutputNode node;
    v3::PipelineContext ctx;
    ctx.SetStatus(2);
    
    bool result = node.Execute(ctx);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(ctx.GetStatus(), 3);
}

TEST(NodeTest, OutputNodeAbnormal) {
    v3::OutputNode node;
    v3::PipelineContext ctx;
    // 不设置状态
    
    bool result = node.Execute(ctx);
    
    EXPECT_FALSE(result);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
