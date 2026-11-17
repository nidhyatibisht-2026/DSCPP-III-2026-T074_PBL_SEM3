#ifndef FOODFLOW_FIFO_QUEUE_HPP
#define FOODFLOW_FIFO_QUEUE_HPP

#include "Models.hpp"

#include <cstddef>

class FIFOQueue {
private:
    struct Node {
        FoodRequest data;
        Node* next;

        explicit Node(const FoodRequest& request)
            : data(request), next(nullptr) {}
    };

    Node* frontNode;
    Node* rearNode;
    std::size_t count;

public:
    FIFOQueue();
    ~FIFOQueue();

    FIFOQueue(const FIFOQueue&) = delete;
    FIFOQueue& operator=(const FIFOQueue&) = delete;

    void push(const FoodRequest& request);
    void pop();
    bool removeById(int requestId);

    FoodRequest& front();
    const FoodRequest& front() const;

    bool empty() const;
    std::size_t size() const;
    void clear();
};

#endif
