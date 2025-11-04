#pragma once

#include "workloads/thread_loops/impls/default_thread_loop.h"
#include "workloads/thread_loops/ratio_thread_loop_parameters.h"
#include "workloads/thread_loops/thread_loop_builder.h"
#include "workloads/args_generators/args_generator_builder.h"
#include "workloads/args_generators/args_generator_json_convector.h"
#include "globals_extern.h"

namespace microbench::workload {

struct DefaultThreadLoopBuilder : public ThreadLoopBuilder {
    RatioThreadLoopParameters parameters;

    ArgsGeneratorBuilder* argsGeneratorBuilder = new DefaultArgsGeneratorBuilder();

    DefaultThreadLoopBuilder* set_ins_ratio(double ins_ratio) {
        parameters.INS_RATIO = ins_ratio;
        return this;
    }

    DefaultThreadLoopBuilder* set_rem_ratio(double del_ratio) {
        parameters.REM_RATIO = del_ratio;
        return this;
    }

    DefaultThreadLoopBuilder* set_rq_ratio(double rq_ratio) {
        parameters.RQ_RATIO = rq_ratio;
        return this;
    }

    DefaultThreadLoopBuilder* set_args_generator_builder(
        ArgsGeneratorBuilder* args_generator_builder) {
        argsGeneratorBuilder = args_generator_builder;
        return this;
    }

    DefaultThreadLoopBuilder* init(int range) override {
        ThreadLoopBuilder::init(range);
        argsGeneratorBuilder->init(range);
        return this;
    }

    //    template<typename K>
    ThreadLoop* build(globals_t* g, Random64& rng, size_t thread_id,
                      StopCondition* stop_condition) override {
        return new DefaultThreadLoop(g, rng, thread_id, stop_condition, this->RQ_RANGE,
                                     argsGeneratorBuilder->build(rng), parameters);
    }

    void to_json(nlohmann::json& json) const override {
        json["ClassName"] = "DefaultThreadLoopBuilder";
        json["parameters"] = parameters;
        json["argsGeneratorBuilder"] = *argsGeneratorBuilder;
    }

    void from_json(const nlohmann::json& j) override {
        parameters = j["parameters"];
        argsGeneratorBuilder = get_args_generator_from_json(j["argsGeneratorBuilder"]);
    }

    std::string to_string(size_t indents = 1) override {
        return indented_title_with_str_data("Type", "Default", indents) +
               indented_title_with_data("INS_RATIO", parameters.INS_RATIO, indents) +
               indented_title_with_data("REM_RATIO", parameters.REM_RATIO, indents) +
               indented_title_with_data("RQ_RATIO", parameters.RQ_RATIO, indents) +
               indented_title("Args generator", indents) +
               argsGeneratorBuilder->to_string(indents + 1);
    }

    ~DefaultThreadLoopBuilder() override {
        delete argsGeneratorBuilder;
    };
};

}  // namespace microbench::workload
