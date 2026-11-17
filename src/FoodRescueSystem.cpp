#include "FoodRescueSystem.hpp"
#include "Utilities.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>
#include <stdexcept>
using namespace std;

FoodRescueSystem::FoodRescueSystem()
    : database("database/food_surplus2.db") {
    seedAllergenDictionary();

    if (!database.isConnected()) {
        throw std::runtime_error("Failed to connect to the FoodFlow database.");
    }

    if (!initializeIdSequences()) {
        throw std::runtime_error("Failed to initialize permanent ID sequences.");
    }

    loadFromDisk();
    autoPurgeExpiredFood();
}

bool FoodRescueSystem::initializeIdSequences() {
    sqlite3* db = database.getConnection();

    if (db == nullptr) {
        cerr << "[Database Error] Cannot initialize ID sequences: no connection.\n";
        return false;
    }

    const char* createSql =
        "CREATE TABLE IF NOT EXISTS id_sequences ("
        "entity TEXT PRIMARY KEY, "
        "next_id INTEGER NOT NULL CHECK(next_id > 0)"
        ");";

    if (!database.execute(createSql)) {
        cerr << "[Database Error] Could not create ID sequence table: "
             << sqlite3_errmsg(db) << '\n';
        return false;
    }

    auto initializeSequence = [&](const char* entity,
                                  const char* table,
                                  const char* idColumn) -> bool {
        string maxSql = "SELECT COALESCE(MAX(" + string(idColumn) +
                        "), 0) + 1 FROM " + string(table) + ";";

        sqlite3_stmt* statement = nullptr;
        int result = sqlite3_prepare_v2(
            db, maxSql.c_str(), -1, &statement, nullptr
        );

        if (result != SQLITE_OK) {
            cerr << "[Database Error] Could not read maximum ID for "
                 << table << ": " << sqlite3_errmsg(db) << '\n';
            return false;
        }

        result = sqlite3_step(statement);
        if (result != SQLITE_ROW) {
            cerr << "[Database Error] Could not calculate next ID for "
                 << table << ": " << sqlite3_errmsg(db) << '\n';
            sqlite3_finalize(statement);
            return false;
        }

        int minimumNextId = sqlite3_column_int(statement, 0);
        sqlite3_finalize(statement);

        const char* insertSql =
            "INSERT OR IGNORE INTO id_sequences(entity, next_id) "
            "VALUES (?, ?);";

        result = sqlite3_prepare_v2(
            db, insertSql, -1, &statement, nullptr
        );

        if (result != SQLITE_OK) {
            cerr << "[Database Error] Could not prepare ID sequence "
                 << "initialization: " << sqlite3_errmsg(db) << '\n';
            return false;
        }

        sqlite3_bind_text(statement, 1, entity, -1, SQLITE_STATIC);
        sqlite3_bind_int(statement, 2, minimumNextId);
        result = sqlite3_step(statement);
        sqlite3_finalize(statement);

        if (result != SQLITE_DONE) {
            cerr << "[Database Error] Could not initialize ID sequence "
                 << entity << ": " << sqlite3_errmsg(db) << '\n';
            return false;
        }

        const char* updateSql =
            "UPDATE id_sequences "
            "SET next_id = CASE WHEN next_id < ? THEN ? ELSE next_id END "
            "WHERE entity = ?;";

        result = sqlite3_prepare_v2(
            db, updateSql, -1, &statement, nullptr
        );

        if (result != SQLITE_OK) {
            cerr << "[Database Error] Could not prepare ID sequence "
                 << "synchronization: " << sqlite3_errmsg(db) << '\n';
            return false;
        }

        sqlite3_bind_int(statement, 1, minimumNextId);
        sqlite3_bind_int(statement, 2, minimumNextId);
        sqlite3_bind_text(statement, 3, entity, -1, SQLITE_STATIC);
        result = sqlite3_step(statement);
        sqlite3_finalize(statement);

        if (result != SQLITE_DONE) {
            cerr << "[Database Error] Could not synchronize ID sequence "
                 << entity << ": " << sqlite3_errmsg(db) << '\n';
            return false;
        }

        return true;
    };

    return initializeSequence("user", "users", "id") &&
           initializeSequence("food", "food_items", "food_id") &&
           initializeSequence("request", "food_requests", "request_id") &&
           initializeSequence("order", "orders", "order_id");
}

int FoodRescueSystem::allocateNextId(const string& entity) {
    sqlite3* db = database.getConnection();

    if (db == nullptr) {
        cerr << "[Database Error] Cannot allocate ID: no database connection.\n";
        return -1;
    }

    if (!database.execute("BEGIN IMMEDIATE;")) {
        cerr << "[Database Error] Could not begin ID allocation: "
             << sqlite3_errmsg(db) << '\n';
        return -1;
    }

    sqlite3_stmt* statement = nullptr;
    const char* selectSql =
        "SELECT next_id FROM id_sequences WHERE entity = ?;";

    int result = sqlite3_prepare_v2(
        db, selectSql, -1, &statement, nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "[Database Error] Could not prepare ID allocation: "
             << sqlite3_errmsg(db) << '\n';
        database.execute("ROLLBACK;");
        return -1;
    }

    sqlite3_bind_text(statement, 1, entity.c_str(), -1, SQLITE_TRANSIENT);
    result = sqlite3_step(statement);

    if (result != SQLITE_ROW) {
        cerr << "[Database Error] No ID sequence found for "
             << entity << ": " << sqlite3_errmsg(db) << '\n';
        sqlite3_finalize(statement);
        database.execute("ROLLBACK;");
        return -1;
    }

    int allocatedId = sqlite3_column_int(statement, 0);
    sqlite3_finalize(statement);

    const char* updateSql =
        "UPDATE id_sequences SET next_id = next_id + 1 WHERE entity = ?;";

    result = sqlite3_prepare_v2(
        db, updateSql, -1, &statement, nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "[Database Error] Could not prepare ID sequence update: "
             << sqlite3_errmsg(db) << '\n';
        database.execute("ROLLBACK;");
        return -1;
    }

    sqlite3_bind_text(statement, 1, entity.c_str(), -1, SQLITE_TRANSIENT);
    result = sqlite3_step(statement);
    bool updated = (result == SQLITE_DONE && sqlite3_changes(db) == 1);
    sqlite3_finalize(statement);

    if (!updated) {
        cerr << "[Database Error] Could not advance ID sequence for "
             << entity << ": " << sqlite3_errmsg(db) << '\n';
        database.execute("ROLLBACK;");
        return -1;
    }

    if (!database.execute("COMMIT;")) {
        cerr << "[Database Error] Could not commit ID allocation: "
             << sqlite3_errmsg(db) << '\n';
        database.execute("ROLLBACK;");
        return -1;
    }

    return allocatedId;
}

FoodRescueSystem::~FoodRescueSystem() {
    autoPurgeExpiredFood();
    saveToDisk();

    for (User* user : userRegistry) {
        delete user;
    }
    userRegistry.clear();
}

void FoodRescueSystem::seedAllergenDictionary() {
    allergenDictionary.insert("milk", "Dairy");
    allergenDictionary.insert("milk solids", "Dairy");
    allergenDictionary.insert("cheese", "Dairy");
    allergenDictionary.insert("yogurt", "Dairy");
    allergenDictionary.insert("butter", "Dairy");
    allergenDictionary.insert("paneer", "Dairy");
    allergenDictionary.insert("cashew", "Tree Nuts");
    allergenDictionary.insert("almond", "Tree Nuts");
    allergenDictionary.insert("walnut", "Tree Nuts");
    allergenDictionary.insert("peanut", "Peanuts");
    allergenDictionary.insert("peanuts", "Peanuts");
    allergenDictionary.insert("peanut butter", "Peanuts");
    allergenDictionary.insert("wheat", "Gluten");
    allergenDictionary.insert("maida", "Gluten");
    allergenDictionary.insert("semolina", "Gluten");
    allergenDictionary.insert("bread", "Gluten");
    allergenDictionary.insert("egg", "Egg");
    allergenDictionary.insert("soy", "Soy");
    allergenDictionary.insert("soy sauce", "Soy");
    allergenDictionary.insert("tofu", "Soy");
}

bool FoodRescueSystem::isPhoneRegistered(const string& phone) const {
    for (User* user : userRegistry) {
        if (user->getContact() == phone) return true;
    }
    return false;
}

