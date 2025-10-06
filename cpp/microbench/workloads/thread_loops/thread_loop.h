//
// Created by Ravil Galiev on 21.07.2023.
//

#ifndef SETBENCH_THREAD_LOOP_H
#define SETBENCH_THREAD_LOOP_H

#include "adapter.h"
#include "fibers.h"
#include "threading.h"
#include "threads.h"
#include "workloads/stop_condition/stop_condition.h"
#include "globals_t.h"

//#define VALUE_TYPE void *

typedef long long K;

template <typename, typename, typename VT, VT NoValue, threading>
struct Run;

class ThreadLoop {
protected:
    K garbage = 0;
    K * rqResultKeys;
    VALUE_TYPE * rqResultValues;
    VALUE_TYPE NO_VALUE;
    int rq_cnt;
    size_t RQ_RANGE;
public:
    size_t threadId;
    // std::shared_ptr<globals_t> g;
    using RT = Run<ds_adapter<test_type, VALUE_TYPE>, test_type, VALUE_TYPE, nullptr, fibers>;
    RT& ctx_;
    std::shared_ptr<StopCondition> stopCondition;

    ThreadLoop(RT& ctx, size_t thread_id, std::shared_ptr<StopCondition> stop_condition, size_t rq_range)
            : ctx_(ctx), threadId(thread_id), stopCondition(stop_condition), RQ_RANGE(rq_range) {}

    template<typename K>
    K * ExecuteInsert(K & key);

    template<typename K>
    K * ExecuteRemove(const K & key);

    template<typename K>
    K * ExecuteGet(const K & key);

    template<typename K>
    bool ExecuteContains(const K & key);

    /**
     * the result is in the arrays rqResultKeys and rqResultValues
     */
    template<typename K>
    void ExecuteRangeQuery(const K & left_key, const K & right_key);

    virtual void run();

    virtual void Step() = 0;
};

#ifndef MAIN_BENCH

template<typename K>
void ThreadLoop::ExecuteRangeQuery(const K &leftKey, const K &rightKey) {

}

template<typename K>
bool ThreadLoop::ExecuteContains(const K &key) {
    return false;
}

template<typename K>
K *ThreadLoop::ExecuteGet(const K &key) {
    return nullptr;
}

template<typename K>
K *ThreadLoop::ExecuteRemove(const K &key) {
    return nullptr;
}

template<typename K>
K *ThreadLoop::ExecuteInsert(K &key) {
    return nullptr;
}

void ThreadLoop::run() {

}

#endif

#endif //SETBENCH_THREAD_LOOP_H
