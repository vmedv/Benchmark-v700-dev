/*
 * This file should replace globals_t as global provider of everything
 * since it's literally impossible to scale and reuse code
 *
 * Env, adapter, all necessry bindings must be provided for Run
 * see below for details
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <memory>
#include <string_view>
#include "plaf.h"
#include "random_xoshiro256p.h"
#include "server_clock.h"
#include "workloads/bench_parameters.h"
#include "workloads/parameters.h"

#include "threadlike/threading.h"

template <typename DataStructure, typename KeyType, typename ValueType, ValueType NoValue,
        #ifdef THREADLIKE
          threading T  // TODO: should be concept
        #else
          typename
        #endif
          >
struct Run {
    using PreciseTime = std::chrono::time_point<std::chrono::high_resolution_clock>;
    const ValueType no_value_ = NoValue;
    const KeyType key_min_;
    const KeyType key_max_;
    const int64_t prefill_interval_millis_;
    PAD;
    int64_t elapsed_millis_;
    int64_t cur_key_sum_ = 0;
    size_t cur_size_ = 0;
    PreciseTime execution_start_time_;
    PreciseTime execution_end_time_;
    PAD;
    std::unique_ptr<DataStructure> ds_;
    std::unique_ptr<BenchParameters> bench_params_;
    PAD;
    volatile test_type garbage_;  // used to prevent optimizing out some code
    PAD;
    Random64 rngs_[MAX_THREADS_POW2];
    volatile bool start_ = false;
    PAD;
    volatile bool done_ = false;
    PAD;
    volatile size_t running_ = 0;  // number of threads that are running
    PAD;

public:
    // initialize environment
    template <typename SeedType = time_t>
    explicit Run(std::unique_ptr<BenchParameters> bench_params, SeedType seed = time(nullptr))
        : key_min_(0),
          key_max_(bench_params->range + 1),
          prefill_interval_millis_(200),
          ds_(new DataStructure(bench_params->getMaxThreads(), key_min_, key_max_, no_value_,
                                rngs_)),
          bench_params_(std::move(bench_params)) {
        srand(seed);
        for (size_t i = 0; i < bench_params_->getMaxThreads(); ++i) {
            rngs_[i].setSeed(rand());
        }
        CreateDataStructure();
    }

    void RunExperiment() {
        PrefillStage();
        WarmUpStage();
        TestStage();
    }

    void PrefillStage() {
        if (bench_params_->prefill->getNumThreads() == 0) {
            return;
        }
        std::string stage = "PREFILL";
        PrintPrettySeparator(stage);
        Execute(bench_params_->prefill, stage);
        PrintPrettySeparator( stage + "_END");
    }

    void WarmUpStage() {
        if (bench_params_->warmUp->getNumThreads() == 0) {
            return;
        }
        std::string stage = "WARMUP";
        PrintPrettySeparator(stage);
        Execute(bench_params_->warmUp, stage);
        PrintPrettySeparator( stage + "_END");
    }

    void TestStage() {
        std::string stage = "TEST";
        PrintPrettySeparator(stage);
        Execute(bench_params_->test, stage);
        PrintPrettySeparator( stage + "_END");
    }

    void Execute(std::shared_ptr<Parameters> p, std::string stage) {
        std::vector<typename T::thread> threads;
        threads.reserve(p->getNumThreads());
        std::vector<std::shared_ptr<ThreadLoop>> thread_loops =
            p->getWorkload(*this, rngs_);
        binding_setCustom(p->getPin());
        BindThreads(p->getNumThreads());

        for (int i = 0; i < p->getNumThreads(); ++i) {
            threads.emplace_back(&ThreadLoop::run, thread_loops[i]);
        }

        while (running_ < p->getNumThreads()) {}
        SOFTWARE_BARRIER;
        execution_start_time_ = std::chrono::high_resolution_clock::now();
        SOFTWARE_BARRIER;
        printUptimeStampForPERF(stage + "_START");
        p->stopCondition->start(p->getNumThreads());
        start_ = true;
        SOFTWARE_BARRIER;


        for (size_t i = 0; i < p->getNumThreads(); ++i) {
            threads[i].join();
        }

        SOFTWARE_BARRIER;
        done_ = true;
        __sync_synchronize();
        execution_end_time_ = std::chrono::high_resolution_clock::now();
        __sync_synchronize();
        printUptimeStampForPERF(stage + "_END");

        if (ds_->validateStructure()) {
            std::cout << "Structural validation OK" << std::endl;
        } else {
            std::cout << "Structural validation FAILURE." << std::endl;
        }
        ds_->printSummary();
        elapsed_millis_ =
            std::chrono::duration_cast<std::chrono::milliseconds>(execution_end_time_ - execution_start_time_).count();

        p->stopCondition->clean();
        binding_deinit();

        start_ = false;
        done_ = false;

    }

private:
    static void PrintPrettySeparator(std::string_view message) {
        size_t blk = (80 - message.size() - 2) / 2;
        std::cout << std::string(blk, '#') << " " << message << (((message.size() % 2) != 0u) ? "  " : " ") << std::string(blk, '#')
                  << std::endl;
    }

    void CreateDataStructure() {
        ds_ = std::unique_ptr<DataStructure>(new DataStructure(
            bench_params_->getMaxThreads(), key_min_, key_max_, no_value_, rngs_));
    }

    void BindThreads(int nthreads) {
        binding_configurePolicy(nthreads);
        std::cout << "ACTUAL_THREAD_BINDINGS=";
        for (int i = 0; i < nthreads; ++i) {
            std::cout << ((i != 0) ? "," : "") << binding_getActualBinding(i);
        }
        std::cout << std::endl;
    }

};