void FoodRescueSystem::autoPurgeExpiredFood() {
    sqlite3* db = database.getConnection();

    if (db == nullptr) {
        cerr << "[Database Error] Cannot update expired food records.\n";
        return;
    }

    inventory.forEach([&](FoodItem& item) {
        if (!item.isAssigned && !item.isCancelled && !item.isExpired) {
            if (item.getRemainingMinutes() <= 0.0) {

                const char* sql =
                    "UPDATE food_items SET is_expired = 1 "
                    "WHERE food_id = ?;";

                sqlite3_stmt* statement = nullptr;

                int result = sqlite3_prepare_v2(
                    db, sql, -1, &statement, nullptr
                );

                if (result != SQLITE_OK) {
                    cerr << "[Database Error] Could not prepare expiry update: "
                         << sqlite3_errmsg(db) << '\n';
                    return;
                }

                sqlite3_bind_int(statement, 1, item.foodID);

                result = sqlite3_step(statement);
                sqlite3_finalize(statement);

                if (result != SQLITE_DONE) {
                    cerr << "[Database Error] Could not save expired food: "
                         << sqlite3_errmsg(db) << '\n';
                    return;
                }

                item.isExpired = true;

                string alertMsg =
                    "[AUTOMATIC EXPIRY NOTICE] Donation Food #" +
                    to_string(item.foodID) + " (" + item.name +
                    ") expired without recipient requests. Automatically marked EXPIRED and removed from donation radar.";

                addNotification(item.donorID, alertMsg);
            }
        }
    });
}
User* FoodRescueSystem::findUser(int id) const {
    for (User* user : userRegistry) {
        if (user->getId() == id) return user;
    }
    return nullptr;
}

Recipient* FoodRescueSystem::findRecipient(int id) const {
    return dynamic_cast<Recipient*>(findUser(id));
}

Donor* FoodRescueSystem::findDonor(int id) const {
    return dynamic_cast<Donor*>(findUser(id));
}

bool FoodRescueSystem::userExists(int id) const {
    return findUser(id) != nullptr;
}

bool FoodRescueSystem::recipientExists(int id) const {
    return findRecipient(id) != nullptr;
}

bool FoodRescueSystem::isUserActive(int id) const {
    User* user = findUser(id);
    return user != nullptr && user->isActive();
}

bool FoodRescueSystem::isRecipientActive(int id) const {
    Recipient* recipient = findRecipient(id);
    return recipient != nullptr && recipient->isActive();
}

bool FoodRescueSystem::adminSetUserActive(int userId, bool active) {
        sqlite3* db = database.getConnection();

    if (db == nullptr) {
        cout << "Database connection unavailable.\n";
        return false;
    }
    User* user = findUser(userId);

    if (user == nullptr) {
        cout << "User ID not found.\n";
        return false;
    }

    if (user->isActive() == active) {
        cout << "User is already "
             << (active ? "active.\n" : "inactive.\n");
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "UPDATE users SET is_active = ? WHERE id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK) {
        cout << "Database error: " << sqlite3_errmsg(db) << '\n';
        return false;
    }

    sqlite3_bind_int(statement, 1, active ? 1 : 0);
    sqlite3_bind_int(statement, 2, userId);

    bool success = sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);

    if (!success) {
        cout << "Failed to update user status: "
             << sqlite3_errmsg(db) << '\n';
        return false;
    }

    user->setActive(active);

    cout << "User " << userId << " has been "
         << (active ? "reactivated" : "deactivated")
         << ". Historical records have been preserved.\n";

    return true;
}


void FoodRescueSystem::addNotification(int userId, const string& message) {
    sqlite3* db = database.getConnection();

    if (db == nullptr) {
        cerr << "[Database Error] Cannot save notification.\n";
        return;
    }

    long long currentTime = getCurrentEpochSeconds();

    const char* sql =
        "INSERT INTO notifications (user_id, message, timestamp) "
        "VALUES (?, ?, ?);";

    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        db, sql, -1, &statement, nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "[Database Error] Could not prepare notification: "
             << sqlite3_errmsg(db) << '\n';
        return;
    }

    sqlite3_bind_int(statement, 1, userId);
    sqlite3_bind_text(
        statement, 2, message.c_str(), -1, SQLITE_TRANSIENT
    );
    sqlite3_bind_int64(statement, 3, currentTime);

    result = sqlite3_step(statement);
    sqlite3_finalize(statement);

    if (result != SQLITE_DONE) {
        cerr << "[Database Error] Could not save notification: "
             << sqlite3_errmsg(db) << '\n';
        return;
    }

    // Update the in-memory cache only after SQLite confirms the insert.
    notifications.push_back({userId, message, currentTime});
}

int FoodRescueSystem::registerDonor(string name, string contact,
                                    string address, string type) {
    int userId = allocateNextId("user");

    if (userId < 1) {
        cerr << "[Database Error] Failed to allocate user ID.\n";
        return -1;
    }

    Donor* donor = new Donor(userId, name, contact, address, type);

    sqlite3* db = database.getConnection();
    sqlite3_stmt* statement = nullptr;

    const char* sql =
        "INSERT INTO users (id, role, name, contact, address, extra_info) "
        "VALUES (?, 'DONOR', ?, ?, ?, ?);";

    int result = sqlite3_prepare_v2(db, sql, -1, &statement, nullptr);

    if (result != SQLITE_OK) {
        cerr << "[Database Error] " << sqlite3_errmsg(db) << '\n';
        delete donor;
        return -1;
    }

    sqlite3_bind_int(statement, 1, userId);
    sqlite3_bind_text(statement, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, contact.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 4, address.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 5, type.c_str(), -1, SQLITE_TRANSIENT);

    result = sqlite3_step(statement);
    sqlite3_finalize(statement);

    if (result != SQLITE_DONE) {
        cerr << "[Database Error] " << sqlite3_errmsg(db) << '\n';
        delete donor;
        return -1;
    }

    userRegistry.push_back(donor);

    return userId;
}

int FoodRescueSystem::registerNGO(string name, string contact,
                                  string address, string diet) {
    int userId = allocateNextId("user");

    if (userId < 1) {
        cerr << "[Database Error] Failed to allocate user ID.\n";
        return -1;
    }

    NGO* ngo = new NGO(userId, name, contact, address, diet);

    sqlite3* db = database.getConnection();
    sqlite3_stmt* statement = nullptr;

    const char* sql =
        "INSERT INTO users (id, role, name, contact, address, extra_info) "
        "VALUES (?, 'NGO', ?, ?, ?, ?);";

    int result = sqlite3_prepare_v2(db, sql, -1, &statement, nullptr);

    if (result != SQLITE_OK) {
        cerr << "[Database Error] " << sqlite3_errmsg(db) << '\n';
        delete ngo;
        return -1;
    }

    sqlite3_bind_int(statement, 1, userId);
    sqlite3_bind_text(statement, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, contact.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 4, address.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 5, diet.c_str(), -1, SQLITE_TRANSIENT);

    result = sqlite3_step(statement);
    sqlite3_finalize(statement);

    if (result != SQLITE_DONE) {
        cerr << "[Database Error] " << sqlite3_errmsg(db) << '\n';
        delete ngo;
        return -1;
    }

    userRegistry.push_back(ngo);

    return userId;
}

int FoodRescueSystem::registerAnimalShelter(string name, string contact,
                                            string address, string species) {
    int userId = allocateNextId("user");

    if (userId < 1) {
        cerr << "[Database Error] Failed to allocate user ID.\n";
        return -1;
    }

    AnimalShelter* shelter =
        new AnimalShelter(userId, name, contact, address, species);

    sqlite3* db = database.getConnection();
    sqlite3_stmt* statement = nullptr;

    const char* sql =
        "INSERT INTO users (id, role, name, contact, address, extra_info) "
        "VALUES (?, 'SHELTER', ?, ?, ?, ?);";

    int result = sqlite3_prepare_v2(db, sql, -1, &statement, nullptr);

    if (result != SQLITE_OK) {
        cerr << "[Database Error] " << sqlite3_errmsg(db) << '\n';
        delete shelter;
        return -1;
    }

    sqlite3_bind_int(statement, 1, userId);
    sqlite3_bind_text(statement, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, contact.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 4, address.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 5, species.c_str(), -1, SQLITE_TRANSIENT);

    result = sqlite3_step(statement);
    sqlite3_finalize(statement);

    if (result != SQLITE_DONE) {
        cerr << "[Database Error] " << sqlite3_errmsg(db) << '\n';
        delete shelter;
        return -1;
    }

    userRegistry.push_back(shelter);

    return userId;
}

