#ifndef MAXHEAP_H
#define MAXHEAP_H

#include <utility>      // std::swap
#include "Incident.h"   // higherPriority()

// Array-based binary max-heap (0-indexed). T is Incident*.
// parent(i) = (i-1)/2, children 2i+1 and 2i+2.
// Starts with capacity 16 and doubles when full.
// The heap only stores pointers; it never deletes the incidents.
template <typename T>
class MaxHeap {
    T* data;
    int cap;
    int count;

    MaxHeap(const MaxHeap&);              // no copying
    MaxHeap& operator=(const MaxHeap&);

    void grow() {
        T* nd = new T[cap * 2];
        for (int i = 0; i < count; i++) nd[i] = data[i];
        delete[] data;
        data = nd;
        cap *= 2;
    }

    void siftUp(int i) {
        while (i > 0) {
            int p = (i - 1) / 2;
            if (higherPriority(data[i], data[p])) {
                std::swap(data[i], data[p]);
                i = p;
            } else break;
        }
    }

    void siftDown(int i) {
        while (true) {
            int l = 2 * i + 1, r = 2 * i + 2, best = i;
            if (l < count && higherPriority(data[l], data[best])) best = l;
            if (r < count && higherPriority(data[r], data[best])) best = r;
            if (best == i) break;
            std::swap(data[i], data[best]);
            i = best;
        }
    }

public:
    MaxHeap() : data(0), cap(16), count(0) { data = new T[cap]; }
    ~MaxHeap() { delete[] data; }

    void insert(T item) {
        if (count == cap) grow();
        data[count] = item;
        siftUp(count);
        count++;
    }

    // Highest-priority element (heap must not be empty).
    T peek() const { return data[0]; }

    // Removes and returns the highest-priority element (heap must not be empty).
    T extractMax() {
        T top = data[0];
        count--;
        if (count > 0) {
            data[0] = data[count];      // last element goes to the root
            siftDown(0);
        }
        return top;
    }

    // Removes the element whose incident ID is id. O(n) search + O(log n) repair.
    bool removeById(int id) {
        int i = -1;
        for (int j = 0; j < count; j++)
            if (data[j]->id == id) { i = j; break; }
        if (i < 0) return false;
        count--;
        if (i == count) return true;    // it was the last element
        data[i] = data[count];          // replace with the last element
        if (i > 0 && higherPriority(data[i], data[(i - 1) / 2])) siftUp(i);
        else siftDown(i);
        return true;
    }

    int size() const { return count; }
    bool empty() const { return count == 0; }
};

#endif
