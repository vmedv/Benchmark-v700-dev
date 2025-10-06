//
// Created by Ravil Galiev on 06.04.2023.
//

#ifndef SETBENCH_DEFAULT_THREAD_LOOP_H
#define SETBENCH_DEFAULT_THREAD_LOOP_H

#include <memory>
#include "workloads/thread_loops/thread_loop.h"
#include "workloads/args_generators/args_generator.h"
#include "workloads/thread_loops/ratio_thread_loop_parameters.h"

//template<typename K>
class DefaultThreadLoop : public ThreadLoop {
    PAD;
    std::vector<double> cdf;
    Random64 &rng;
    PAD;
    std::shared_ptr<ArgsGenerator<K>> argsGenerator;
    PAD;

public:
    DefaultThreadLoop(RT& ctx, Random64 &rng, size_t thread_id, std::shared_ptr<StopCondition> stop_condition, size_t rq_range,
                      std::shared_ptr<ArgsGenerator<K>> args_generator,
                      RatioThreadLoopParameters &thread_loop_parameters)
            : ThreadLoop(ctx, thread_id, stop_condition, rq_range),
              rng(rng), argsGenerator(args_generator) {
        cdf.resize(3);
        cdf[0] = thread_loop_parameters.INS_RATIO;
        cdf[1] = cdf[0] + thread_loop_parameters.REM_RATIO;
        cdf[2] = cdf[1] + thread_loop_parameters.RQ_RATIO;
    }

    void Step() override {
        double op = (double) rng.next() / (double) rng.max_value;
        if (op < cdf[0]) { // insert
            K key = this->argsGenerator->nextInsert();
            this->ExecuteInsert(key);
        } else if (op < cdf[1]) { // remove
            K key = this->argsGenerator->nextRemove();
            this->ExecuteRemove(key);
        } else if (op < cdf[2]) { // range query
            std::pair<K, K> keys = this->argsGenerator->nextRange();
            this->ExecuteRangeQuery(keys.first, keys.second);
        } else { // read
            K key = this->argsGenerator->nextGet();
            this->GET_FUNC(key);
        }
    }
};

#include "workloads/thread_loops/thread_loop_builder.h"
#include "workloads/args_generators/args_generator_builder.h"
#include "workloads/args_generators/impls/default_args_generator.h"
#include "workloads/args_generators/args_generator_json_convector.h"
#include "globals_extern.h"

//template<typename K>
struct DefaultThreadLoopBuilder : public ThreadLoopBuilder {
    RatioThreadLoopParameters parameters;

    std::shared_ptr<ArgsGeneratorBuilder> argsGeneratorBuilder = std::shared_ptr<DefaultArgsGeneratorBuilder>(new DefaultArgsGeneratorBuilder());

    DefaultThreadLoopBuilder *setInsRatio(double insRatio) {
        parameters.INS_RATIO = insRatio;
        return this;
    }

    DefaultThreadLoopBuilder *setRemRatio(double delRatio) {
        parameters.REM_RATIO = delRatio;
        return this;
    }

    DefaultThreadLoopBuilder *setRqRatio(double rqRatio) {
        parameters.RQ_RATIO = rqRatio;
        return this;
    }

    DefaultThreadLoopBuilder *setArgsGeneratorBuilder(std::shared_ptr<ArgsGeneratorBuilder> _argsGeneratorBuilder) {
        argsGeneratorBuilder = _argsGeneratorBuilder;
        return this;
    }

    DefaultThreadLoopBuilder *init(int range) override {
        ThreadLoopBuilder::init(range);
        argsGeneratorBuilder->init(range);
        return this;
    }

//    template<typename K>
    std::shared_ptr<ThreadLoop> build(ThreadLoop::RT& ctx, Random64 &rng, size_t thread_id, std::shared_ptr<StopCondition> stop_condition) override {
        return std::shared_ptr<ThreadLoop>(new DefaultThreadLoop(ctx, rng, thread_id, stop_condition, this->RQ_RANGE,
                                     argsGeneratorBuilder->build(rng),
                                     parameters));
    }

    void toJson(nlohmann::json &json) const override {
        json["ClassName"] = "DefaultThreadLoopBuilder";
        json["parameters"] = parameters;
        json["argsGeneratorBuilder"] = *argsGeneratorBuilder;
    }


    void fromJson(const nlohmann::json &j) override {
        parameters = j["parameters"];
        argsGeneratorBuilder = getArgsGeneratorFromJson(j["argsGeneratorBuilder"]);
    }

    std::string toString(size_t indents = 1) override {
        return indented_title_with_str_data("Type", "Default", indents)
               + indented_title_with_data("INS_RATIO", parameters.INS_RATIO, indents)
               + indented_title_with_data("REM_RATIO", parameters.REM_RATIO, indents)
               + indented_title_with_data("RQ_RATIO", parameters.RQ_RATIO, indents)
               + indented_title("Args generator", indents)
               + argsGeneratorBuilder->toString(indents + 1);
    }

    ~DefaultThreadLoopBuilder() override = default;
};


#endif //SETBENCH_DEFAULT_THREAD_LOOP_H
