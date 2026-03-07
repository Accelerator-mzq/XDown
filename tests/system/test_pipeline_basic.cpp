// V3.0 Pipeline Basic Test
// 测试框架在 V3.0 下的基本流转功能

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

// ========== Mock Components for V3.0 Framework ==========

class PipelineContext {
public:
    std::string data;
    bool valid;
    std::string error_message;
    
    PipelineContext() : valid(true), error_message("") {}
};

class PipelineNode {
public:
    virtual ~PipelineNode() = default;
    virtual bool process(std::shared_ptr<PipelineContext> ctx) = 0;
    virtual std::string name() const = 0;
};

// ========== Concrete Pipeline Nodes ==========

class ValidatorNode : public PipelineNode {
public:
    bool process(std::shared_ptr<PipelineContext> ctx) override {
        if (ctx->data.empty()) {
            ctx->valid = false;
            ctx->error_message = "Data is empty";
            return false;
        }
        return true;
    }
    std::string name() const override { return "ValidatorNode"; }
};

class ProcessorNode : public PipelineNode {
public:
    bool process(std::shared_ptr<PipelineContext> ctx) override {
        ctx->data = "processed:" + ctx->data;
        return true;
    }
    std::string name() const override { return "ProcessorNode"; }
};

class OutputNode : public PipelineNode {
public:
    bool process(std::shared_ptr<PipelineContext> ctx) override {
        ctx->data = "output:" + ctx->data;
        return true;
    }
    std::string name() const override { return "OutputNode"; }
};

// ========== Pipeline Orchestrator ==========

class Pipeline {
private:
    std::vector<std::shared_ptr<PipelineNode>> nodes_;
    
public:
    void addNode(std::shared_ptr<PipelineNode> node) {
        nodes_.push_back(node);
    }
    
    std::shared_ptr<PipelineContext> execute(std::shared_ptr<PipelineContext> ctx) {
        for (const auto& node : nodes_) {
            if (!node->process(ctx)) {
                break;
            }
        }
        return ctx;
    }
    
    size_t nodeCount() const { return nodes_.size(); }
};

// ========== Test Cases ==========

// 正常路径测试
TEST(PipelineTest, NormalFlow_Success) {
    auto pipeline = std::make_shared<Pipeline>();
    pipeline->addNode(std::make_shared<ValidatorNode>());
    pipeline->addNode(std::make_shared<ProcessorNode>());
    pipeline->addNode(std::make_shared<OutputNode>());
    
    auto ctx = std::make_shared<PipelineContext>();
    ctx->data = "test_data";
    
    auto result = pipeline->execute(ctx);
    
    EXPECT_TRUE(result->valid);
    EXPECT_EQ(result->data, "output:processed:test_data");
    EXPECT_EQ(pipeline->nodeCount(), 3);
}

// 正常路径测试 - 多个数据处理
TEST(PipelineTest, NormalFlow_MultipleData) {
    auto pipeline = std::make_shared<Pipeline>();
    pipeline->addNode(std::make_shared<ValidatorNode>());
    pipeline->addNode(std::make_shared<ProcessorNode>());
    pipeline->addNode(std::make_shared<OutputNode>());
    
    std::vector<std::string> testCases = {"data1", "data2", "data3"};
    
    for (const auto& data : testCases) {
        auto ctx = std::make_shared<PipelineContext>();
        ctx->data = data;
        auto result = pipeline->execute(ctx);
        EXPECT_TRUE(result->valid);
    }
}

// 异常路径测试 - 空数据
TEST(PipelineTest, AbnormalFlow_EmptyData) {
    auto pipeline = std::make_shared<Pipeline>();
    pipeline->addNode(std::make_shared<ValidatorNode>());
    pipeline->addNode(std::make_shared<ProcessorNode>());
    pipeline->addNode(std::make_shared<OutputNode>());
    
    auto ctx = std::make_shared<PipelineContext>();
    ctx->data = "";
    
    auto result = pipeline->execute(ctx);
    
    EXPECT_FALSE(result->valid);
    EXPECT_EQ(result->error_message, "Data is empty");
    EXPECT_EQ(result->data, "");  // 未经过处理
}

// 异常路径测试 - 空 pipeline
TEST(PipelineTest, AbnormalFlow_EmptyPipeline) {
    auto pipeline = std::make_shared<Pipeline>();
    
    auto ctx = std::make_shared<PipelineContext>();
    ctx->data = "test";
    
    auto result = pipeline->execute(ctx);
    
    EXPECT_TRUE(result->valid);
    EXPECT_EQ(result->data, "test");  // 数据保持不变
    EXPECT_EQ(pipeline->nodeCount(), 0);
}

// 异常路径测试 - 单节点 pipeline
TEST(PipelineTest, AbnormalFlow_SingleNode) {
    auto pipeline = std::make_shared<Pipeline>();
    pipeline->addNode(std::make_shared<ValidatorNode>());
    
    auto ctx = std::make_shared<PipelineContext>();
    ctx->data = "test";
    
    auto result = pipeline->execute(ctx);
    
    EXPECT_TRUE(result->valid);
    EXPECT_EQ(result->data, "test");  // 仅验证，未处理
}

// 边界测试 - 长数据
TEST(PipelineTest, EdgeCase_LongData) {
    auto pipeline = std::make_shared<Pipeline>();
    pipeline->addNode(std::make_shared<ValidatorNode>());
    pipeline->addNode(std::make_shared<ProcessorNode>());
    pipeline->addNode(std::make_shared<OutputNode>());
    
    std::string longData(10000, 'x');
    auto ctx = std::make_shared<PipelineContext>();
    ctx->data = longData;
    
    auto result = pipeline->execute(ctx);
    
    EXPECT_TRUE(result->valid);
    EXPECT_EQ(result->data, "output:processed:" + longData);
}

// 边界测试 - 特殊字符
TEST(PipelineTest, EdgeCase_SpecialCharacters) {
    auto pipeline = std::make_shared<Pipeline>();
    pipeline->addNode(std::make_shared<ValidatorNode>());
    pipeline->addNode(std::make_shared<ProcessorNode>());
    
    auto ctx = std::make_shared<PipelineContext>();
    ctx->data = "test\n\t\r\"'<>";
    
    auto result = pipeline->execute(ctx);
    
    EXPECT_TRUE(result->valid);
    EXPECT_EQ(result->data, "processed:test\n\t\r\"'<>");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
