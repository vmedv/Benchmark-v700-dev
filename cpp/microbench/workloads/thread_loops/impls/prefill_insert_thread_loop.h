//
// Created by Ravil Galiev on 07.08.2023.
//

#ifndef SETBENCH_PREFILL_INSERT_THREAD_LOOP_H
#define SETBENCH_PREFILL_INSERT_THREAD_LOOP_H

#include <string>

#include "workloads/thread_loops/thread_loop.h"
#include "workloads/args_generators/args_generator.h"

//template<typename K>
class PrefillInsertThreadLoop : public ThreadLoop {
private:
    PAD;
    Random64 &rng;
    PAD;
    std::shared_ptr<ArgsGenerator<K>> argsGenerator;
    PAD;
    size_t number_of_attempts;

public:
    PrefillInsertThreadLoop(RT& ctx, Random64 &rng, size_t thread_id,
                            std::shared_ptr<StopCondition> stop_condition, size_t rq_range,
                            std::shared_ptr<ArgsGenerator<K>> args_generator, size_t number_of_attempts)
            : ThreadLoop(ctx, thread_id, stop_condition, rq_range),
              rng(rng), argsGenerator(args_generator), number_of_attempts(number_of_attempts) {
    }

    void Step() override {
        size_t counter = 0;
        K *value;
        do {
            K key = this->argsGenerator->nextInsert();
            value = this->ExecuteInsert(key);
            ++counter;
        } while (value != (K *) this->NO_VALUE && counter < number_of_attempts);

        if (value != (K *) this->NO_VALUE) {
            std::cerr << "WARNING: PrefillInsertThreadLoop with threadId=" << threadId
                      << " have not inserted a new key. Number of attempts is: "
                      << number_of_attempts << "\n";
        }
    }
};

#include "workloads/thread_loops/thread_loop_builder.h"
#include "workloads/args_generators/args_generator_builder.h"
#include "workloads/args_generators/impls/default_args_generator.h"
#include "workloads/args_generators/args_generator_json_convector.h"
#include "globals_extern.h"

//template<typename K>
struct PrefillInsertThreadLoopBuilder : public ThreadLoopBuilder {
    std::shared_ptr<ArgsGeneratorBuilder> argsGeneratorBuilder = std::make_shared<DefaultArgsGeneratorBuilder>();
    size_t numberOfAttempts = 10e+6;

    PrefillInsertThreadLoopBuilder *setNumberOfAttempts(size_t _numberOfAttempts) {
        numberOfAttempts = _numberOfAttempts;
        return this;
    }

    PrefillInsertThreadLoopBuilder *setArgsGeneratorBuilder(std::shared_ptr<ArgsGeneratorBuilder> _argsGeneratorBuilder) {
        argsGeneratorBuilder = _argsGeneratorBuilder;
        return this;
    }

    PrefillInsertThreadLoopBuilder *init(int range) override {
        ThreadLoopBuilder::init(range);
        argsGeneratorBuilder->init(range);
        return this;
    }

//    template<typename K>
    std::shared_ptr<ThreadLoop> build(ThreadLoop::RT& ctx, Random64 &rng, size_t thread_id, std::shared_ptr<StopCondition> stop_condition) override {
        return std::shared_ptr<PrefillInsertThreadLoop>(new PrefillInsertThreadLoop(ctx, rng, thread_id, stop_condition, this->RQ_RANGE,
                                           argsGeneratorBuilder->build(rng), numberOfAttempts));
    }

    void toJson(nlohmann::json &json) const override {
        json["ClassName"] = "PrefillInsertThreadLoopBuilder";
        json["numberOfAttempts"] = numberOfAttempts;
        json["argsGeneratorBuilder"] = *argsGeneratorBuilder;
    }

    void fromJson(const nlohmann::json &j) override {
        if (j.contains("numberOfAttempts")) {
            numberOfAttempts = j["numberOfAttempts"];
        }
        argsGeneratorBuilder = getArgsGeneratorFromJson(j["argsGeneratorBuilder"]);
    }

    std::string toString(size_t indents = 1) override {
        return indented_title_with_str_data("Type", "Prefill Insert", indents)
               + indented_title_with_data("Number of attempts", numberOfAttempts, indents)
               + indented_title("Args generator", indents)
               + argsGeneratorBuilder->toString(indents + 1);
    }

    ~PrefillInsertThreadLoopBuilder() override = default;
};

#endif //SETBENCH_PREFILL_INSERT_THREAD_LOOP_H
