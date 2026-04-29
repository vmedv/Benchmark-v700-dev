#pragma once

#include <cassert>
#include <memory>
#include "globals_extern.h"
#include "random_xoshiro256p.h"
#include "workloads/distributions/distribution_builder.h"
#include "workloads/distributions/impls/balanced_prefill_distribution.h"

namespace microbench::workload {

/*
 * Builder for BalancedPrefillDistribution.  The shared sequence + cursor live
 * in the builder so that every per-thread distribution returned by build()
 * sees the same monotonically advancing position.
 *
 * Note: only the fixed-range build(rng, range) form is supported; the
 * mutable variant is intentionally absent because the sequence depends on
 * the range and cannot be reseated cheaply.
 */
struct BalancedPrefillDistributionBuilder : public DistributionBuilder {
    PAD;
    BalancedPrefillStatePtr state_;
    size_t state_range_{0};
    PAD;

    DistributionPtr build(Random64& /*rng*/, size_t range) override {
        if (!state_ || state_range_ != range) {
            state_ = build_balanced_prefill_state(range);
            state_range_ = range;
        }
        return std::make_unique<BalancedPrefillDistribution>(state_);
    }

    void to_json(nlohmann::json& j) const override {
        j["ClassName"] = "BalancedPrefillDistributionBuilder";
    }

    void from_json(const nlohmann::json& /*j*/) override {
    }

    std::string to_string(size_t indents = 1) override {
        return indented_title_with_str_data("Type", "BalancedPrefill", indents);
    }

    ~BalancedPrefillDistributionBuilder() override = default;
};

}  // namespace microbench::workload
