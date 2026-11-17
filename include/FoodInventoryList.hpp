#ifndef FOODFLOW_FOOD_INVENTORY_LIST_HPP
#define FOODFLOW_FOOD_INVENTORY_LIST_HPP

#include "Models.hpp"

#include <iostream>

class FoodInventoryList {
private:
    struct Node {
        FoodItem data;
        Node* next;

        explicit Node(const FoodItem& food)
            : data(food), next(nullptr) {}
    };

    Node* head;

public:
    FoodInventoryList();
    ~FoodInventoryList();

    FoodInventoryList(const FoodInventoryList&) = delete;
    FoodInventoryList& operator=(const FoodInventoryList&) = delete;

    void clear();
    FoodItem* insert(const FoodItem& item);
    FoodItem* findById(int id);
    bool removeById(int id);
    void displayAll() const;

    template<typename Func>
    void forEach(Func f) {
        Node* curr = head;
        while (curr) {
            f(curr->data);
            curr = curr->next;
        }
    }
};

#endif