int FoodRescueSystem::donateFood(
    int donorId, string name, string category, int quantity,
    int bestBeforeHours, string dietType, string ingredients,
    string spice, string extraNotes) {

    autoPurgeExpiredFood();

    User* user = findUser(donorId);
    if (!user) {
        cout << "[Error] Donor/Entity #" << donorId
             << " is not registered.\n";
        return -1;
    }
    if (!user->isActive()) {
    cout << "[Error] This donor account is inactive. "
         << "Please contact the administrator.\n";
    return -1;
}

    string identifiedAllergens =
        allergenDictionary.mapIngredientsToAllergens(ingredients);

    long long prepTime = getCurrentEpochSeconds();
    long long expiryTime = prepTime + (bestBeforeHours * 3600LL);

    int foodId = allocateNextId("food");
    if (foodId <= 0) {
        cout << "[Error] Could not allocate a unique Food ID.\n";
        return -1;
    }


    sqlite3* db = database.getConnection();
    sqlite3_stmt* statement = nullptr;

    const char* sql =
        "INSERT INTO food_items "
        "(food_id, name, category, quantity, prep_time, expiry_time, "
        "shelf_hours, diet_type, ingredients, allergens, spice_level, "
        "extra_notes, donor_id, donor_address) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    int result = sqlite3_prepare_v2(
        db, sql, -1, &statement, nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "[Database Error] "
             << sqlite3_errmsg(db) << '\n';
        return -1;
    }

    sqlite3_bind_int(statement, 1, foodId);
    sqlite3_bind_text(statement, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(statement, 4, quantity);
    sqlite3_bind_int64(statement, 5, prepTime);
    sqlite3_bind_int64(statement, 6, expiryTime);
    sqlite3_bind_int(statement, 7, bestBeforeHours);
    sqlite3_bind_text(statement, 8, dietType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 9, ingredients.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 10, identifiedAllergens.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 11, spice.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 12, extraNotes.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(statement, 13, donorId);
    sqlite3_bind_text(statement, 14, user->getAddress().c_str(), -1, SQLITE_TRANSIENT);

    result = sqlite3_step(statement);
    sqlite3_finalize(statement);

    if (result != SQLITE_DONE) {
        cerr << "[Database Error] "
             << sqlite3_errmsg(db) << '\n';
        return -1;
    }

    FoodItem item(
        foodId, name, category, quantity, prepTime,
        expiryTime, bestBeforeHours, dietType, ingredients,
        identifiedAllergens, spice, extraNotes, donorId,
        user->getAddress()
    );

    FoodItem* storedPtr = inventory.insert(item);
    urgencyHeap.insert(storedPtr);

    cout << "\n>>> Item added for donation successfully. Food ID: "
         << storedPtr->foodID << '\n';

    if (storedPtr->getRemainingMinutes() <= 30.0) {
        cout << ">>> [URGENT WINDOW ALERT] Food has 30 mins or less "
             << "remaining! Flagged for quick pickup.\n";
    }

    addNotification(donorId, "Food #" + to_string(storedPtr->foodID) +
                    " (" + name + ") successfully logged for donation.");

    processMatchingPipeline();

    return storedPtr->foodID;
}

int FoodRescueSystem::requestFood(
    int recipientId, string foodName, int quantity, string allergies) {

    autoPurgeExpiredFood();

    Recipient* recipient = findRecipient(recipientId);
    if (!recipient) {
        cout << "[Error] Recipient #" << recipientId
             << " not found in registry.\n";
        return -1;
    }
    if (!recipient->isActive()) {
    cout << "[Error] This recipient account is inactive. "
         << "Please contact the administrator.\n";
    return -1;
}

    if (quantity <= 0) {
        cout << "[Error] Requested quantity must be greater than zero.\n";
        return -1;
    }

    int requestId = allocateNextId("request");
    if (requestId <= 0) {
        cout << "[Error] Could not allocate a unique Request ID.\n";
        return -1;
    }

    long long requestTime = getCurrentEpochSeconds();

    sqlite3* db = database.getConnection();
    sqlite3_stmt* statement = nullptr;

    const char* sql =
        "INSERT INTO food_requests "
        "(request_id, recipient_id, food_name, quantity, allergies, "
        "status, request_timestamp) "
        "VALUES (?, ?, ?, ?, ?, ?, ?);";

    int result = sqlite3_prepare_v2(
        db, sql, -1, &statement, nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "[Database Error] Failed to prepare food request: "
             << sqlite3_errmsg(db) << '\n';
        return -1;
    }

    sqlite3_bind_int(statement, 1, requestId);
    sqlite3_bind_int(statement, 2, recipientId);
    sqlite3_bind_text(statement, 3, foodName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(statement, 4, quantity);
    sqlite3_bind_text(statement, 5, allergies.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 6, "Pending", -1, SQLITE_STATIC);
    sqlite3_bind_int64(statement, 7, requestTime);

    result = sqlite3_step(statement);
    sqlite3_finalize(statement);

    if (result != SQLITE_DONE) {
        cerr << "[Database Error] Failed to save food request: "
             << sqlite3_errmsg(db) << '\n';
        return -1;
    }

    FoodRequest request{
        requestId,
        recipientId,
        foodName,
        quantity,
        allergies,
        "Pending",
        requestTime
    };

    requestQueue.push(request);

    cout << "\n>>> Request #" << request.requestID
         << " saved to database and placed into queue.\n";

    addNotification(recipientId, "Request #" +
                    to_string(request.requestID) + " for " + foodName +
                    " placed into queue.");

    processMatchingPipeline();

    return request.requestID;
}

bool FoodRescueSystem::cancelDonation(int foodId, int donorId) {
    autoPurgeExpiredFood();

    FoodItem* food = inventory.findById(foodId);

    if (!food) {
        cout << "[Error] Food item #" << foodId
             << " not found.\n";
        return false;
    }

    if (food->donorID != donorId) {
        cout << "[Access Denied] You can only cancel your own "
                "donations.\n";
        return false;
    }

    if (food->isAssigned || food->isCancelled) {
        cout << "[Error] This food item is already assigned or "
                "cancelled and cannot be cancelled.\n";
        return false;
    }

    sqlite3* db = database.getConnection();

    if (db == nullptr) {
        cout << "[Database Error] No active database connection.\n";
        return false;
    }

    const char* sql =
        "UPDATE food_items SET is_cancelled = 1 "
        "WHERE food_id = ? AND donor_id = ?;";

    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        db, sql, -1, &statement, nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "[Database Error] Could not prepare cancellation update: "
             << sqlite3_errmsg(db) << '\n';
        return false;
    }

    sqlite3_bind_int(statement, 1, foodId);
    sqlite3_bind_int(statement, 2, donorId);

    result = sqlite3_step(statement);
    int affectedRows = sqlite3_changes(db);

    sqlite3_finalize(statement);

    if (result != SQLITE_DONE || affectedRows == 0) {
        cerr << "[Database Error] Could not save donation cancellation: "
             << sqlite3_errmsg(db) << '\n';
        return false;
    }

    food->isCancelled = true;

    addNotification(donorId,
        "Food #" + to_string(foodId) +
        " (" + food->name +
        ") donation has been cancelled.");

    cout << "\n>>> Donation #" << foodId
         << " cancelled successfully.\n";

    return true;
}

void FoodRescueSystem::setInteractiveMode(bool enabled) {
    interactiveMode = enabled;
}

void FoodRescueSystem::processMatchingPipeline() {
    autoPurgeExpiredFood();

    if (requestQueue.empty()) {
        return;
    }

    sqlite3* db = database.getConnection();

    if (db == nullptr) {
        cerr << "[Database Error] Cannot process requests without a database connection.\n";
        return;
    }

    struct MatchPlan {
        FoodItem* food;
        int quantityToTake;
        int remainingQuantity;
        bool assigned;
        int orderId;
    };

    vector<FoodRequest> pendingRequests;

    // Move requests out of the FIFO queue for processing.
    while (!requestQueue.empty()) {
        pendingRequests.push_back(requestQueue.front());
        requestQueue.pop();
    }

int processedRequests = 0;
int fullyMatchedRequests = 0;
int partiallyMatchedRequests = 0;
int ordersCreated = 0;

    for (auto& req : pendingRequests) {
        if (req.status != "Pending" || req.requestedQuantity <= 0) {
            continue;
        }
        processedRequests++;

        Recipient* recipient = findRecipient(req.recipientID);

        if (!recipient) {
            cerr << "[Matching] Recipient #" << req.recipientID
                 << " not found. Request #" << req.requestID
                 << " will remain pending.\n";
            continue;
        }
        if (!recipient->isActive()) {
    cerr << "[Matching] Recipient #" << req.recipientID
         << " is inactive. Request #" << req.requestID
         << " will remain pending.\n";
    continue;
}

        vector<FoodItem*> compatibleLots;
        int totalAvailable = 0;

        // Find food that matches the request and recipient requirements.
        inventory.forEach([&](FoodItem& food) {
            if (food.isAssigned || food.isCancelled ||
                food.isExpired || food.quantity <= 0) {
                return;
            }
            if (!isUserActive(food.donorID)) {
    return;
}

            if (food.donorID == req.recipientID) {
                return;
            }

            if (!isFoodNameMatch(req.foodName, food.name)) {
    return;
}

            string rejectionReason;

            if (!recipient->isCompatible(food, rejectionReason)) {
                return;
            }

            vector<string> requestedAllergies =
                splitTokens(req.allergyRestrictions);

            vector<string> foodAllergens =
                splitTokens(food.allergens);

            bool allergenConflict = false;

            for (const string& allergy : requestedAllergies) {
                for (const string& allergen : foodAllergens) {
                    if (allergy == allergen ||
                        allergy + "s" == allergen ||
                        allergen + "s" == allergy) {
                        allergenConflict = true;
                        break;
                    }
                }

                if (allergenConflict) {
                    break;
                }
            }

            if (allergenConflict) {
                return;
            }

            compatibleLots.push_back(&food);
            totalAvailable += food.quantity;
        });

        if (compatibleLots.empty()) {
    cout << "[Matching] No suitable food found for Request #"
         << req.requestID << " (" << req.foodName << ").\n";
    cout << "[Matching] Request remains pending for future donations.\n";
    continue;
}

        int needed = req.requestedQuantity;
        bool acceptPartial = false;

        // Ask the recipient before accepting an insufficient quantity.
        if (totalAvailable < needed) {
            if (!interactiveMode) {
                cout << "[Matching] Insufficient compatible food for Request #"
                     << req.requestID
                     << ". Request remains pending in web mode.\n";
                continue;
            }

            cout << "\n[Partial Match] Request #" << req.requestID
                 << " needs " << needed << " units, but only "
                 << totalAvailable << " compatible units are available.\n";

            cout << "Accept partial quantity? (1 = Yes, 2 = Wait): ";

            int choice = readMenuChoice("Your choice: ", 1, 2);

            if (choice == 2) {
                cout << "Request remains pending for future donations.\n";
                continue;
            }

            acceptPartial = true;
        }

        // Prepare the changes without modifying memory yet.
vector<MatchPlan> plans;
int remainingNeeded = needed;
bool orderIdsAllocated = true;

for (FoodItem* foodPtr : compatibleLots) {
    if (!acceptPartial && remainingNeeded <= 0) {
        break;
    }

    int take = acceptPartial
        ? foodPtr->quantity
        : min(foodPtr->quantity, remainingNeeded);

    if (take <= 0) {
        continue;
    }

    // Allocate a unique order ID through SQLite.
    int orderId = allocateNextId("order");

    if (orderId < 1) {
        cerr << "[Database Error] Failed to allocate order ID.\n";
        orderIdsAllocated = false;
        break;
    }

    int newQuantity = foodPtr->quantity - take;
    bool willBeAssigned = (newQuantity == 0);

    plans.push_back({
        foodPtr,
        take,
        newQuantity,
        willBeAssigned,
        orderId
    });

    remainingNeeded -= take;
}

// Do not proceed if any order ID allocation failed.
if (!orderIdsAllocated) {
    cerr << "[Matching] Could not allocate all required order IDs. "
         << "Request remains pending.\n";
    continue;
}

if (plans.empty()) {
    continue;
}

        int newRequestedQuantity = max(0, remainingNeeded);
        string newRequestStatus =
            (newRequestedQuantity == 0) ? "Matched" : "Pending";

        // Start a transaction for this request.
        if (sqlite3_exec(db, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr)
            != SQLITE_OK) {
            cerr << "[Database Error] Could not begin matching transaction: "
                 << sqlite3_errmsg(db) << '\n';
            continue;
        }

        bool transactionSuccessful = true;

        const char* foodSql =
            "UPDATE food_items "
            "SET quantity = ?, is_assigned = ?, "
            "is_cancelled = ?, is_expired = ? "
            "WHERE food_id = ?;";

        const char* orderSql =
            "INSERT INTO orders "
            "(order_id, food_id, donor_id, recipient_id, quantity, "
            "delivery_cost, status, donor_confirmed, recipient_confirmed, "
            "is_spoiled) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

        const char* requestSql =
            "UPDATE food_requests SET quantity = ?, status = ? "
            "WHERE request_id = ?;";

        sqlite3_stmt* foodStmt = nullptr;
        sqlite3_stmt* orderStmt = nullptr;
        sqlite3_stmt* requestStmt = nullptr;

        if (sqlite3_prepare_v2(
                db, foodSql, -1, &foodStmt, nullptr
            ) != SQLITE_OK ||
            sqlite3_prepare_v2(
                db, orderSql, -1, &orderStmt, nullptr
            ) != SQLITE_OK ||
            sqlite3_prepare_v2(
                db, requestSql, -1, &requestStmt, nullptr
            ) != SQLITE_OK) {

            cerr << "[Database Error] Could not prepare matching statements: "
                 << sqlite3_errmsg(db) << '\n';

            transactionSuccessful = false;
        }

        // Save food updates and corresponding orders.
        if (transactionSuccessful) {
            for (const MatchPlan& plan : plans) {
                FoodItem* foodPtr = plan.food;

                sqlite3_bind_int(foodStmt, 1, plan.remainingQuantity);
                sqlite3_bind_int(foodStmt, 2, plan.assigned);
                sqlite3_bind_int(foodStmt, 3, foodPtr->isCancelled);
                sqlite3_bind_int(foodStmt, 4, foodPtr->isExpired);
                sqlite3_bind_int(foodStmt, 5, foodPtr->foodID);

                int result = sqlite3_step(foodStmt);
                int affectedRows = sqlite3_changes(db);

                if (result != SQLITE_DONE || affectedRows == 0) {
                    cerr << "[Database Error] Could not update food #"
                         << foodPtr->foodID << ": "
                         << sqlite3_errmsg(db) << '\n';

                    transactionSuccessful = false;
                    break;
                }

                sqlite3_reset(foodStmt);
                sqlite3_clear_bindings(foodStmt);

                const string orderStatus = "Dispatched";
                const double deliveryFee = 30.0;

                sqlite3_bind_int(orderStmt, 1, plan.orderId);
                sqlite3_bind_int(orderStmt, 2, foodPtr->foodID);
                sqlite3_bind_int(orderStmt, 3, foodPtr->donorID);
                sqlite3_bind_int(orderStmt, 4, recipient->getId());
                sqlite3_bind_int(orderStmt, 5, plan.quantityToTake);
                sqlite3_bind_double(orderStmt, 6, deliveryFee);
                sqlite3_bind_text(
                    orderStmt, 7, orderStatus.c_str(),
                    -1, SQLITE_TRANSIENT
                );
                sqlite3_bind_int(orderStmt, 8, 0);
                sqlite3_bind_int(orderStmt, 9, 0);
                sqlite3_bind_int(orderStmt, 10, 0);

                result = sqlite3_step(orderStmt);

                if (result != SQLITE_DONE) {
                    cerr << "[Database Error] Could not save order #"
                         << plan.orderId << ": "
                         << sqlite3_errmsg(db) << '\n';

                    transactionSuccessful = false;
                    break;
                }

                sqlite3_reset(orderStmt);
                sqlite3_clear_bindings(orderStmt);
            }
        }

        // Save the request's remaining quantity and status.
        if (transactionSuccessful) {
            sqlite3_bind_int(requestStmt, 1, newRequestedQuantity);
            sqlite3_bind_text(
                requestStmt, 2, newRequestStatus.c_str(),
                -1, SQLITE_TRANSIENT
            );
            sqlite3_bind_int(requestStmt, 3, req.requestID);

            if (sqlite3_step(requestStmt) != SQLITE_DONE ||
                sqlite3_changes(db) == 0) {

                cerr << "[Database Error] Could not update request #"
                     << req.requestID << ": "
                     << sqlite3_errmsg(db) << '\n';

                transactionSuccessful = false;
            }
        }

        if (foodStmt != nullptr) {
            sqlite3_finalize(foodStmt);
        }

        if (orderStmt != nullptr) {
            sqlite3_finalize(orderStmt);
        }

        if (requestStmt != nullptr) {
            sqlite3_finalize(requestStmt);
        }

        if (!transactionSuccessful) {
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);

            cerr << "[Matching] Request #" << req.requestID
                 << " was not processed because its database changes failed.\n";

            continue;
        }

        if (sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr)
            != SQLITE_OK) {
            cerr << "[Database Error] Could not commit matching transaction: "
                 << sqlite3_errmsg(db) << '\n';

            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            continue;
        }

        // Only update memory after all database changes have committed.
        for (const MatchPlan& plan : plans) {
            FoodItem* foodPtr = plan.food;

            foodPtr->quantity = plan.remainingQuantity;
            foodPtr->isAssigned = plan.assigned;

            orderHistory.emplace_back(
                plan.orderId,
                foodPtr->foodID,
                foodPtr->donorID,
                recipient->getId(),
                plan.quantityToTake,
                30.0,
                "Dispatched"
            );
            ordersCreated++;

            cout << ">>> Order #" << plan.orderId
                 << " dispatched: " << plan.quantityToTake
                 << " units of " << foodPtr->name << ".\n";

            addNotification(
                foodPtr->donorID,
                "Food #" + to_string(foodPtr->foodID) +
                " has been booked by recipient #" +
                to_string(recipient->getId()) + "."
            );

            addNotification(
                recipient->getId(),
                "Order #" + to_string(plan.orderId) +
                " created for " + foodPtr->name +
                ". Standard fee: Rs. 30."
            );
        }

        req.requestedQuantity = newRequestedQuantity;
        req.status = newRequestStatus;

        if (req.status == "Pending") {
    partiallyMatchedRequests++;

    cout << "[Matching] Request #" << req.requestID
         << " is partially fulfilled. Remaining quantity: "
         << req.requestedQuantity << ".\n";
} else {
    fullyMatchedRequests++;

    cout << "[Matching] Request #" << req.requestID
         << " has been fully matched.\n";
}
    }

    // Reinsert all requests that still need food into the FIFO queue.
    for (const auto& req : pendingRequests) {
        if (req.status == "Pending" && req.requestedQuantity > 0) {
            requestQueue.push(req);
        }
    }
    cout << "\n========== MATCHING PIPELINE SUMMARY ==========\n";
