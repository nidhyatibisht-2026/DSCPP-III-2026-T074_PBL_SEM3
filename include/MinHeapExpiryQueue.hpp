#ifndef FOODFLOW_MIN_HEAP_EXPIRY_QUEUE_HPP
#define FOODFLOW_MIN_HEAP_EXPIRY_QUEUE_HPP

#include "Models.hpp"

class MinHeapExpiryQueue {
private:
    FoodItem** heapArray;
    int capacity;
    int currentSize;

    void heapifyUp(int index);
    void heapifyDown(int index);

public:
    explicit MinHeapExpiryQueue(int cap = 100);
    ~MinHeapExpiryQueue();

    MinHeapExpiryQueue(const MinHeapExpiryQueue&) = delete;
    MinHeapExpiryQueue& operator=(const MinHeapExpiryQueue&) = delete;

    bool isEmpty() const;
    void insert(FoodItem* item);
    bool removeById(int foodId);
    void displayHeap() const;
};

#endif
