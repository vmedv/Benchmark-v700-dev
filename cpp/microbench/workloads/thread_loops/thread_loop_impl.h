//
// Created by Ravil Galiev on 21.07.2023.
//

#ifndef SETBENCH_THREAD_LOOP_IMPL_H
#define SETBENCH_THREAD_LOOP_IMPL_H

#include "adapter.h"

template<typename K>
K *ThreadLoop::ExecuteInsert(K &key) {
    TRACE COUTATOMICTID("### calling INSERT " << key << std::endl);


    VALUE_TYPE value = ctx_.ds_->insertIfAbsent(threadId, key, KEY_TO_VALUE(key));
//    K *value = (K *) g->dsAdapter->insertIfAbsent(threadId, key, KEY_TO_VALUE(key));

    if (value == ctx_.ds_->getNoValue()) {
        // TRACE COUTATOMICTID("### completed INSERT modification for " << key << std::endl);
        GSTATS_ADD(threadId, key_checksum, key);
//             GSTATS_ADD(tid, size_checksum, 1);
        GSTATS_ADD(threadId, num_successful_inserts, 1);
    } else {
        // TRACE COUTATOMICTID("### completed READ-ONLY" << std::endl);
        GSTATS_ADD(threadId, num_fail_inserts, 1);
    }
    GSTATS_ADD(threadId, num_inserts, 1);
    GSTATS_ADD(threadId, num_operations, 1);

    return (K *) value;
}

template<typename K>
K *ThreadLoop::ExecuteRemove(const K &key) {
    TRACE COUTATOMICTID("### calling ERASE " << key << std::endl);
//    K *value = (K *) g->dsAdapter->erase(this->threadId, key);
    VALUE_TYPE value = ctx_.ds_->erase(this->threadId, key);

    if (value != this->ctx_.ds_->getNoValue()) {
        TRACE COUTATOMICTID("### completed ERASE modification for " << key << std::endl);
        GSTATS_ADD(threadId, key_checksum, -key);
//             GSTATS_ADD(tid, size_checksum, -1);
        GSTATS_ADD(threadId, num_successful_removes, 1);
    } else {
        TRACE COUTATOMICTID("### completed READ-ONLY" << std::endl);
        GSTATS_ADD(threadId, num_fail_removes, 1);
    }
    GSTATS_ADD(threadId, num_removes, 1);
    GSTATS_ADD(threadId, num_operations, 1);

    return (K *) value;
}

template<typename K>
K *ThreadLoop::ExecuteGet(const K &key) {
//    K *value = (K *) this->g->dsAdapter->find(this->threadId, key);
    VALUE_TYPE value = this->ctx_.ds_->find(this->threadId, key);

    if (value != this->ctx_.ds_->getNoValue()) {
        garbage += key; // prevent optimizing out
        GSTATS_ADD(threadId, num_successful_searches, 1);
    } else {
        GSTATS_ADD(threadId, num_fail_searches, 1);
    }
    GSTATS_ADD(threadId, num_searches, 1);
    GSTATS_ADD(threadId, num_operations, 1);

    return (K *) value;
}

template<typename K>
bool ThreadLoop::ExecuteContains(const K &key) {
    bool value = this->ctx_.ds_->contains(this->threadId, key);

    if (value) {
        garbage += key; // prevent optimizing out
        // GSTATS_ADD(threadId, num_successful_searches, 1);
    } else {
        // GSTATS_ADD(threadId, num_fail_searches, 1);
    }
    // GSTATS_ADD(threadId, num_searches, 1);
    // GSTATS_ADD(threadId, num_operations, 1);

    return value;
}

/**
 * the result is in the arrays rqResultKeys and rqResultValues
 */
template<typename K>
void ThreadLoop::ExecuteRangeQuery(const K &leftKey, const K &rightKey) {
    // ++rq_cnt;
    // size_t rqcnt;
    // if ((rqcnt = this->g->dsAdapter->rangeQuery(this->threadId, leftKey, rightKey,
    //                                             rqResultKeys, (VALUE_TYPE*) rqResultValues))) {
    //     garbage += rqResultKeys[0] +
    //                rqResultKeys[rqcnt - 1]; // prevent rqResultValues and count from being optimized out
    // }
    // GSTATS_ADD(threadId, num_rq, 1);
    // GSTATS_ADD(threadId, num_operations, 1);
}

void ThreadLoop::run() {
    auto tid = this->threadId;
    binding_bindThread(tid);
    garbage = 0;
    this->ctx_.ds_->initThread(threadId);
    __sync_fetch_and_add(&this->ctx_.running_, 1);
    __sync_synchronize();
    while (!this->ctx_.start_) { SOFTWARE_BARRIER; }

    while (!stopCondition->isStopped(threadId)) {
        // ++cnt;
        // VERBOSE if (cnt && ((cnt % 1000000) == 0)) COUTATOMICTID("op# " << cnt << std::endl);
        Step();
    }

    __sync_fetch_and_add(&this->ctx_.running_, -1);
    SOFTWARE_BARRIER;
    while (this->ctx_.running_) { SOFTWARE_BARRIER; }
    this->ctx_.ds_->deinitThread(tid); \
    this->ctx_.garbage_ += garbage;
}


#endif //SETBENCH_THREAD_LOOP_IMPL_H