cout << "Requests Processed: " << processedRequests << '\n';
cout << "Fully Matched: " << fullyMatchedRequests << '\n';
cout << "Partially Matched: " << partiallyMatchedRequests << '\n';
cout << "Orders Created: " << ordersCreated << '\n';
cout << "Requests Still Pending: "
     << (processedRequests - fullyMatchedRequests
         - partiallyMatchedRequests) << '\n';
cout << "===============================================\n";
}

void FoodRescueSystem::confirmDelivery(int orderId, int userId, bool isDonor) {
    for (auto& order : orderHistory) {
        if (order.orderID != orderId) continue;

        bool donorConfirmed = order.donorConfirmed;
        bool recipientConfirmed = order.recipientConfirmed;

        if (isDonor && order.donorID == userId) {
            donorConfirmed = true;
        } else if (!isDonor && order.recipientID == userId) {
            recipientConfirmed = true;
        } else {
            cout << "[Error] Order not found for this user and role.\n";
            return;
        }

        string status = (donorConfirmed && recipientConfirmed)
            ? "Delivered/Fulfilled"
            : order.status;

        sqlite3* db = database.getConnection();

        if (db == nullptr) {
            cout << "[Database Error] No active database connection.\n";
            return;
        }

        const char* sql =
            "UPDATE orders "
            "SET donor_confirmed = ?, recipient_confirmed = ?, status = ? "
            "WHERE order_id = ?;";

        sqlite3_stmt* statement = nullptr;

        int result = sqlite3_prepare_v2(
            db, sql, -1, &statement, nullptr
        );

        if (result != SQLITE_OK) {
            cerr << "[Database Error] Could not prepare delivery confirmation: "
                 << sqlite3_errmsg(db) << '\n';
            return;
        }

        sqlite3_bind_int(statement, 1, donorConfirmed ? 1 : 0);
        sqlite3_bind_int(statement, 2, recipientConfirmed ? 1 : 0);
        sqlite3_bind_text(
            statement, 3, status.c_str(), -1, SQLITE_TRANSIENT
        );
        sqlite3_bind_int(statement, 4, orderId);

        result = sqlite3_step(statement);
        sqlite3_finalize(statement);

        if (result != SQLITE_DONE) {
            cerr << "[Database Error] Could not save delivery confirmation: "
                 << sqlite3_errmsg(db) << '\n';
            return;
        }

        // Update the in-memory order only after the database update succeeds.
        order.donorConfirmed = donorConfirmed;
        order.recipientConfirmed = recipientConfirmed;
        order.status = status;

        if (order.donorConfirmed && order.recipientConfirmed) {
            addNotification(order.donorID,
                "Order #" + to_string(orderId) +
                " has been confirmed as delivered by both parties.");

            addNotification(order.recipientID,
                "Order #" + to_string(orderId) +
                " has been confirmed as delivered by both parties.");

            cout << "\n>>> Delivery confirmed by both parties. Order fulfilled.\n";
        } else {
            cout << "\n>>> Confirmation recorded. Waiting for the other party.\n";
        }

        return;
    }

    cout << "[Error] Order #" << orderId << " not found.\n";
}

