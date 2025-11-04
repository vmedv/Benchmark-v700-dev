//
// Created by Ravil Galiev on 06.04.2023.
//
#pragma once

#include "plaf.h"
#include "random_xoshiro256p.h"
#include "workloads/thread_loops/thread_loop.h"
#include "workloads/args_generators/args_generator.h"
#include "workloads/thread_loops/ratio_thread_loop_parameters.h"

namespace microbench::workload {

class DefaultThreadLoop : public ThreadLoop {
    PAD;
    double* cdf_;
    Random64& rng_;
    PAD;
    ArgsGenerator<K>* args_generator_;
    PAD;

public:
    DefaultThreadLoop(globals_t* g, Random64& rng, size_t thread_id, StopCondition* stop_condition,
                      size_t rq_range, ArgsGenerator<K>* args_generator,
                      RatioThreadLoopParameters& thread_loop_parameters)
        : ThreadLoop(g, thread_id, stop_condition, rq_range),
          rng_(rng),
          args_generator_(args_generator) {
        cdf_ = new double[3];
        cdf_[0] = thread_loop_parameters.INS_RATIO;
        cdf_[1] = cdf_[0] + thread_loop_parameters.REM_RATIO;
        cdf_[2] = cdf_[1] + thread_loop_parameters.RQ_RATIO;
    }

    void step() override {
        double op = (double)rng_.next() / (double)rng_.max_value;
        if (op < cdf_[0]) {  // insert
            K key = this->args_generator_->next_insert();
            this->execute_insert(key);
        } else if (op < cdf_[1]) {  // remove
            K key = this->args_generator_->next_remove();
            this->execute_remove(key);
        } else if (op < cdf_[2]) {  // range query
            std::pair<K, K> keys = this->args_generator_->next_range();
            this->execute_range_query(keys.first, keys.second);
        } else {  // read
            K key = this->args_generator_->next_get();
            this->GET_FUNC(key);
        }
    }
};

}  // namespace microbench::workload
