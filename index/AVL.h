#ifndef AVL_H
#define AVL_H

// AVL tree: self-balancing BST. Height(empty)=0, height(leaf)=1.
// Balance factor = height(left) - height(right).
template <typename K, typename V>
class AVL {
    struct Node {
        K key;
        V value;
        Node* left;
        Node* right;
        int height;
        Node(const K& k, const V& v) : key(k), value(v), left(0), right(0), height(1) {}
    };

    Node* root;
    int count;

    AVL(const AVL&);              // no copying
    AVL& operator=(const AVL&);

    static int height(Node* n) { return n ? n->height : 0; }
    static int balance(Node* n) { return n ? height(n->left) - height(n->right) : 0; }
    static void update(Node* n) {
        int l = height(n->left), r = height(n->right);
        n->height = 1 + (l > r ? l : r);
    }

    // Right rotation (fixes LL case).
    static Node* rotateRight(Node* y) {
        Node* x = y->left;
        y->left = x->right;
        x->right = y;
        update(y);
        update(x);
        return x;
    }
    // Left rotation (fixes RR case).
    static Node* rotateLeft(Node* x) {
        Node* y = x->right;
        x->right = y->left;
        y->left = x;
        update(x);
        update(y);
        return y;
    }

    // Inserts below n, updates heights while returning up the path, rebalances.
    Node* insertRec(Node* n, const K& k, const V& v, bool& inserted) {
        if (!n) {
            inserted = true;
            return new Node(k, v);
        }
        if (k < n->key)      n->left  = insertRec(n->left, k, v, inserted);
        else if (n->key < k) n->right = insertRec(n->right, k, v, inserted);
        else { inserted = false; return n; }      // duplicate: nothing changes

        if (!inserted) return n;
        update(n);
        int bf = balance(n);
        if (bf > 1) {                              // left heavy
            if (balance(n->left) < 0)              // LR
                n->left = rotateLeft(n->left);
            return rotateRight(n);                 // LL (or second step of LR)
        }
        if (bf < -1) {                             // right heavy
            if (balance(n->right) > 0)             // RL
                n->right = rotateRight(n->right);
            return rotateLeft(n);                  // RR (or second step of RL)
        }
        return n;
    }

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
        delete n;
    }

public:
    AVL() : root(0), count(0) {}
    ~AVL() { destroy(root); }

    // Returns false (and changes nothing) if the key already exists.
    bool insert(const K& k, const V& v) {
        bool inserted = false;
        root = insertRec(root, k, v, inserted);
        if (inserted) count++;
        return inserted;
    }

    // Returns pointer to the stored value, or 0 if not found.
    V* find(const K& k) const {
        Node* cur = root;
        while (cur) {
            if (k < cur->key) cur = cur->left;
            else if (cur->key < k) cur = cur->right;
            else return &cur->value;
        }
        return 0;
    }

    int size() const { return count; }
    bool empty() const { return count == 0; }

    // Visitor f is called as f(key, value).
    template <typename F> void inorder(F& f) const { inorderRec(root, f); }
    template <typename F> void preorder(F& f) const { preorderRec(root, f); }
};

#endif
