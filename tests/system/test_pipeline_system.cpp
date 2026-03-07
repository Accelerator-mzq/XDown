/**
 * Pipeline System Tests for V3.0 Framework
 * 
 * Tests the basic pipeline flow to ensure the framework can operate correctly.
 * This test covers normal paths and error handling scenarios.
 */

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <stdexcept>

// ============================================================================
// Mock Pipeline Components for V3.0 Framework
// ============================================================================

class PipelineContext {
public:
    std::string pipeline_id;
    std::string current_stage;
    std::vector<std::string> execution_log;
    std::map<std::string, std::string> metadata;
    bool is_completed = false;
    bool has_error = false;
    std::string last_error;

    void log(const std::string& message) {
        execution_log.push_back(message);
    }
};

enum class StageStatus {
    Pending,
    Running,
    Completed,
    Failed
};

class PipelineStage {
public:
    std::string name;
    std::function<StageStatus(PipelineContext&)> executor;
    StageStatus status = StageStatus::Pending;

    PipelineStage(const std::string& stage_name, 
                 std::function<StageStatus(PipelineContext&)> exec)
        : name(stage_name), executor(exec) {}
};

class Pipeline {
private:
    std::string pipeline_id;
    std::vector<std::shared_ptr<PipelineStage>> stages;
    std::shared_ptr<PipelineContext> context;

public:
    Pipeline(const std::string& id) : pipeline_id(id) {
        context = std::make_shared<PipelineContext>();
        context->pipeline_id = id;
    }

    void add_stage(std::shared_ptr<PipelineStage> stage) {
        stages.push_back(stage);
    }

    bool execute() {
        context->current_stage = "init";
        context->log("Pipeline started: " + pipeline_id);

        for (const auto& stage : stages) {
            context->current_stage = stage->name;
            context->log("Executing stage: " + stage->name);
            stage->status = StageStatus::Running;

            try {
                StageStatus result = stage->executor(*context);
                stage->status = result;

                if (result == StageStatus::Failed) {
                    context->has_error = true;
                    context->last_error = "Stage failed: " + stage->name;
                    context->log("Stage failed: " + stage->name);
                    return false;
                }

                context->log("Stage completed: " + stage->name);
            } catch (const std::exception& e) {
                stage->status = StageStatus::Failed;
                context->has_error = true;
                context->last_error = e.what();
                context->log(std::string("Exception in stage: ") + e.what());
                return false;
            }
        }

        context->is_completed = true;
        context->log("Pipeline completed successfully");
        return true;
    }

    std::shared_ptr<PipelineContext> get_context() const {
        return context;
    }

    const std::vector<std::shared_ptr<PipelineStage>>& get_stages() const {
        return stages;
    }
};

// ============================================================================
// Test Cases
// ============================================================================

class PipelineTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test: Normal pipeline execution
TEST_F(PipelineTest, NormalExecution_Success) {
    auto pipeline = std::make_shared<Pipeline>("test_pipeline_001");

    // Stage 1: Initialization
    auto stage1 = std::make_shared<PipelineStage>("init", 
        [](PipelineContext& ctx) -> StageStatus {
            ctx.metadata["initialized"] = "true";
            return StageStatus::Completed;
        });
    
    // Stage 2: Processing
    auto stage2 = std::make_shared<PipelineStage>("process",
        [](PipelineContext& ctx) -> StageStatus {
            ctx.metadata["processed"] = "true";
            return StageStatus::Completed;
        });

    // Stage 3: Finalization
    auto stage3 = std::make_shared<PipelineStage>("finalize",
        [](PipelineContext& ctx) -> StageStatus {
            ctx.metadata["finalized"] = "true";
            return StageStatus::Completed;
        });

    pipeline->add_stage(stage1);
    pipeline->add_stage(stage2);
    pipeline->add_stage(stage3);

    bool result = pipeline->execute();

    EXPECT_TRUE(result);
    EXPECT_TRUE(pipeline->get_context()->is_completed);
    EXPECT_FALSE(pipeline->get_context()->has_error);
    EXPECT_EQ(pipeline->get_context()->execution_log.size(), 7); // 1 start + 3 stage starts + 3 stage completes
    EXPECT_EQ(pipeline->get_context()->metadata["initialized"], "true");
    EXPECT_EQ(pipeline->get_context()->metadata["processed"], "true");
    EXPECT_EQ(pipeline->get_context()->metadata["finalized"], "true");
}

