#pragma once

#include <memory>
#include <vector>

#include "data_maps/data_map.h"
#include "errors.h"
#include "globals_extern.h"
#include "workloads/data_maps/data_map_builder.h"
#include "workloads/data_maps/impls/array_data_map.h"

namespace microbench::workload {

class BiasedPrefixDataMapBuilder : public DataMapBuilder {
    double count_ = 0;
    double bias_ = 0;
    std::vector<int64_t> data_;

public:
    BiasedPrefixDataMapBuilder& set_count(double count) {
        count_ = count;
        return *this;
    }

    BiasedPrefixDataMapBuilder& set_bias(double bias) {
        bias_ = bias;
        return *this;
    }

    BiasedPrefixDataMapBuilder& init(size_t range) override {
        if (count_ < 0 || count_ > 1) {
            setbench_error("BiasedPrefixDataMapBuilder: count must be in [0, 1]");
        }
        if (bias_ < 0 || bias_ > 1) {
            setbench_error("BiasedPrefixDataMapBuilder: bias must be in [0, 1]");
        }

        data_.clear();
        data_.reserve(range);

        const size_t prefix_count = static_cast<size_t>(range * count_);
        const size_t start_count = static_cast<size_t>(prefix_count * (1.0 - bias_));
        const size_t end_count = prefix_count - start_count;

        for (int64_t key = 1; key <= static_cast<int64_t>(start_count); ++key) {
            data_.push_back(key);
        }

        for (int64_t key = static_cast<int64_t>(range - end_count + 1);
             key <= static_cast<int64_t>(range); ++key) {
            data_.push_back(key);
        }

        for (int64_t key = static_cast<int64_t>(start_count + 1);
             key <= static_cast<int64_t>(range - end_count); ++key) {
            data_.push_back(key);
        }

        return *this;
    }

    DataMapPtr build() override {
        return std::make_unique<ArrayDataMap>(data_);
    }

    void to_json(nlohmann::json& j) const override {
        j["ClassName"] = "BiasedPrefixDataMapBuilder";
        j["count"] = count_;
        j["bias"] = bias_;
    }

    void from_json(const nlohmann::json& j) override {
        count_ = j["count"];
        bias_ = j["bias"];
    }

    std::string to_string(size_t indents = 1) override {
        return indented_title_with_str_data("Type", "BiasedPrefixDataMap", indents) +
               indented_title_with_data("Count", count_, indents) +
               indented_title_with_data("Bias", bias_, indents) +
               indented_title_with_data("ID", id, indents);
    }

    ~BiasedPrefixDataMapBuilder() override = default;
};

}  // namespace microbench::workload
