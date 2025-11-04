//
// Created by Ravil Galiev on 16.08.2023.
//
#pragma once

#include "random_xoshiro256p.h"
#include "workloads/args_generators/args_generator.h"
#include "workloads/args_generators/args_generator_builder.h"
#include "workloads/thread_loops/thread_loop.h"
#include "workloads/thread_loops/ratio_thread_loop_parameters.h"

namespace microbench::workload {

class TemporaryOperationThreadLoop : public ThreadLoop {
    PAD;
    double** cdf_;
    Random64& rng_;
    PAD;
    ArgsGenerator<K>* args_generator_;
    PAD;
    size_t time_;
    size_t pointer_;
    size_t stages_number_;
    size_t* stages_durations_;
    PAD;

    void update_pointer() {
        if (time_ >= stages_durations_[pointer_]) {
            time_ = 0;
            ++pointer_;
            if (pointer_ >= stages_number_) {
                pointer_ = 0;
            }
        }
        ++time_;
    }

public:
    TemporaryOperationThreadLoop(globals_t* g, Random64& rng, size_t thread_id,
                                 StopCondition* stop_condition, size_t rq_range,
                                 size_t stages_number, size_t* stages_durations,
                                 RatioThreadLoopParameters** ratios,
                                 ArgsGenerator<K>* args_generator)
        : ThreadLoop(g, thread_id, stop_condition, rq_range),
          rng_(rng),
          args_generator_(args_generator),
          stages_number_(stages_number),
          time_(0),
          pointer_(0) {
        cdf_ = new double*[stages_number];
        stages_durations_ = new size_t[stages_number];
        std::copy(stages_durations, stages_durations + stages_number, stages_durations_);

        for (size_t i = 0; i < stages_number; ++i) {
            cdf_[i] = new double[3];
            cdf_[i][0] = ratios[i]->INS_RATIO;
            cdf_[i][1] = cdf_[i][0] + ratios[i]->REM_RATIO;
            cdf_[i][2] = cdf_[i][1] + ratios[i]->RQ_RATIO;
        }
    }

    void step() override {
        update_pointer();

        double op = (double)rng_.next() / (double)rng_.max_value;
        if (op < cdf_[pointer_][0]) {  // insert
            K key = this->args_generator_->next_insert();
            this->execute_insert(key);
        } else if (op < cdf_[pointer_][1]) {  // remove
            K key = this->args_generator_->next_remove();
            this->execute_remove(key);
        } else if (op < cdf_[pointer_][2]) {  // range query
            std::pair<K, K> keys = this->args_generator_->next_range();
            this->execute_range_query(keys.first, keys.second);
        } else {  // read
            K key = this->args_generator_->next_get();
            this->GET_FUNC(key);
        }
    }
};

}  // namespace microbench::workload