void FoodRescueSystem::fileSpoiledFoodComplaint(
    int orderId, int recipientId, const string& details) {

    Recipient* recipient = findRecipient(recipientId);

    if (!recipient) {
        cout << "[Error] Recipient not found.\n";
        return;
    }

    for (auto& order : orderHistory) {
        if (order.orderID != orderId ||
            order.recipientID != recipientId) {
            continue;
        }

        if (order.isSpoiledComplaint) {
            cout << "[Error] A complaint has already been filed for this order.\n";
            return;
        }

        sqlite3* db = database.getConnection();

        if (db == nullptr) {
            cout << "[Database Error] No active database connection.\n";
            return;
        }

        // Save the complaint status in SQLite first.
        const char* sql =
            "UPDATE orders "
            "SET is_spoiled = 1, status = ? "
            "WHERE order_id = ? AND recipient_id = ?;";

        sqlite3_stmt* statement = nullptr;

        int result = sqlite3_prepare_v2(
            db, sql, -1, &statement, nullptr
        );

        if (result != SQLITE_OK) {
            cerr << "[Database Error] Could not prepare complaint update: "
                 << sqlite3_errmsg(db) << '\n';
            return;
        }

        const string complaintStatus = "Complaint: Spoiled Food";

        sqlite3_bind_text(
            statement, 1, complaintStatus.c_str(), -1, SQLITE_TRANSIENT
        );
        sqlite3_bind_int(statement, 2, orderId);
        sqlite3_bind_int(statement, 3, recipientId);

        result = sqlite3_step(statement);
        int affectedRows = sqlite3_changes(db);
        sqlite3_finalize(statement);

        if (result != SQLITE_DONE || affectedRows == 0) {
            cerr << "[Database Error] Could not save spoiled-food complaint: "
                 << sqlite3_errmsg(db) << '\n';
            return;
        }

        // Update the in-memory order after the database update succeeds.
        order.isSpoiledComplaint = true;
        order.status = complaintStatus;

        User* donor = findUser(order.donorID);

        if (donor) {
            donor->addPenalty(250.0);

            // Persist the donor's updated penalty details.
            const char* penaltySql =
                "UPDATE users "
                "SET penalty_points = ?, penalty_fines = ? "
                "WHERE id = ?;";

            sqlite3_stmt* penaltyStatement = nullptr;

            int penaltyResult = sqlite3_prepare_v2(
                db, penaltySql, -1, &penaltyStatement, nullptr
            );

            if (penaltyResult == SQLITE_OK) {
                sqlite3_bind_int(
                    penaltyStatement, 1, donor->getPenaltyPoints()
                );
                sqlite3_bind_double(
                    penaltyStatement, 2, donor->getPenaltyFines()
                );
                sqlite3_bind_int(
                    penaltyStatement, 3, donor->getId()
                );

                penaltyResult = sqlite3_step(penaltyStatement);

                if (penaltyResult != SQLITE_DONE) {
                    cerr << "[Database Error] Could not save donor penalty: "
                         << sqlite3_errmsg(db) << '\n';
                }

                sqlite3_finalize(penaltyStatement);
            } else {
                cerr << "[Database Error] Could not prepare donor penalty update: "
                     << sqlite3_errmsg(db) << '\n';
            }

            addNotification(order.donorID,
                "Spoiled-food complaint filed for order #" +
                to_string(orderId) +
                ". Penalty: Rs. 250. Details: " + details);
        }

        addNotification(recipientId,
            "Your spoiled-food complaint for order #" +
            to_string(orderId) + " has been recorded.");

        cout << "\n>>> Complaint recorded successfully.\n";
        return;
    }

    cout << "[Error] Matching order not found for this recipient.\n";
}

