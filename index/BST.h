#ifndef BST_H
#define BST_H

// Plain (unbalanced) binary search tree.
// V must be a POINTER type (Incident*). The tree OWNS the pointed-to objects:
// remove() and the destructor delete them.
template <typename K, typename V>
class BST {
    struct Node {
        K key;
        V value;
        Node* left;
        Node* right;
        Node(const K& k, const V& v) : key(k), value(v), left(0), right(0) {}
    };

    Node* root;
    int count;

    BST(const BST&);              // no copying
    BST& operator=(const BST&);

    template <typename F>
    static void inorderRec(Node* n, F& f) {
        if (!n) return;
        inorderRec(n->left, f);
        f(n->key, n->value);
        inorderRec(n->right, f);
    }
    template <typename F>
    static void preorderRec(Node* n, F& f) {
        if (!n) return;
        f(n->key, n->value);
        preorderRec(n->left, f);
        preorderRec(n->right, f);
    }
    static void destroy(Node* n) {
        if (!n) return;
        destroy(n->left);
        destroy(n->right);
        delete n->value;     // the BST owns the object
        delete n;
    }

public:
    BST() : root(0), count(0) {}
    ~BST() { destroy(root); }

    // Iterative insert. Returns false if the key already exists.
    bool insert(const K& k, const V& v) {
        if (!root) { root = new Node(k, v); count++; return true; }
        Node* cur = root;
        while (true) {
            if (k < cur->key) {
                if (!cur->left) { cur->left = new Node(k, v); break; }
                cur = cur->left;
            } else if (cur->key < k) {
                if (!cur->right) { cur->right = new Node(k, v); break; }
                cur = cur->right;
            } else {
                return false;
            }
        }
        count++;
        return true;
    }

    V* find(const K& k) const {
        Node* cur = root;
        while (cur) {
            if (k < cur->key) cur = cur->left;
            else if (cur->key < k) cur = cur->right;
            else return &cur->value;
        }
        return 0;
    }

    // Deletes the node with key k AND the object it owns.
    // Cases: leaf, one child, two children (in-order successor).
    bool remove(const K& k) {
        Node* parent = 0;
        Node* cur = root;
        while (cur && (k < cur->key || cur->key < k)) {
            parent = cur;
            cur = (k < cur->key) ? cur->left : cur->right;
        }
        if (!cur) return false;

        if (cur->left && cur->right) {
            // Two children: successor = minimum of right subtree.
            Node* succParent = cur;
            Node* succ = cur->right;
            while (succ->left) { succParent = succ; succ = succ->left; }

            delete cur->value;            // free the incident being closed
            cur->key = succ->key;         // move successor's key and POINTER
            cur->value = succ->value;     // (the object itself is never copied)

            if (succParent == cur) succParent->right = succ->right;
            else                   succParent->left  = succ->right;
            delete succ;                  // delete only the node, not its object
        } else {
            // Leaf or one child: link the child (or null) to the parent.
            Node* child = cur->left ? cur->left : cur->right;
            if (!parent) root = child;
            else if (parent->left == cur) parent->left = child;
            else parent->right = child;
            delete cur->value;
            delete cur;
        }
        count--;
        return true;
    }

    int size() const { return count; }
    bool empty() const { return count == 0; }

    template <typename F> void inorder(F& f) const { inorderRec(root, f); }
    template <typename F> void preorder(F& f) const { preorderRec(root, f); }
};

#endif
