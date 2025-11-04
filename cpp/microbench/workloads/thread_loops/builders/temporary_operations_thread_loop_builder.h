#pragma once

#include "workloads/args_generators/args_generator_builder.h"
#include "workloads/args_generators/builders/default_args_generator_builder.h"
#include "workloads/thread_loops/impls/temporary_operations_thread_loop.h"
#include "workloads/thread_loops/ratio_thread_loop_parameters.h"
#include "workloads/thread_loops/thread_loop_builder.h"

namespace microbench::workload {

struct TemporaryOperationsThreadLoopBuilder : public ThreadLoopBuilder {
    size_t stagesNumber = 0;
    size_t* stagesDurations;
    RatioThreadLoopParameters** ratios;

    ArgsGeneratorBuilder* argsGeneratorBuilder = new DefaultArgsGeneratorBuilder();

    TemporaryOperationsThreadLoopBuilder* set_stages_number(const size_t stages_number) {
        stagesNumber = stages_number;
        ratios = new RatioThreadLoopParameters*[stages_number];
        stagesDurations = new size_t[stages_number];

        for (size_t i = 0; i < stages_number; ++i) {
            ratios[i] = new RatioThreadLoopParameters();
        }

        return this;
    }

    TemporaryOperationsThreadLoopBuilder* set_stage_duration(const size_t index,
                                                             size_t stage_duration) {
        assert(index < stagesNumber);
        stagesDurations[index] = stage_duration;
        return this;
    }

    TemporaryOperationsThreadLoopBuilder* set_stages_durations(size_t* stages_durations) {
        stagesDurations = stages_durations;
        return this;
    }

    TemporaryOperationsThreadLoopBuilder* set_ins_ratio(const size_t index, double ins_ratio) {
        assert(index < stagesNumber);
        ratios[index]->INS_RATIO = ins_ratio;
        return this;
    }

    TemporaryOperationsThreadLoopBuilder* set_rem_ratio(const size_t index, double rem_ratio) {
        assert(index < stagesNumber);
        ratios[index]->REM_RATIO = rem_ratio;
        return this;
    }

    TemporaryOperationsThreadLoopBuilder* set_rq_ratio(const size_t index, double rq_ratio) {
        assert(index < stagesNumber);
        ratios[index]->RQ_RATIO = rq_ratio;
        return this;
    }

    TemporaryOperationsThreadLoopBuilder* set_ratios(const size_t index,
                                                     RatioThreadLoopParameters* ratio) {
        assert(index < stagesNumber);
        ratios[index] = ratio;
        return this;
    }

    TemporaryOperationsThreadLoopBuilder* set_ratios(RatioThreadLoopParameters** ratios) {
        ratios = ratios;
        return this;
    }

    TemporaryOperationsThreadLoopBuilder* set_args_generator_builder(
        ArgsGeneratorBuilder* args_generator_builder) {
        argsGeneratorBuilder = args_generator_builder;
        return this;
    }

    TemporaryOperationsThreadLoopBuilder* init(int range) override {
        ThreadLoopBuilder::init(range);
        argsGeneratorBuilder->init(range);
        return this;
    }

    TemporaryOperationThreadLoop* build(globals_t* g, Random64& rng, size_t tid,
                                        StopCondition* stop_condition) override {
        return new TemporaryOperationThreadLoop(g, rng, tid, stop_condition, this->RQ_RANGE,
                                                stagesNumber, stagesDurations, ratios,
                                                argsGeneratorBuilder->build(rng));
    }

    void to_json(nlohmann::json& j) const override {
        j["ClassName"] = "TemporaryOperationsThreadLoopBuilder";
        j["stagesNumber"] = stagesNumber;
        for (size_t i = 0; i < stagesNumber; ++i) {
            j["ratios"].push_back(*ratios[i]);
            j["stagesDurations"].push_back(stagesDurations[i]);
        }
        j["argsGeneratorBuilder"] = *argsGeneratorBuilder;
    }

    void from_json(const nlohmann::json& j) override {
        this->set_stages_number(j["stagesNumber"]);

        std::copy(std::begin(j["stagesDurations"]), std::end(j["stagesDurations"]),
                  stagesDurations);

        size_t i = 0;
        for (const auto& j_i : j["ratios"]) {
            ratios[i] = new RatioThreadLoopParameters(j_i);
            ++i;
        }

        argsGeneratorBuilder = get_args_generator_from_json(j["argsGeneratorBuilder"]);
    }

    std::string to_string(size_t indents) override {
        std::string result = indented_title_with_str_data("Type", "TEMPORARY_OPERATION", indents) +
                             indented_title_with_data("Stages number", stagesNumber, indents) +
                             indented_title("Stages Durations", indents);

        for (size_t i = 0; i < stagesNumber; ++i) {
            result += indented_title_with_data("Stage Duration " + std::to_string(i),
                                               stagesDurations[i], indents + 1);
        }

        result += indented_title("Ratios", indents);

        for (size_t i = 0; i < stagesNumber; ++i) {
            result += indented_title("Ratio " + std::to_string(i), indents + 1) +
                      ratios[i]->to_string(indents + 2);
        }

        result += indented_title("Args generator", indents) +
                  argsGeneratorBuilder->to_string(indents + 1);

        return result;
    }

    ~TemporaryOperationsThreadLoopBuilder() override {
        delete stagesDurations;
        delete[] ratios;
        delete argsGeneratorBuilder;
    };
};

}  // namespace microbench::workload
