#ifndef HASHMAP_H
#define HASHMAP_H

// Hash table with SEPARATE CHAINING (own linked list per bucket).
// Hash function: index = key % capacity.  Initial capacity 11.
// Resize (after an insertion): when active entries / capacity > 0.75,
// new capacity = smallest prime >= 2 * old capacity.  Never shrinks.
// V is a pointer type; the table never deletes the objects it points to.
template <typename K, typename V>
class HashMap {
    struct Node {
        K key;
        V value;
        Node* next;
        Node(const K& k, const V& v, Node* n) : key(k), value(v), next(n) {}
    };

    Node** buckets;
    int cap;
    int count;

    HashMap(const HashMap&);              // no copying
    HashMap& operator=(const HashMap&);

    static bool isPrime(int n) {
        if (n < 2) return false;
        for (int i = 2; (long long)i * i <= n; i++)
            if (n % i == 0) return false;
        return true;
    }
    static int nextPrime(int n) {
        while (!isPrime(n)) n++;
        return n;
    }

    void resize() {
        int newCap = nextPrime(2 * cap);
        Node** nb = new Node*[newCap];
        for (int i = 0; i < newCap; i++) nb[i] = 0;
        // Re-insert every active entry (nodes are re-linked, not copied).
        for (int i = 0; i < cap; i++) {
            Node* cur = buckets[i];
            while (cur) {
                Node* nxt = cur->next;
                int idx = cur->key % newCap;
                cur->next = nb[idx];
                nb[idx] = cur;
                cur = nxt;
            }
        }
        delete[] buckets;
        buckets = nb;
        cap = newCap;
    }

public:
    HashMap() : buckets(0), cap(11), count(0) {
        buckets = new Node*[cap];
        for (int i = 0; i < cap; i++) buckets[i] = 0;
    }
    ~HashMap() {
        for (int i = 0; i < cap; i++) {
            Node* cur = buckets[i];
            while (cur) { Node* nxt = cur->next; delete cur; cur = nxt; }
        }
        delete[] buckets;
    }

    // Returns false if the key already exists.
    bool insert(const K& k, const V& v) {
        if (find(k)) return false;
        int idx = k % cap;
        buckets[idx] = new Node(k, v, buckets[idx]);
        count++;
        if ((long long)count * 4 > (long long)cap * 3) resize();   // load factor > 0.75
        return true;
    }

    V* find(const K& k) const {
        Node* cur = buckets[k % cap];
        while (cur) {
            if (cur->key == k) return &cur->value;
            cur = cur->next;
        }
        return 0;
    }

    bool remove(const K& k) {
        int idx = k % cap;
        Node* prev = 0;
        Node* cur = buckets[idx];
        while (cur) {
            if (cur->key == k) {
                if (prev) prev->next = cur->next;
                else buckets[idx] = cur->next;
                delete cur;
                count--;
                return true;
            }
            prev = cur;
            cur = cur->next;
        }
        return false;
    }

    int capacity() const { return cap; }
    int size() const { return count; }
};

#endif
