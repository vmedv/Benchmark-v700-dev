#pragma once

#include <algorithm>
#include <atomic>
#include <concepts>
#include <vector>

/*
 * This is unbalanced tree: it should not be compared to other data structures:
 * however, it can (and actually should) be compared to itself:
 * It has HookFn template arg, which will do some work with Node::payload
 * e.g. it make some fetch_and_add just to make cache line invalidate
 */

namespace spc {

template <typename Key, typename HookFn>
    requires std::invocable<HookFn, std::atomic_int64_t&>
struct Tree {
public:
    struct Node {
        Node* left;
        Node* right;
        bool is_leaf;
        bool has_value;
        Key key;
        volatile std::atomic_int64_t payload_;
    };

private:
    Node* root_{nullptr};
    std::vector<Key> keys_;
    HookFn hook_{};

public:
    Tree() {
    }

    ~Tree() {
        destroy(root_);
    }

    Tree(const Tree&) = delete;
    Tree& operator=(const Tree&) = delete;
    Tree(Tree&&) = delete;
    Tree& operator=(Tree&&) = delete;

    void insert(Key key) {
        keys_.push_back(key);
        rebuild_external_tree();
    }

    Node* find(Key key) {
        Node* node = root_;
        while (node && !node->is_leaf) {
            hook_(node->payload_);
            if (key <= node->key) {
                node = node->left;
            } else {
                node = node->right;
            }
        }
        if ((node == nullptr) || !node->has_value) {
            return nullptr;
        }
        return node->key == key ? node : nullptr;
    }

private:
    struct BuildResult {
        Node* node;
        bool has_any;
        Key max_key;
    };

    static void destroy(Node* node) {
        if (node == nullptr) {
            return;
        }
        destroy(node->left);
        destroy(node->right);
        delete node;
    }

    void rebuild_external_tree() {
        destroy(root_);
        root_ = nullptr;

        std::sort(keys_.begin(), keys_.end());
        if (keys_.empty()) {
            return;
        }

        const size_t leaf_count = next_power_of_two(keys_.size());
        const size_t height = log2_pow2(leaf_count);
        size_t index = 0;
        root_ = build_full(height, 0, index).node;
    }

    static size_t next_power_of_two(size_t value) {
        size_t pow2 = 1;
        while (pow2 < value) {
            pow2 <<= 1;
        }
        return pow2;
    }

    static size_t log2_pow2(size_t pow2) {
        size_t height = 0;
        while ((static_cast<size_t>(1) << height) < pow2) {
            ++height;
        }
        return height;
    }

    BuildResult build_full(size_t height, size_t depth, size_t& index) {
        if (depth == height) {
            Node* leaf = new Node{nullptr, nullptr, true, false, Key{}, 0};
            if (index < keys_.size()) {
                leaf->key = keys_[index++];
                leaf->has_value = true;
                return {leaf, true, leaf->key};
            }
            return {leaf, false, Key{}};
        }

        BuildResult left = build_full(height, depth + 1, index);
        BuildResult right = build_full(height, depth + 1, index);
        Node* node = new Node{left.node, right.node, false, false, Key{}, 0};
        const bool has_any = left.has_any || right.has_any;
        if (left.has_any) {
            node->key = left.max_key;
        } else if (right.has_any) {
            node->key = right.max_key;
        } else {
            node->key = Key{};
        }
        const Key max_key =
            right.has_any ? right.max_key : (left.has_any ? left.max_key : Key{});
        return {node, has_any, max_key};
    }
};
}  // namespace spc
