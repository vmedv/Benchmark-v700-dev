//
// Created by Ravil Galiev on 07.08.2023.
//
#pragma once

#include <iostream>

#include "random_xoshiro256p.h"
#include "workloads/thread_loops/thread_loop.h"
#include "workloads/args_generators/args_generator.h"

namespace microbench::workload {

class PrefillInsertThreadLoop : public ThreadLoop {
private:
    PAD;
    Random64& rng_;
    PAD;
    ArgsGenerator<K>* args_generator_;
    PAD;
    size_t number_of_attempts_;

public:
    PrefillInsertThreadLoop(globals_t* g, Random64& rng, size_t thread_id,
                            StopCondition* stop_condition, size_t rq_range,
                            ArgsGenerator<K>* args_generator, size_t number_of_attempts)
        : ThreadLoop(g, thread_id, stop_condition, rq_range),
          rng_(rng),
          args_generator_(args_generator),
          number_of_attempts_(number_of_attempts) {
    }

    void step() override {
        size_t counter = 0;
        K* value;
        do {
            K key = this->args_generator_->next_insert();
            value = this->execute_insert(key);
            ++counter;
        } while (value != (K*)this->NO_VALUE && counter < number_of_attempts_);

        if (value != (K*)this->NO_VALUE) {
            std::cerr << "WARNING: PrefillInsertThreadLoop with threadId=" << threadId
                      << " have not inserted a new key. Number of attempts is: "
                      << number_of_attempts_ << "\n";
        }
    }
};

}  // namespace microbench::workload
