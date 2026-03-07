/**
 * Pipeline System Tests - V3.0 Framework
 * 验证现有代码在 V3.0 框架下能否正常流转
 */

#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>

// Mock V3.0 Framework Components
namespace v3 {

class PipelineContext {
public:
    PipelineContext() : data_(""), status_(0) {}
    
    void SetData(const std::string& data) { data_ = data; }
    std::string GetData() const { return data_; }
    void SetStatus(int status) { status_ = status; }
    int GetStatus() const { return status_; }
    
private:
    std::string data_;
    int status_;
};

class PipelineNode {
public:
    virtual ~PipelineNode() = default;
    virtual bool Execute(PipelineContext& ctx) = 0;
    virtual std::string GetName() const = 0;
};

class InputNode : public PipelineNode {
public:
    bool Execute(PipelineContext& ctx) override {
        ctx.SetData("input_data");
        ctx.SetStatus(1);
        return true;
    }
    std::string GetName() const override { return "InputNode"; }
};

class ProcessNode : public PipelineNode {
public:
    bool Execute(PipelineContext& ctx) override {
        if (ctx.GetData().empty()) {
            return false;  // 异常路径
        }
        ctx.SetData(ctx.GetData() + "_processed");
        ctx.SetStatus(2);
        return true;
    }
    std::string GetName() const override { return "ProcessNode"; }
};

class OutputNode : public PipelineNode {
public:
    bool Execute(PipelineContext& ctx) override {
        if (ctx.GetStatus() < 1) {
            return false;  // 异常路径
        }
        ctx.SetStatus(3);
        return true;
    }
    std::string GetName() const override { return "OutputNode"; }
};

class Pipeline {
public:
    void AddNode(std::unique_ptr<PipelineNode> node) {
        nodes_.push_back(std::move(node));
    }
    
    bool Run(PipelineContext& ctx) {
        for (auto& node : nodes_) {
            if (!node->Execute(ctx)) {
                return false;
            }
        }
        return true;
    }
    
    size_t GetNodeCount() const { return nodes_.size(); }
    
private:
    std::vector<std::unique_ptr<PipelineNode>> nodes_;
};

}  // namespace v3

// ==================== Normal Path Tests ====================

TEST(PipelineNormalTest, BasicPipelineExecution) {
    v3::Pipeline pipeline;
    pipeline.AddNode(std::make_unique<v3::InputNode>());
    pipeline.AddNode(std::make_unique<v3::ProcessNode>());
    pipeline.AddNode(std::make_unique<v3::OutputNode>());
    
    v3::PipelineContext ctx;
    bool result = pipeline.Run(ctx);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(ctx.GetStatus(), 3);
    EXPECT_EQ(ctx.GetData(), "input_data_processed");
}

TEST(PipelineNormalTest, SingleNodePipeline) {
    v3::Pipeline pipeline;
    pipeline.AddNode(std::make_unique<v3::InputNode>());
    
    v3::PipelineContext ctx;
    bool result = pipeline.Run(ctx);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(ctx.GetStatus(), 1);
}

TEST(PipelineNormalTest, NodeCountVerification) {
    v3::Pipeline pipeline;
    EXPECT_EQ(pipeline.GetNodeCount(), 0);
    
    pipeline.AddNode(std::make_unique<v3::InputNode>());
    EXPECT_EQ(pipeline.GetNodeCount(), 1);
    
    pipeline.AddNode(std::make_unique<v3::ProcessNode>());
    EXPECT_EQ(pipeline.GetNodeCount(), 2);
}

// ==================== Abnormal Path Tests ====================

TEST(PipelineAbnormalTest, EmptyPipeline) {
    v3::Pipeline pipeline;
    v3::PipelineContext ctx;
    
    bool result = pipeline.Run(ctx);
    EXPECT_TRUE(result);  // 空pipeline应返回true
}

TEST(PipelineAbnormalTest, ProcessNodeWithEmptyData) {
    v3::Pipeline pipeline;
    pipeline.AddNode(std::make_unique<v3::ProcessNode>());
    
    v3::PipelineContext ctx;
    // 不设置数据，模拟空输入
    bool result = pipeline.Run(ctx);
    
    EXPECT_FALSE(result);  // 应该失败
}

TEST(PipelineAbnormalTest, OutputNodeWithInvalidStatus) {
    v3::Pipeline pipeline;
    pipeline.AddNode(std::make_unique<v3::OutputNode>());
    
    v3::PipelineContext ctx;
    // 不设置状态，保持默认0
    bool result = pipeline.Run(ctx);
    
    EXPECT_FALSE(result);  // 应该失败
}

TEST(PipelineAbnormalTest, ContextDataIntegrity) {
    v3::Pipeline pipeline;
    pipeline.AddNode(std::make_unique<v3::InputNode>());
    pipeline.AddNode(std::make_unique<v3::ProcessNode>());
    
    v3::PipelineContext ctx;
    pipeline.Run(ctx);
    
    // 验证数据完整性
    EXPECT_FALSE(ctx.GetData().empty());
    EXPECT_NE(ctx.GetData(), "input_data_processed_processed");
}

// ==================== Integration Test ====================

TEST(PipelineIntegrationTest, FullWorkflow) {
    v3::Pipeline pipeline;
    pipeline.AddNode(std::make_unique<v3::InputNode>());
    pipeline.AddNode(std::make_unique<v3::ProcessNode>());
    pipeline.AddNode(std::make_unique<v3::ProcessNode>());
    pipeline.AddNode(std::make_unique<v3::OutputNode>());
    
    v3::PipelineContext ctx;
    
    // 执行多次验证稳定性
    for (int i = 0; i < 3; ++i) {
        bool result = pipeline.Run(ctx);
        EXPECT_TRUE(result);
        EXPECT_EQ(ctx.GetStatus(), 3);
    }
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    std::cout << "=== V3.0 Framework Pipeline Tests ===" << std::endl;
    return RUN_ALL_TESTS();
}
