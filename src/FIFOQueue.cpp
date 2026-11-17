#include "FIFOQueue.hpp"

FIFOQueue::FIFOQueue()
    : frontNode(nullptr), rearNode(nullptr), count(0) {}

FIFOQueue::~FIFOQueue() {
    clear();
}

void FIFOQueue::push(const FoodRequest& request) {
    Node* newNode = new Node(request);

    if (rearNode == nullptr) {
        frontNode = newNode;
        rearNode = newNode;
    } else {
        rearNode->next = newNode;
        rearNode = newNode;
    }

    ++count;
}

void FIFOQueue::pop() {
    if (frontNode == nullptr) {
        return;
    }

    Node* oldFront = frontNode;
    frontNode = frontNode->next;
    delete oldFront;
    --count;

    if (frontNode == nullptr) {
        rearNode = nullptr;
    }
}

bool FIFOQueue::removeById(int requestId) {
    Node* curr = frontNode;
    Node* prev = nullptr;

    while (curr != nullptr) {
        if (curr->data.requestID == requestId) {
            if (prev == nullptr) {
                frontNode = curr->next;
            } else {
                prev->next = curr->next;
            }

            if (curr == rearNode) {
                rearNode = prev;
            }

            delete curr;
            --count;

            return true;
        }

        prev = curr;
        curr = curr->next;
    }

    return false;
}

FoodRequest& FIFOQueue::front() {
    return frontNode->data;
}

const FoodRequest& FIFOQueue::front() const {
    return frontNode->data;
}

bool FIFOQueue::empty() const {
    return count == 0;
}

std::size_t FIFOQueue::size() const {
    return count;
}

void FIFOQueue::clear() {
    while (!empty()) {
        pop();
    }
}
