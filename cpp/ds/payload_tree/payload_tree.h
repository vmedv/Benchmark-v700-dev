#pragma once

#include <atomic>
#include <concepts>

/*
 * Unbalanced external (leaf-oriented) BST.
 *
 * This is intentionally unbalanced: it should not be compared to other data
 * structures, only to itself. Insertions are O(tree height), not O(N log N):
 * we descend to a leaf and locally split it instead of rebuilding the tree.
 *
 * Keys live only in leaves (Node::has_value && Node::is_leaf).
 * Internal nodes hold a split key equal to the maximum key in their left
 * subtree: keys <= node->key go left, keys > node->key go right.
 *
 * HookFn is invoked on Node::payload_ of every internal node visited during
 * find() and insert() (e.g. an atomic fetch_and_add to invalidate cache
 * lines), which is the whole point of the structure.
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
        std::atomic_int64_t payload_;
    };

private:
    Node* root_{nullptr};
    HookFn hook_{};

public:
    Tree() = default;

    ~Tree() {
        destroy(root_);
    }

    Tree(const Tree&) = delete;
    Tree& operator=(const Tree&) = delete;
    Tree(Tree&&) = delete;
    Tree& operator=(Tree&&) = delete;

    void insert(Key key) {
        if (root_ == nullptr) {
            root_ = make_leaf(key, /*has_value=*/true);
            return;
        }

        // Descend to a leaf, remembering the parent and which side we took.
        Node* parent = nullptr;
        Node* node = root_;
        bool from_left = false;
        while (!node->is_leaf) {
            hook_(node->payload_);
            parent = node;
            if (key <= node->key) {
                node = node->left;
                from_left = true;
            } else {
                node = node->right;
                from_left = false;
            }
        }

        // Empty leaf: just place the key.
        if (!node->has_value) {
            node->key = key;
            node->has_value = true;
            return;
        }

        // Duplicate: no-op (set semantics, matches insertIfAbsent).
        if (node->key == key) {
            return;
        }

        // Split the leaf into an internal node with two leaf children.
        const Key old_key = node->key;
        const Key smaller = key < old_key ? key : old_key;
        const Key larger = key < old_key ? old_key : key;

        Node* left = make_leaf(smaller, /*has_value=*/true);
        Node* right = make_leaf(larger, /*has_value=*/true);
        Node* internal = new Node{left, right, /*is_leaf=*/false,
                                  /*has_value=*/false, smaller, 0};

        // Reuse `node`'s slot in the parent (or as the new root) by replacing
        // the pointer; then free the old leaf.
        if (parent == nullptr) {
            root_ = internal;
        } else if (from_left) {
            parent->left = internal;
        } else {
            parent->right = internal;
        }
        delete node;
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
    static Node* make_leaf(Key key, bool has_value) {
        return new Node{nullptr, nullptr, /*is_leaf=*/true, has_value, key, 0};
    }

    static void destroy(Node* node) {
        if (node == nullptr) {
            return;
        }
        destroy(node->left);
        destroy(node->right);
        delete node;
    }
};
}  // namespace spc
