#ifndef FOODFLOW_MODELS_HPP
#define FOODFLOW_MODELS_HPP

#include <iostream>
#include <string>
#include <chrono>

using namespace std;
using namespace std::chrono;

long long getCurrentEpochSeconds();

struct Notification {
    int userId;
    string message;
    long long timestamp;
};

struct FoodItem {
    int foodID;
    string name;
    string category;
    int quantity;
    long long prepTimestamp;
    long long expiryTimestamp;
    int shelfHours;
    string dietType;
    string ingredients;
    string allergens;
    string spiceLevel;
    string extraNotes;
    int donorID;
    string donorAddress;
    bool isAssigned;
    bool isCancelled;
    bool isExpired;

    FoodItem()
        : foodID(0), name(""), category(""), quantity(0), prepTimestamp(0),
          expiryTimestamp(0), shelfHours(0), dietType(""), ingredients(""),
          allergens(""), spiceLevel(""), extraNotes(""), donorID(0),
          donorAddress(""), isAssigned(false), isCancelled(false),
          isExpired(false) {}

    FoodItem(int id, string n, string c, int q, long long pTime, long long eTime,
             int sHrs, string dType, string ing, string alg, string spice,
             string notes, int dId, string addr)
        : foodID(id), name(n), category(c), quantity(q), prepTimestamp(pTime),
          expiryTimestamp(eTime), shelfHours(sHrs), dietType(dType),
          ingredients(ing), allergens(alg), spiceLevel(spice),
          extraNotes(notes), donorID(dId), donorAddress(addr),
          isAssigned(false), isCancelled(false), isExpired(false) {}

    double getRemainingHours() const {
        long long now = getCurrentEpochSeconds();
        if (now >= expiryTimestamp) return 0.0;
        return static_cast<double>(expiryTimestamp - now) / 3600.0;
    }

    double getRemainingMinutes() const {
        long long now = getCurrentEpochSeconds();
        if (now >= expiryTimestamp) return 0.0;
        return static_cast<double>(expiryTimestamp - now) / 60.0;
    }

    void display() const {
        cout << "Food #" << foodID << " [" << name << " | " << dietType
             << " | Spice: " << spiceLevel << "]\n"
             << "       Qty: " << quantity << " servings | Remainder: "
             << getRemainingHours() << " hr(s) (" << getRemainingMinutes()
             << " mins)\n"
             << "       Ingredients: " << ingredients << " | Allergens: "
             << (allergens.empty() ? "None" : allergens) << "\n"
             << "       Donor #" << donorID << " Address: " << donorAddress
             << " | Notes: " << extraNotes << "\n";
    }
};

struct FoodRequest {
    int requestID;
    int recipientID;
    string foodName;
    int requestedQuantity;
    string allergyRestrictions;
    string status;
    long long requestTimestamp;
};

struct OrderTransaction {
    int orderID;
    int foodID;
    int donorID;
    int recipientID;
    int quantity;
    double deliveryCost;
    string status;
    bool donorConfirmed;
    bool recipientConfirmed;
    bool isSpoiledComplaint;

    OrderTransaction(int oId, int fId, int dId, int rId, int qty,
                     double cost, string stat)
        : orderID(oId), foodID(fId), donorID(dId), recipientID(rId),
          quantity(qty), deliveryCost(cost), status(stat),
          donorConfirmed(false), recipientConfirmed(false),
          isSpoiledComplaint(false) {}
};

#endif
