/**
 * Implementation of the internal lock-free BST of Ramachandran and Mittal.
 * This is a heavily modified version of the original authors' implementation.
 * (See copyright notice in intlf.h)
 * The modifications are copyrighted (consistent with the original license)
 *   by Maya Arbel-Raviv and Trevor Brown, 2018.
 */

#ifndef BST_ADAPTER_H
#define BST_ADAPTER_H

#include <functional>
#include "allocator_new.h"
#include "ds/payload_tree/payload_tree.h"
#include "errors.h"
#include "pool_none.h"
#include "reclaimer_debra.h"

template <typename K, typename V, class Reclaim = reclaimer_debra<K>,
          class Alloc = allocator_new<K>, class Pool = pool_none<K>>
class ds_adapter {
private:
    const V NO_VALUE;
    using DS = spc::Tree<K, std::identity>;
    DS* ds;

public:
    ds_adapter(const int NUM_THREADS, const K& KEY_MIN, const K& KEY_MAX, const V& VALUE_RESERVED,
               Random64* const unused2)
        : NO_VALUE(VALUE_RESERVED),
          ds(new DS()) {
    }
    ~ds_adapter() {
        delete ds;
    }

    V getNoValue() {
        return NO_VALUE;
    }

    void initThread(const int tid) {
    }
    void deinitThread(const int tid) {
    }

    V insert(const int tid, const K& key, const V& val) {
        setbench_error("not implemented");
    }
    V insertIfAbsent(const int tid, const K& key, const V& val) {
        ds->insert(key);
        return nullptr;
    }
    V erase(const int tid, const K& key) {
        setbench_error("not implemented");
    }
    V find(const int tid, const K& key) {
        return ds->find(key);
    }
    bool contains(const int tid, const K& key) {
        return find(tid, key) != getNoValue();
    }
    int getHeight() {
        return ds->getHeight();
    }
    int rangeQuery(const int tid, const K& lo, const K& hi, K* const resultKeys,
                   V* const resultValues) {
        setbench_error("not implemented");
    }
    void printSummary() {
    }
    bool validateStructure() {
        return true;
    }
    void printObjectSizes() {
        std::cout << "sizes: tree=" << (sizeof(DS)) << "node=" << (sizeof(typename DS::Node)) << std::endl;
    }
    // try to clean up: must only be called by a single thread as part of the test harness!
    void debugGCSingleThreaded() {
    }
};


#endif