void FoodRescueSystem::displayUserNotifications(int userId) const {
    bool found = false;

    cout << "\n========== USER NOTIFICATIONS ==========\n";
    for (const auto& notification : notifications) {
        if (notification.userId != userId) continue;

        found = true;
        cout << "- " << notification.message << '\n';
    }

    if (!found) {
        cout << "No notifications found for user #" << userId << ".\n";
    }
}

void FoodRescueSystem::displayOrderTransactions() {
    sqlite3* db = database.getConnection();

    if (db == nullptr) {
        cout << "[Database Error] No active database connection.\n";
        return;
    }

    cout << "\n========== ORDER TRANSACTION HISTORY ==========\n";

    const char* sql =
        "SELECT o.order_id, o.food_id, f.name, "
        "o.quantity, o.donor_id, d.name, "
        "o.recipient_id, r.name, o.delivery_cost, "
        "o.status, o.donor_confirmed, "
        "o.recipient_confirmed, o.is_spoiled, o.created_at "
        "FROM orders o "
        "LEFT JOIN food_items f ON o.food_id = f.food_id "
        "LEFT JOIN users d ON o.donor_id = d.id "
        "LEFT JOIN users r ON o.recipient_id = r.id "
        "ORDER BY o.order_id DESC;";

    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        db, sql, -1, &statement, nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "[Database Error] "
             << sqlite3_errmsg(db) << '\n';
        return;
    }

    int count = 0;

    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        count++;

        auto getText = [&](int column) -> string {
            const unsigned char* value =
                sqlite3_column_text(statement, column);

            return value
                ? reinterpret_cast<const char*>(value)
                : "Unavailable";
        };

        cout << "\nOrder ID: " << sqlite3_column_int(statement, 0)
             << "\nFood ID: " << sqlite3_column_int(statement, 1)
             << "\nFood Name: " << getText(2)
             << "\nQuantity: " << sqlite3_column_int(statement, 3)
             << "\nDonor ID: " << sqlite3_column_int(statement, 4)
             << "\nDonor Name: " << getText(5)
             << "\nRecipient ID: " << sqlite3_column_int(statement, 6)
             << "\nRecipient Name: " << getText(7)
             << "\nDelivery Fee: Rs. "
             << sqlite3_column_double(statement, 8)
             << "\nStatus: " << getText(9)
             << "\nDonor Confirmed: "
             << (sqlite3_column_int(statement, 10) ? "Yes" : "No")
             << "\nRecipient Confirmed: "
             << (sqlite3_column_int(statement, 11) ? "Yes" : "No")
             << "\nComplaint Filed: "
             << (sqlite3_column_int(statement, 12) ? "Yes" : "No")
             << "\nCreated At: " << getText(13)
             << "\n----------------------------------------\n";
    }

    if (result != SQLITE_DONE) {
        cerr << "[Database Error] Failed to retrieve order history: "
             << sqlite3_errmsg(db) << '\n';
    }

    if (count == 0) {
        cout << "No order transactions recorded yet.\n";
    }

    sqlite3_finalize(statement);
}

