#include "MinHeapExpiryQueue.hpp"

#include <algorithm>
#include <iostream>

MinHeapExpiryQueue::MinHeapExpiryQueue(int cap)
    : capacity(cap), currentSize(0) {
    heapArray = new FoodItem*[capacity];

    for (int i = 0; i < capacity; ++i) {
        heapArray[i] = nullptr;
    }
}

MinHeapExpiryQueue::~MinHeapExpiryQueue() {
    delete[] heapArray;
}

void MinHeapExpiryQueue::heapifyUp(int index) {
    while (index > 0) {
        int parent = (index - 1) / 2;

        if (heapArray[index]->expiryTimestamp <
            heapArray[parent]->expiryTimestamp) {
            std::swap(heapArray[index], heapArray[parent]);
            index = parent;
        } else {
            break;
        }
    }
}

void MinHeapExpiryQueue::heapifyDown(int index) {
    while (index < currentSize) {
        int left = 2 * index + 1;
        int right = 2 * index + 2;
        int smallest = index;

        if (left < currentSize &&
            heapArray[left]->expiryTimestamp <
            heapArray[smallest]->expiryTimestamp) {
            smallest = left;
        }

        if (right < currentSize &&
            heapArray[right]->expiryTimestamp <
            heapArray[smallest]->expiryTimestamp) {
            smallest = right;
        }

        if (smallest != index) {
            std::swap(heapArray[index], heapArray[smallest]);
            index = smallest;
        } else {
            break;
        }
    }
}

bool MinHeapExpiryQueue::isEmpty() const {
    return currentSize == 0;
}

void MinHeapExpiryQueue::insert(FoodItem* item) {
    if (!item) {
        return;
    }

    if (currentSize == capacity) {
        int newCapacity = capacity * 2;
        FoodItem** newArray = new FoodItem*[newCapacity];

        for (int i = 0; i < currentSize; ++i) {
            newArray[i] = heapArray[i];
        }

        for (int i = currentSize; i < newCapacity; ++i) {
            newArray[i] = nullptr;
        }

        delete[] heapArray;
        heapArray = newArray;
        capacity = newCapacity;
    }

    heapArray[currentSize] = item;
    heapifyUp(currentSize);
    currentSize++;
}

bool MinHeapExpiryQueue::removeById(int foodId) {
    int index = -1;

    // Find the food item in the heap.
    for (int i = 0; i < currentSize; ++i) {
        if (heapArray[i] && heapArray[i]->foodID == foodId) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        return false;
    }

    // Replace the removed entry with the last heap entry.
    currentSize--;

    if (index != currentSize) {
        heapArray[index] = heapArray[currentSize];
        heapArray[currentSize] = nullptr;

        // Restore min-heap order.
        if (index > 0 &&
            heapArray[index]->expiryTimestamp <
            heapArray[(index - 1) / 2]->expiryTimestamp) {
            heapifyUp(index);
        } else {
            heapifyDown(index);
        }
    } else {
        heapArray[currentSize] = nullptr;
    }

    return true;
}
void MinHeapExpiryQueue::displayHeap() const {
    if (isEmpty()) {
        std::cout << "  (Urgency Heap is empty)\n";
        return;
    }

    int rank = 1;

    for (int i = 0; i < currentSize; ++i) {
        FoodItem* item = heapArray[i];

        if (item && !item->isAssigned &&
            !item->isCancelled && !item->isExpired) {
            double minutes = item->getRemainingMinutes();

            std::cout << "  [" << rank++ << "] Food #"
                      << item->foodID << " - " << item->name
                      << " (Remaining: " << item->getRemainingHours()
                      << " hrs / " << minutes << " mins)";

            if (minutes <= 30.0 && minutes > 0.0) {
                std::cout
                    << " *** CRITICAL: PICKUP REQUIRED WITHIN 30 MINS! ***";
            }

            std::cout << "\n";
        }
    }

    if (rank == 1) {
        std::cout << "  (No active items in urgency radar)\n";
    }
}
