#pragma once

#include "workloads/thread_loops/impls/prefill_insert_thread_loop.h"
#include "workloads/thread_loops/thread_loop_builder.h"
#include "workloads/args_generators/args_generator_builder.h"
#include "workloads/args_generators/args_generator_json_convector.h"
#include "globals_extern.h"

namespace microbench::workload {

struct PrefillInsertThreadLoopBuilder : public ThreadLoopBuilder {
    ArgsGeneratorBuilder* argsGeneratorBuilder = new DefaultArgsGeneratorBuilder();
    size_t numberOfAttempts = 10e+6;

    PrefillInsertThreadLoopBuilder* set_number_of_attempts(size_t number_of_attempts) {
        numberOfAttempts = number_of_attempts;
        return this;
    }

    PrefillInsertThreadLoopBuilder* set_args_generator_builder(
        ArgsGeneratorBuilder* args_generator_builder) {
        argsGeneratorBuilder = args_generator_builder;
        return this;
    }

    PrefillInsertThreadLoopBuilder* init(int range) override {
        ThreadLoopBuilder::init(range);
        argsGeneratorBuilder->init(range);
        return this;
    }

    //    template<typename K>
    ThreadLoop* build(globals_t* g, Random64& rng, size_t thread_id,
                      StopCondition* stop_condition) override {
        return new PrefillInsertThreadLoop(g, rng, thread_id, stop_condition, this->RQ_RANGE,
                                           argsGeneratorBuilder->build(rng), numberOfAttempts);
    }

    void to_json(nlohmann::json& json) const override {
        json["ClassName"] = "PrefillInsertThreadLoopBuilder";
        json["numberOfAttempts"] = numberOfAttempts;
        json["argsGeneratorBuilder"] = *argsGeneratorBuilder;
    }

    void from_json(const nlohmann::json& j) override {
        if (j.contains("numberOfAttempts")) {
            numberOfAttempts = j["numberOfAttempts"];
        }
        argsGeneratorBuilder = get_args_generator_from_json(j["argsGeneratorBuilder"]);
    }

    std::string to_string(size_t indents = 1) override {
        return indented_title_with_str_data("Type", "Prefill Insert", indents) +
               indented_title_with_data("Number of attempts", numberOfAttempts, indents) +
               indented_title("Args generator", indents) +
               argsGeneratorBuilder->to_string(indents + 1);
    }

    ~PrefillInsertThreadLoopBuilder() override {
        delete argsGeneratorBuilder;
    };
};

}  // namespace microbench::workload