void FoodRescueSystem::saveToDisk() {
    sqlite3* db = database.getConnection();

    if (db == nullptr) {
        cerr << "[Database Error] Cannot save user records: "
             << "No active database connection.\n";
        return;
    }

    const char* sql =
        "UPDATE users "
        "SET penalty_points = ?, penalty_fines = ? "
        "WHERE id = ?;";

    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        db, sql, -1, &statement, nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "[Database Error] Failed to prepare user update: "
             << sqlite3_errmsg(db) << '\n';
        return;
    }

    int failedUpdates = 0;
    int successfulUpdates = 0;

    for (const User* user : userRegistry) {

        sqlite3_bind_int(
            statement, 1, user->getPenaltyPoints()
        );

        sqlite3_bind_double(
            statement, 2, user->getPenaltyFines()
        );

        sqlite3_bind_int(
            statement, 3, user->getId()
        );

        result = sqlite3_step(statement);

        if (result != SQLITE_DONE) {
            cerr << "[Database Error] Failed to update user ID "
                 << user->getId() << ": "
                 << sqlite3_errmsg(db) << '\n';

            failedUpdates++;
        }
        else if (sqlite3_changes(db) == 0) {
            cerr << "[Database Warning] No user record found for ID "
                 << user->getId() << ".\n";

            failedUpdates++;
        }
        else {
            successfulUpdates++;
        }

        sqlite3_reset(statement);
        sqlite3_clear_bindings(statement);
    }

    sqlite3_finalize(statement);

    if (failedUpdates == 0) {
        cout << "[System] User records saved to SQLite successfully. "
             << "Updated: " << successfulUpdates << '\n';
    }
    else {
        cerr << "[Database Warning] User records saved with "
             << failedUpdates << " failed update(s). "
             << "Successful updates: " << successfulUpdates << '\n';
    }
}

   void FoodRescueSystem::loadFromDisk() {
    sqlite3* db = database.getConnection();

    if (db == nullptr) {
        cerr << "[Database Error] Cannot load users: "
             << "No active database connection.\n";
        return;
    }

    const char* sql =
    "SELECT id, role, name, contact, address, extra_info, "
    "penalty_points, penalty_fines, is_active "
    "FROM users ORDER BY id;";
    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        db, sql, -1, &statement, nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "[Database Error] Failed to load users: "
             << sqlite3_errmsg(db) << '\n';
        return;
    }

    int maxId = 0;

    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        int id = sqlite3_column_int(statement, 0);

        auto getText = [&](int column) -> string {
            const unsigned char* value =
                sqlite3_column_text(statement, column);

            return value
                ? reinterpret_cast<const char*>(value)
                : "";
        };

        string role = toLower(trim(getText(1)));
        string name = getText(2);
        string contact = getText(3);
        string address = getText(4);
        string extra = getText(5);

        int penaltyPoints = sqlite3_column_int(statement, 6);
        double penaltyFines = sqlite3_column_double(statement, 7);
        bool isActive = sqlite3_column_int(statement, 8) == 1;
        User* user = nullptr;

        if (role == "donor") {
            user = new Donor(
                id, name, contact, address, extra,
                penaltyPoints, penaltyFines
            );
        } else if (role == "ngo") {
            user = new NGO(
                id, name, contact, address, extra,
                penaltyPoints, penaltyFines
            );
        } else if (role == "shelter") {
            user = new AnimalShelter(
                id, name, contact, address, extra,
                penaltyPoints, penaltyFines
            );
        } else {
            cerr << "[Warning] Unknown user role for ID "
                 << id << ". Skipping record.\n";
            continue;
        }
        user->setActive(isActive);
        userRegistry.push_back(user);
        maxId = max(maxId, id);

    }

    if (result != SQLITE_DONE) {
        cerr << "[Database Error] Error while reading users: "
             << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(statement);


// Load all food records from SQLite.
const char* foodSql =
    "SELECT food_id, name, category, quantity, prep_time, expiry_time, "
    "shelf_hours, diet_type, ingredients, allergens, spice_level, "
    "extra_notes, donor_id, donor_address, is_assigned, "
    "is_cancelled, is_expired FROM food_items "
    "WHERE is_cancelled = 0 AND is_expired = 0 "
    "ORDER BY food_id;";

sqlite3_stmt* foodStatement = nullptr;

if (sqlite3_prepare_v2(db, foodSql, -1, &foodStatement, nullptr) == SQLITE_OK) {
    int foodCount = 0;
    int foodResult;

    while ((foodResult = sqlite3_step(foodStatement)) == SQLITE_ROW) {
        auto getText = [&](int column) -> string {
            const unsigned char* value =
                sqlite3_column_text(foodStatement, column);
            return value
                ? reinterpret_cast<const char*>(value)
                : "";
        };

        FoodItem item(
            sqlite3_column_int(foodStatement, 0),
            getText(1),
            getText(2),
            sqlite3_column_int(foodStatement, 3),
            sqlite3_column_int64(foodStatement, 4),
            sqlite3_column_int64(foodStatement, 5),
            sqlite3_column_int(foodStatement, 6),
            getText(7),
            getText(8),
            getText(9),
            getText(10),
            getText(11),
            sqlite3_column_int(foodStatement, 12),
            getText(13)
        );

        item.isAssigned = sqlite3_column_int(foodStatement, 14) != 0;
        item.isCancelled = sqlite3_column_int(foodStatement, 15) != 0;
        item.isExpired = sqlite3_column_int(foodStatement, 16) != 0;

       FoodItem* storedPtr = inventory.insert(item);

if (storedPtr != nullptr &&
    !storedPtr->isAssigned &&
    !storedPtr->isCancelled &&
    !storedPtr->isExpired) {
    urgencyHeap.insert(storedPtr);
}

foodCount++;
    }

    if (foodResult != SQLITE_DONE) {
        cerr << "[Database Error] Failed while loading food items: "
             << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(foodStatement);

    cout << "[System] Loaded " << foodCount
         << " food item(s) from SQLite.\n";

} else {
    cerr << "[Database Error] Failed to load food items: "
         << sqlite3_errmsg(db) << '\n';
}
// Load existing notifications from SQLite.
const char* notificationSql =
    "SELECT user_id, message, timestamp FROM notifications;";

sqlite3_stmt* notificationStatement = nullptr;

if (sqlite3_prepare_v2(
        db, notificationSql, -1, &notificationStatement, nullptr
    ) == SQLITE_OK) {

    int notificationCount = 0;
    int notificationResult;

    while ((notificationResult =
                sqlite3_step(notificationStatement)) == SQLITE_ROW) {

        int userId =
            sqlite3_column_int(notificationStatement, 0);

        const unsigned char* messageValue =
            sqlite3_column_text(notificationStatement, 1);

        string message = messageValue
            ? reinterpret_cast<const char*>(messageValue)
            : "";

        long long timestamp =
            sqlite3_column_int64(notificationStatement, 2);

        notifications.push_back({userId, message, timestamp});
        notificationCount++;
    }

    if (notificationResult != SQLITE_DONE) {
        cerr << "[Database Error] Failed while loading notifications: "
             << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(notificationStatement);

    cout << "[System] Loaded " << notificationCount
         << " notification(s) from SQLite.\n";

} else {
    cerr << "[Database Error] Failed to load notifications: "
         << sqlite3_errmsg(db) << '\n';
}

    // Load existing orders from SQLite.
    const char* orderSql =
        "SELECT order_id, food_id, donor_id, recipient_id, quantity, "
        "delivery_cost, status, donor_confirmed, recipient_confirmed, "
        "is_spoiled FROM orders ORDER BY order_id;";

    sqlite3_stmt* orderStatement = nullptr;

    if (sqlite3_prepare_v2(
            db, orderSql, -1, &orderStatement, nullptr
        ) == SQLITE_OK) {

        int orderCount = 0;
        int orderResult;

        while ((orderResult = sqlite3_step(orderStatement)) == SQLITE_ROW) {

            const unsigned char* statusValue =
                sqlite3_column_text(orderStatement, 6);

            string status = statusValue
                ? reinterpret_cast<const char*>(statusValue)
                : "";

  OrderTransaction order(
    sqlite3_column_int(orderStatement, 0),
    sqlite3_column_int(orderStatement, 1),
    sqlite3_column_int(orderStatement, 2),
    sqlite3_column_int(orderStatement, 3),
    sqlite3_column_int(orderStatement, 4),
    sqlite3_column_double(orderStatement, 5),
    status
);

order.donorConfirmed =
    sqlite3_column_int(orderStatement, 7) != 0;

order.recipientConfirmed =
    sqlite3_column_int(orderStatement, 8) != 0;

order.isSpoiledComplaint =
    sqlite3_column_int(orderStatement, 9) != 0;

            orderHistory.push_back(order);
            orderCount++;
        }

        if (orderResult != SQLITE_DONE) {
            cerr << "[Database Error] Failed while loading orders: "
                 << sqlite3_errmsg(db) << '\n';
        }

        sqlite3_finalize(orderStatement);

        cout << "[System] Loaded " << orderCount
             << " order(s) from SQLite.\n";

    } else {
        cerr << "[Database Error] Failed to load orders: "
             << sqlite3_errmsg(db) << '\n';
    }
    // Restore pending food requests into the FIFO queue.
    const char* requestSql =
        "SELECT request_id, recipient_id, food_name, quantity, "
        "allergies, status, request_timestamp "
        "FROM food_requests "
        "WHERE status = 'Pending' AND quantity > 0 "
        "ORDER BY request_id ASC;";

    sqlite3_stmt* requestStatement = nullptr;

    if (sqlite3_prepare_v2(
            db, requestSql, -1, &requestStatement, nullptr
        ) == SQLITE_OK) {

        int requestCount = 0;
        int requestResult;

        while ((requestResult =
                    sqlite3_step(requestStatement)) == SQLITE_ROW) {

            FoodRequest request;

            request.requestID =
                sqlite3_column_int(requestStatement, 0);

            request.recipientID =
                sqlite3_column_int(requestStatement, 1);

            auto getRequestText = [&](int column) -> string {
                const unsigned char* value =
                    sqlite3_column_text(requestStatement, column);

                return value
                    ? reinterpret_cast<const char*>(value)
                    : "";
            };

            request.foodName = getRequestText(2);

            request.requestedQuantity =
                sqlite3_column_int(requestStatement, 3);

            request.allergyRestrictions = getRequestText(4);
            request.status = getRequestText(5);

            request.requestTimestamp =
                sqlite3_column_int64(requestStatement, 6);

            requestQueue.push(request);
            requestCount++;
        }

        if (requestResult != SQLITE_DONE) {
            cerr << "[Database Error] Failed while loading requests: "
                 << sqlite3_errmsg(db) << '\n';
        }

        sqlite3_finalize(requestStatement);

        cout << "[System] Restored " << requestCount
             << " pending request(s) into the FIFO queue.\n";

    } else {
        cerr << "[Database Error] Failed to load pending requests: "
             << sqlite3_errmsg(db) << '\n';
    }
}


void FoodRescueSystem::adminDisplayInventory() {
    autoPurgeExpiredFood();
    cout << "\n========== FOOD INVENTORY (CUSTOM LINKED LIST) ==========\n";
    inventory.displayAll();
}

void FoodRescueSystem::adminDisplayFoodHistory() {
    sqlite3* db = database.getConnection();

    if (db == nullptr) {
        cout << "[Database Error] No active database connection.\n";
        return;
    }

    cout << "\n========== COMPLETE FOOD HISTORY ==========\n";

    const char* sql =
        "SELECT f.food_id, f.name, f.category, f.quantity, "
        "f.donor_id, u.name, f.is_assigned, "
        "f.is_cancelled, f.is_expired, f.logged_at "
        "FROM food_items f "
        "LEFT JOIN users u ON f.donor_id = u.id "
        "ORDER BY f.food_id DESC;";

    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        db, sql, -1, &statement, nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "[Database Error] "
             << sqlite3_errmsg(db) << '\n';
        return;
    }

    int count = 0;

    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        count++;

        cout << "\nFood ID: " << sqlite3_column_int(statement, 0)
             << "\nFood Name: "
             << reinterpret_cast<const char*>(sqlite3_column_text(statement, 1))
             << "\nCategory: "
             << reinterpret_cast<const char*>(sqlite3_column_text(statement, 2))
             << "\nQuantity: " << sqlite3_column_int(statement, 3)
             << "\nDonor ID: " << sqlite3_column_int(statement, 4)
             << "\nDonor Name: ";

        const unsigned char* donorName = sqlite3_column_text(statement, 5);
        cout << (donorName
            ? reinterpret_cast<const char*>(donorName)
            : "Unavailable");

        cout << "\nAssigned: "
             << (sqlite3_column_int(statement, 6) ? "Yes" : "No")
             << "\nCancelled: "
             << (sqlite3_column_int(statement, 7) ? "Yes" : "No")
             << "\nExpired: "
             << (sqlite3_column_int(statement, 8) ? "Yes" : "No");

        const unsigned char* loggedAt = sqlite3_column_text(statement, 9);
        cout << "\nAdded On: "
             << (loggedAt
                 ? reinterpret_cast<const char*>(loggedAt)
                 : "Unavailable")
             << "\n----------------------------------------\n";
    }

    if (result != SQLITE_DONE) {
        cerr << "[Database Error] Failed to retrieve food history.\n";
    }

    if (count == 0) {
        cout << "No food records found.\n";
    }

    sqlite3_finalize(statement);
}

