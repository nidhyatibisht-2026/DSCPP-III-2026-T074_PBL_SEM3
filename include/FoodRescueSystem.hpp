#ifndef FOODFLOW_SYSTEM_HPP
#define FOODFLOW_SYSTEM_HPP

#include "HashTable.hpp"
#include "FoodInventoryList.hpp"
#include "MinHeapExpiryQueue.hpp"
#include "FIFOQueue.hpp"
#include "User.hpp"
#include "Models.hpp"
#include "Database.hpp"
#include <string>
#include <vector>

class FoodRescueSystem {
private:
    Database database;
    HashTable allergenDictionary;
    FoodInventoryList inventory;
    MinHeapExpiryQueue urgencyHeap;
    FIFOQueue requestQueue;

    std::vector<User*> userRegistry;
    std::vector<Notification> notifications;
    std::vector<OrderTransaction> orderHistory;


    void seedAllergenDictionary();
    bool interactiveMode = true;
    bool initializeIdSequences();
    int allocateNextId(const std::string& entity);
    User* findUser(int id) const;
    Recipient* findRecipient(int id) const;
    Donor* findDonor(int id) const;
    void addNotification(int userId, const std::string& message);

public:
    FoodRescueSystem();
    ~FoodRescueSystem();

    FoodRescueSystem(const FoodRescueSystem&) = delete;
    FoodRescueSystem& operator=(const FoodRescueSystem&) = delete;

    bool isPhoneRegistered(const std::string& phone) const;
    void autoPurgeExpiredFood();

    bool userExists(int id) const;
    bool recipientExists(int id) const;
    bool isUserActive(int id) const;
    bool isRecipientActive(int id) const;
    bool adminSetUserActive(int userId, bool active);

    int registerDonor(std::string name, std::string contact,
                      std::string address, std::string type);
    int registerNGO(std::string name, std::string contact,
                    std::string address, std::string diet);
    int registerAnimalShelter(std::string name, std::string contact,
                              std::string address, std::string species);

    int donateFood(int donorId, std::string name, std::string category,
                   int quantity, int bestBeforeHours, std::string dietType,
                   std::string ingredients, std::string spice,
                   std::string extraNotes);
    int requestFood(int recipientId, std::string foodName,
                    int quantity, std::string allergies);

    bool cancelDonation(int foodId, int donorId);
    void processMatchingPipeline();
    void setInteractiveMode(bool enabled);
    void confirmDelivery(int orderId, int userId, bool isDonor);
    void fileSpoiledFoodComplaint(int orderId, int recipientId,
                                  const std::string& details);

    void displayUserNotifications(int userId) const;
    void displayOrderTransactions();

    void saveToDisk();
    void loadFromDisk();

    void adminDisplayInventory();
    void adminDisplayFoodHistory();
    void adminDisplayRequestHistory() const;
    void adminDisplayUrgencyHeap();
    void adminDisplayRequestQueue();
    void adminDisplayUsers() const;
    bool adminRemoveFood(int foodId);
    bool adminCancelRequest(int requestId);
};

std::string readValidPhoneNumber(const std::string& prompt,
                                 const FoodRescueSystem& system);

#endif
