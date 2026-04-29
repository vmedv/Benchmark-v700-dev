#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <vector>
#include "plaf.h"
#include "random_xoshiro256p.h"
#include "workloads/distributions/distribution.h"

/*
 * BalancedPrefillDistribution
 *
 * Emits indices in [0, range) in an order that, when fed to an external
 * (leaf-oriented) BST whose leaves correspond to those indices in sorted
 * order, builds a perfectly balanced tree via single-key incremental
 * insertions.
 *
 * The order is the pre-order of the recursive construction
 *
 *     emit(lo, hi):
 *       emit hi
 *       fill(lo, hi - 1)
 *
 *     fill(lo, hi):
 *       if lo > hi: return
 *       m = (lo + hi) / 2
 *       emit m
 *       fill(lo, m - 1)
 *       fill(m + 1, hi)
 *
 * which guarantees that at the moment each new key is inserted it splits the
 * leaf that will eventually be its sibling in the perfect tree.
 *
 * After the precomputed sequence is exhausted (i.e. more than `range` calls
 * to next()), subsequent calls fall back to round-robin re-emission of the
 * sequence so the workload keeps making progress; this only matters for
 * misconfigured runs (commonOperationLimit > range).
 *
 * The sequence and the position counter are shared across all threads of the
 * prefill stage, so multi-threaded prefill stays correct (one shared atomic
 * cursor; insertions still serialize on the data structure as usual).
 */

namespace microbench::workload {

struct BalancedPrefillState {
    std::vector<size_t> sequence;
    PAD;
    std::atomic<size_t> position{0};
    PAD;
};

using BalancedPrefillStatePtr = std::shared_ptr<BalancedPrefillState>;

class BalancedPrefillDistribution : public Distribution {
private:
    PAD;
    BalancedPrefillStatePtr state_;
    PAD;

public:
    explicit BalancedPrefillDistribution(BalancedPrefillStatePtr state)
        : state_(std::move(state)) {
    }

    size_t next() override {
        const size_t n = state_->sequence.size();
        if (n == 0) {
            return 0;
        }
        const size_t i = state_->position.fetch_add(1, std::memory_order_relaxed);
        return state_->sequence[i % n];
    }

    ~BalancedPrefillDistribution() override = default;
};

inline BalancedPrefillStatePtr build_balanced_prefill_state(size_t range) {
    auto state = std::make_shared<BalancedPrefillState>();
    state->sequence.reserve(range);
    if (range == 0) {
        return state;
    }

    // Iterative emission of the recursive scheme described above, using an
    // explicit stack of [lo, hi] subranges to fill.  We emit (range - 1)
    // first, then split [0, range - 2] recursively by midpoint.
    state->sequence.push_back(range - 1);
    if (range == 1) {
        return state;
    }

    struct Frame {
        size_t lo;
        size_t hi;
    };  // inclusive
    std::vector<Frame> stack;
    stack.push_back({0, range - 2});

    while (!stack.empty()) {
        Frame f = stack.back();
        stack.pop_back();
        if (f.lo > f.hi) {
            continue;
        }
        const size_t m = f.lo + (f.hi - f.lo) / 2;
        state->sequence.push_back(m);
        // Push right first so left is processed first (preserves ordering).
        if (m + 1 <= f.hi) {
            stack.push_back({m + 1, f.hi});
        }
        if (f.lo + 1 <= m) {
            stack.push_back({f.lo, m - 1});
        }
    }

    return state;
}

}  // namespace microbench::workload
