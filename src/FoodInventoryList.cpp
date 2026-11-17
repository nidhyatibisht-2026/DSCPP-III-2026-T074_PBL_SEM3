#include "FoodInventoryList.hpp"

FoodInventoryList::FoodInventoryList() : head(nullptr) {}

FoodInventoryList::~FoodInventoryList() {
    clear();
}

void FoodInventoryList::clear() {
    Node* curr = head;

    while (curr) {
        Node* temp = curr;
        curr = curr->next;
        delete temp;
    }

    head = nullptr;
}

FoodItem* FoodInventoryList::insert(const FoodItem& item) {
    Node* newNode = new Node(item);
    newNode->next = head;
    head = newNode;
    return &(newNode->data);
}

FoodItem* FoodInventoryList::findById(int id) {
    Node* curr = head;

    while (curr) {
        if (curr->data.foodID == id) {
            return &(curr->data);
        }
        curr = curr->next;
    }

    return nullptr;
}

bool FoodInventoryList::removeById(int id) {
    Node* curr = head;
    Node* prev = nullptr;

    while (curr) {
        if (curr->data.foodID == id) {
            if (prev == nullptr) {
                head = curr->next;
            } else {
                prev->next = curr->next;
            }

            delete curr;
            return true;
        }

        prev = curr;
        curr = curr->next;
    }

    return false;
}
void FoodInventoryList::displayAll() const {
    if (!head) {
        std::cout << "  (Inventory is empty)\n";
        return;
    }

    Node* curr = head;
    bool any = false;

    while (curr) {
        if (!curr->data.isAssigned &&
            !curr->data.isCancelled &&
            !curr->data.isExpired) {
            curr->data.display();
            any = true;
        }

        curr = curr->next;
    }

    if (!any) {
        std::cout << "  (No active donation items available)\n";
    }
}