void FoodRescueSystem::adminDisplayRequestHistory() const {
    sqlite3* db = database.getConnection();

    if (db == nullptr) {
        cout << "[Database Error] No active database connection.\n";
        return;
    }

    cout << "\n========== COMPLETE FOOD REQUEST HISTORY ==========\n";

    const char* sql =
        "SELECT fr.request_id, fr.recipient_id, u.name, "
        "fr.food_name, fr.quantity, fr.allergies, "
        "fr.status, fr.request_timestamp "
        "FROM food_requests fr "
        "LEFT JOIN users u ON fr.recipient_id = u.id "
        "ORDER BY fr.request_id DESC;";

    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        db, sql, -1, &statement, nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "[Database Error] "
             << sqlite3_errmsg(db) << '\n';
        return;
    }

    int count = 0;

    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        count++;

        int requestId = sqlite3_column_int(statement, 0);
        int recipientId = sqlite3_column_int(statement, 1);

        const unsigned char* recipientName =
            sqlite3_column_text(statement, 2);

        const unsigned char* foodName =
            sqlite3_column_text(statement, 3);

        int quantity = sqlite3_column_int(statement, 4);

        const unsigned char* allergies =
            sqlite3_column_text(statement, 5);

        const unsigned char* status =
            sqlite3_column_text(statement, 6);

        long long timestamp =
            sqlite3_column_int64(statement, 7);

        time_t requestTime = static_cast<time_t>(timestamp);
        tm* localTime = localtime(&requestTime);

        char formattedTime[30] = "Unavailable";

        if (localTime != nullptr) {
            strftime(
                formattedTime,
                sizeof(formattedTime),
                "%d-%m-%Y %I:%M:%S %p",
                localTime
            );
        }

        cout << "\n----------------------------------------\n";
        cout << "Request ID: " << requestId
             << "\nRecipient ID: " << recipientId
             << "\nRecipient Name: "
             << (recipientName
                     ? reinterpret_cast<const char*>(recipientName)
                     : "Unknown")
             << "\nFood Requested: "
             << (foodName
                     ? reinterpret_cast<const char*>(foodName)
                     : "Unknown")
             << "\nQuantity: " << quantity
             << "\nAllergies: "
             << (allergies
                     ? reinterpret_cast<const char*>(allergies)
                     : "None")
             << "\nStatus: "
             << (status
                     ? reinterpret_cast<const char*>(status)
                     : "Unknown")
             << "\nRequest Date: " << formattedTime << '\n';
    }

    if (result != SQLITE_DONE) {
        cerr << "[Database Error] "
             << sqlite3_errmsg(db) << '\n';
    }

    sqlite3_finalize(statement);

    if (count == 0) {
        cout << "No food requests found.\n";
    } else {
        cout << "\nTotal Requests: " << count << '\n';
    }

    cout << "===============================================\n";
}

void FoodRescueSystem::adminDisplayUrgencyHeap() {
    autoPurgeExpiredFood();
    cout << "\n========== EXPIRY PRIORITY (CUSTOM MIN HEAP) ==========\n";
    urgencyHeap.displayHeap();
}

void FoodRescueSystem::adminDisplayRequestQueue() {
    autoPurgeExpiredFood();
    cout << "\n========== REQUEST PIPELINE (CUSTOM FIFO QUEUE) ==========\n";

    if (requestQueue.empty()) {
        cout << "No pending requests in the queue.\n";
        return;
    }

    size_t count = requestQueue.size();
    for (size_t i = 0; i < count; ++i) {
        const FoodRequest& request = requestQueue.front();
        cout << "Request #" << request.requestID
             << " | Recipient #" << request.recipientID
             << " | Food: " << request.foodName
             << " | Quantity: " << request.requestedQuantity
             << " | Status: " << request.status << '\n';

        FoodRequest copy = request;
        requestQueue.pop();
        requestQueue.push(copy);
    }
}

void FoodRescueSystem::adminDisplayUsers() const {
    cout << "\n========== REGISTERED USERS ==========\n";

    if (userRegistry.empty()) {
        cout << "No registered users found.\n";
        return;
    }

    for (const User* user : userRegistry) {
        cout << "\nID: " << user->getId()
             << "\nName: " << user->getName()
             << "\nContact: " << user->getContact()
             << "\nAddress: " << user->getAddress()
             << "\nStatus: " << (user->isActive() ? "ACTIVE" : "INACTIVE")
             << "\nPenalty Points: " << user->getPenaltyPoints()
             << "\nPenalty Fines: Rs. " << user->getPenaltyFines()
             << "\n----------------------------------------\n";
    }
}

bool FoodRescueSystem::adminRemoveFood(int foodId) {
    FoodItem* food = inventory.findById(foodId);

    if (!food) {
        cout << "[Admin Error] Food item #" << foodId
             << " was not found.\n";
        return false;
    }

    if (food->isAssigned) {
        cout << "[Admin Error] This food item is already assigned "
                "to an order and cannot be removed.\n";
        return false;
    }

    sqlite3* db = database.getConnection();

    if (db == nullptr) {
        cout << "[Database Error] No active database connection.\n";
        return false;
    }

// Preserve the food record in database history.
const char* sql =
    "UPDATE food_items "
    "SET is_cancelled = 1 "
    "WHERE food_id = ? AND is_cancelled = 0;";

sqlite3_stmt* statement = nullptr;

int result = sqlite3_prepare_v2(
    db, sql, -1, &statement, nullptr
);

if (result != SQLITE_OK) {
    cerr << "[Database Error] "
         << sqlite3_errmsg(db) << '\n';
    return false;
}

sqlite3_bind_int(statement, 1, foodId);

result = sqlite3_step(statement);
int affectedRows = sqlite3_changes(db);

sqlite3_finalize(statement);

if (result != SQLITE_DONE || affectedRows == 0) {
    cerr << "[Database Error] Could not cancel food item.\n";
    return false;
}

    // Remove the pointer from the expiry heap before
    // deleting the linked-list node.
    urgencyHeap.removeById(foodId);
    inventory.removeById(foodId);
    cout << "\n[Admin] Food item #" << foodId
         << " removed from active inventory. "
         << "Historical record preserved.\n";

    return true;
}

bool FoodRescueSystem::adminCancelRequest(int requestId) {
    sqlite3* db = database.getConnection();

    if (db == nullptr) {
        cout << "[Database Error] No active database connection.\n";
        return false;
    }

    // Find the recipient before changing the request.
    const char* selectSql =
        "SELECT recipient_id FROM food_requests "
        "WHERE request_id = ? AND status = 'Pending';";

    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        db, selectSql, -1, &statement, nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "[Database Error] "
             << sqlite3_errmsg(db) << '\n';
        return false;
    }

    sqlite3_bind_int(statement, 1, requestId);

    result = sqlite3_step(statement);

    if (result != SQLITE_ROW) {
        sqlite3_finalize(statement);

        cout << "[Admin Error] Request #" << requestId
             << " was not found or is no longer pending.\n";
        return false;
    }

    int recipientId = sqlite3_column_int(statement, 0);
    sqlite3_finalize(statement);

    // Update the database.
    const char* updateSql =
        "UPDATE food_requests "
        "SET status = 'Cancelled' "
        "WHERE request_id = ? AND status = 'Pending';";

    result = sqlite3_prepare_v2(
        db, updateSql, -1, &statement, nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "[Database Error] "
             << sqlite3_errmsg(db) << '\n';
        return false;
    }

    sqlite3_bind_int(statement, 1, requestId);

    result = sqlite3_step(statement);
    int affectedRows = sqlite3_changes(db);

    sqlite3_finalize(statement);

    if (result != SQLITE_DONE || affectedRows == 0) {
        cerr << "[Database Error] Could not cancel request.\n";
        return false;
    }

    // Remove the request from the FIFO queue.
    requestQueue.removeById(requestId);

    // Notify the affected recipient.
    addNotification(
        recipientId,
        "Your food request #" + to_string(requestId) +
        " has been cancelled by the administrator."
    );

    cout << "\n[Admin] Request #" << requestId
         << " cancelled successfully.\n";

    return true;
}

string readValidPhoneNumber(const string& prompt,
                            const FoodRescueSystem& system) {
    while (true) {
        string phone = readNonEmptyLine(prompt);

        bool valid = phone.size() == 10;
        for (char ch : phone) {
            if (!isdigit(static_cast<unsigned char>(ch))) {
                valid = false;
                break;
            }
        }

        if (!valid) {
            cout << "[Error] Enter a valid 10-digit phone number.\n";
            continue;
        }

        if (system.isPhoneRegistered(phone)) {
            cout << "[Error] This phone number is already registered.\n";
            continue;
        }

        return phone;
    }
}