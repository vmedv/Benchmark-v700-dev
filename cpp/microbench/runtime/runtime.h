#pragma once

#include <mutex>
#include <string_view>
#include <thread>

#if !defined(RT_STD) && !defined(RT_FIBERS)
#error "Unsupported runtime backend. Use RT_STD or RT_FIBERS."
#endif

#if defined(RT_FIBERS)
#include "fibers.h"
#else
#include "threads.h"
#endif

namespace microbench::runtime {

#if defined(RT_FIBERS)
using Backend = fibers;
inline constexpr std::string_view kRuntimeName = "boost.fibers";
inline constexpr bool kSupportsWorkerPinning = false;
inline constexpr bool kNeedsCooperativeYield = true;
inline constexpr int kYieldEveryOperations = 256;
#else
using Backend = threads;
inline constexpr std::string_view kRuntimeName = "std.threads";
inline constexpr bool kSupportsWorkerPinning = true;
inline constexpr bool kNeedsCooperativeYield = false;
inline constexpr int kYieldEveryOperations = 0;
#endif

using Environment = Backend;
using Worker = Backend::thread;

inline Environment create() {
    return Backend::create();
}

inline constexpr std::string_view name() {
    return kRuntimeName;
}

inline constexpr bool supports_worker_pinning() {
    return kSupportsWorkerPinning;
}

inline constexpr bool needs_cooperative_yield() {
    return kNeedsCooperativeYield;
}

inline constexpr int yield_every_operations() {
    return kYieldEveryOperations;
}

inline void yield() {
    Backend::yield{}();
}

inline bool yield_if_needed(int operation_count) {
    if constexpr (kNeedsCooperativeYield) {
        if ((operation_count % kYieldEveryOperations) == 0) {
            yield();
            return true;
        }
    }
    return false;
}

inline Worker spawn(Environment& env, Backend::Workload workload, Backend::WorkloadArg arg) {
    return env.spawn(workload, arg);
}

}  // namespace microbench::runtime