// Test: Pipeline with failing stage
TEST_F(PipelineTest, AbnormalExecution_StageFailure) {
    auto pipeline = std::make_shared<Pipeline>("test_pipeline_failure");

    auto stage1 = std::make_shared<PipelineStage>("success_stage",
        [](PipelineContext& ctx) -> StageStatus {
            return StageStatus::Completed;
        });

    auto stage2 = std::make_shared<PipelineStage>("fail_stage",
        [](PipelineContext& ctx) -> StageStatus {
            return StageStatus::Failed;
        });

    auto stage3 = std::make_shared<PipelineStage>("never_reached",
        [](PipelineContext& ctx) -> StageStatus {
            return StageStatus::Completed;
        });

    pipeline->add_stage(stage1);
    pipeline->add_stage(stage2);
    pipeline->add_stage(stage3);

    bool result = pipeline->execute();

    EXPECT_FALSE(result);
    EXPECT_FALSE(pipeline->get_context()->is_completed);
    EXPECT_TRUE(pipeline->get_context()->has_error);
    EXPECT_EQ(pipeline->get_context()->last_error, "Stage failed: fail_stage");
    EXPECT_EQ(stage3->status, StageStatus::Pending); // Never executed
}

// Test: Pipeline with exception
TEST_F(PipelineTest, AbnormalExecution_ExceptionHandling) {
    auto pipeline = std::make_shared<Pipeline>("test_pipeline_exception");

    auto stage1 = std::make_shared<PipelineStage>("exception_stage",
        [](PipelineContext& ctx) -> StageStatus {
            throw std::runtime_error("Simulated exception");
        });

    pipeline->add_stage(stage1);

    bool result = pipeline->execute();

    EXPECT_FALSE(result);
    EXPECT_TRUE(pipeline->get_context()->has_error);
    EXPECT_EQ(pipeline->get_context()->last_error, "Simulated exception");
}

// Test: Empty pipeline
TEST_F(PipelineTest, NormalExecution_EmptyPipeline) {
    auto pipeline = std::make_shared<Pipeline>("empty_pipeline");

    bool result = pipeline->execute();

    EXPECT_TRUE(result);
    EXPECT_TRUE(pipeline->get_context()->is_completed);
    EXPECT_FALSE(pipeline->get_context()->has_error);
}

// Test: Pipeline execution log
TEST_F(PipelineTest, NormalExecution_ExecutionLog) {
    auto pipeline = std::make_shared<Pipeline>("log_test_pipeline");

    auto stage1 = std::make_shared<PipelineStage>("stage_a",
        [](PipelineContext& ctx) -> StageStatus {
            return StageStatus::Completed;
        });

    auto stage2 = std::make_shared<PipelineStage>("stage_b",
        [](PipelineContext& ctx) -> StageStatus {
            return StageStatus::Completed;
        });

    pipeline->add_stage(stage1);
    pipeline->add_stage(stage2);

    pipeline->execute();

    const auto& log = pipeline->get_context()->execution_log;
    EXPECT_GT(log.size(), 0);
    EXPECT_EQ(log[0], "Pipeline started: log_test_pipeline");
}

// Test: Multiple pipelines sequential execution
TEST_F(PipelineTest, NormalExecution_MultiplePipelines) {
    std::vector<std::shared_ptr<Pipeline>> pipelines;

    for (int i = 0; i < 3; ++i) {
        auto pipeline = std::make_shared<Pipeline>("pipeline_" + std::to_string(i));
        
        auto stage = std::make_shared<PipelineStage>("stage_" + std::to_string(i),
            [i](PipelineContext& ctx) -> StageStatus {
                ctx.metadata["pipeline_index"] = std::to_string(i);
                return StageStatus::Completed;
            });
        
        pipeline->add_stage(stage);
        pipelines.push_back(pipeline);
    }

    for (const auto& p : pipelines) {
        EXPECT_TRUE(p->execute());
        EXPECT_TRUE(p->get_context()->is_completed);
    }
}

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
