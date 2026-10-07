#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <limits>
#include <cctype>
using namespace std;

int readInt(const string &prompt) {
    int value;
    while (true) {
        cout << prompt;
        if (cin >> value) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        }
        cout << "Invalid input - please enter a whole number.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

int readNonNegativeInt(const string &prompt) {
    while (true) {
        int v = readInt(prompt);
        if (v >= 0) return v;
        cout << "Value cannot be negative. Try again.\n";
    }
}

string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(),
              [](unsigned char c) { return static_cast<char>(tolower(c)); });
    return s;
}

string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t");
    size_t end = s.find_last_not_of(" \t");
    if (start == string::npos) return "";
    return s.substr(start, end - start + 1);
}

vector<string> splitTokens(const string &s) {
    vector<string> tokens;
    stringstream ss(s);
    string part;

    while (getline(ss, part, ',')) {
        string t = toLower(trim(part));
        if (!t.empty() && t != "none") {
            if (find(tokens.begin(), tokens.end(), t) == tokens.end())
                tokens.push_back(t);
        }
    }
    return tokens;
}

bool containsToken(const string &list, const string &value) {
    string target = toLower(trim(value));
    if (target.empty() || target == "none") return false;

    for (const string &token : splitTokens(list)) {
        if (token == target) return true;
    }
    return false;
}

// ---------------- Base class ----------------
class User {
protected:
    int id;
    string name;

public:
    User(int id, const string &name) : id(id), name(name) {}
    virtual ~User() = default;

    int getId() const { return id; }

    virtual void display() const {
        cout << "User #" << id << " - " << name << endl;
    }
};

class Donor : public User {
    string address;

public:
    Donor(int id, const string &name, const string &address)
        : User(id, name), address(address) {}

    string getAddress() const { return address; }

    void display() const override {
        cout << "Donor #" << id << " - " << name
             << " | Address: " << address << endl;
    }
};

class NGO : public User {
    string requirements;
    string allergyRestrictions;

public:
    NGO(int id, const string &name, const string &requirements,
        const string &allergyRestrictions)
        : User(id, name),
          requirements(requirements),
          allergyRestrictions(allergyRestrictions) {}

    string getRequirements() const { return requirements; }

    string getAllergyRestrictions() const {
        return allergyRestrictions;
    }

    void display() const override {
        cout << "NGO #" << id << " - " << name
             << " | Needs: "
             << (requirements.empty() ? "(not specified)" : requirements)
             << " | Cannot accept: "
             << (allergyRestrictions.empty() ? "(none stated)" : allergyRestrictions)
             << endl;
    }
};

// ---------------- Food ----------------
struct FoodItem {
    int id, donorId, expiryDays;
    string foodName;
    string allergens;
    string foodType;
    string pickupAddress;

    void display() const {
        cout << "FoodItem#" << id << " [" << foodName << "]"
             << " | Type: " << foodType
             << " | Expires in: " << expiryDays << " day(s)"
             << " | Donor#" << donorId
             << " | Pickup Address: " << pickupAddress
             << " | Allergens: "
             << (allergens.empty() ? "none listed" : allergens)
             << endl;
    }
};

struct FoodNode {
    FoodItem item;
    FoodNode *next;

    FoodNode(const FoodItem &item) : item(item), next(nullptr) {}
};

struct FoodRequest {
    int id, ngoId;
    string foodName;

    void display() const {
        cout << "Request#" << id
             << " [" << foodName
             << "] by NGO#" << ngoId << endl;
    }
};

// ---------------- Min Heap ----------------
struct CompareExpiry {
    bool operator()(const FoodItem &a, const FoodItem &b) const {
        return a.expiryDays > b.expiryDays;
    }
};

// ---------------- System ----------------
class FoodSurplusManagementSystem {
private:
    vector<Donor> donors;
    vector<NGO> ngos;

    // Complete inventory
    FoodNode *foodHead = nullptr;

    // Expiry priority, grouped by normalized food name
    map<string, priority_queue<FoodItem, vector<FoodItem>, CompareExpiry>>
        foodQueuesByName;

    // Allergen -> food IDs
    unordered_map<string, vector<int>> allergyHashTable;

    // NGO requests in FIFO order
    queue<FoodRequest> requestQueue;

    vector<string> history;
    map<int, vector<int>> donorToNgo;

    int nextUserId = 1;
    int nextFoodId = 1;
    int nextRequestId = 1;

    void addToFoodInventory(const FoodItem &item) {
        FoodNode *newNode = new FoodNode(item);

        if (!foodHead) {
            foodHead = newNode;
            return;
        }

        FoodNode *current = foodHead;
        while (current->next)
            current = current->next;

        current->next = newNode;
    }

    void addToAllergyHash(const FoodItem &item) {
        for (const string &allergen : splitTokens(item.allergens)) {
            allergyHashTable[allergen].push_back(item.id);
        }
    }

    void removeFromAllergyHash(const FoodItem &item) {
        for (const string &allergen : splitTokens(item.allergens)) {
            auto it = allergyHashTable.find(allergen);
            if (it == allergyHashTable.end())
                continue;

            auto &ids = it->second;
            ids.erase(remove(ids.begin(), ids.end(), item.id), ids.end());

            if (ids.empty())
                allergyHashTable.erase(it);
        }
    }

    bool removeFromFoodInventory(int foodId, FoodItem &removed) {
        FoodNode *current = foodHead;
        FoodNode *previous = nullptr;

        while (current) {
            if (current->item.id == foodId) {
                removed = current->item;

                if (previous)
                    previous->next = current->next;
                else
                    foodHead = current->next;

                delete current;
                return true;
            }

            previous = current;
            current = current->next;
        }

        return false;
    }

    void clearFoodInventory() {
        while (foodHead) {
            FoodNode *temp = foodHead;
            foodHead = foodHead->next;
            delete temp;
        }
    }

    void showFoodInventory() {
        cout << "\n--- Food Inventory (Linked List) ---\n";

        if (!foodHead) {
            cout << "  (empty)\n";
            return;
        }

        FoodNode *current = foodHead;
        while (current) {
            cout << "  ";
            current->item.display();
            current = current->next;
        }
    }

    void allergyLookup(const string &allergen) {
        string key = toLower(trim(allergen));

        if (key.empty() || key == "none") {
            cout << "Please enter a valid allergen.\n";
            return;
        }

        cout << "\n--- Allergy Hash Table Lookup ---\n";

        auto it = allergyHashTable.find(key);

        if (it == allergyHashTable.end()) {
            cout << "No active food items found containing allergen: "
                 << key << endl;
            return;
        }

        cout << "Active food item IDs containing " << key << ": ";
        for (int id : it->second)
            cout << "#" << id << " ";

        cout << endl;
    }

    NGO *findNgoById(int ngoId) {
        for (auto &n : ngos) {
            if (n.getId() == ngoId)
                return &n;
        }
        return nullptr;
    }

    bool isAllergySafe(const FoodItem &food, const NGO &ngo) {
        vector<string> foodAllergens = splitTokens(food.allergens);
        vector<string> restrictions = splitTokens(ngo.getAllergyRestrictions());

        for (const string &allergen : foodAllergens) {
            for (const string &restricted : restrictions) {
                if (allergen == restricted)
                    return false;
            }
        }

        return true;
    }

    bool meetsRequirement(const FoodItem &food, const NGO &ngo) {
        vector<string> requirements =
            splitTokens(ngo.getRequirements());

        // "none" / empty means no specific food requirement.
        if (requirements.empty())
            return true;

        string foodName = toLower(trim(food.foodName));

        for (const string &required : requirements) {
            if (foodName == required)
                return true;
        }

        return false;
    }

    bool tryMatch(const FoodRequest &req) {
        // NEVER allow an unregistered NGO to reach matching.
        NGO *ngo = findNgoById(req.ngoId);

        if (ngo == nullptr) {
            cout << "MATCH REJECTED: NGO #" << req.ngoId
                 << " is not registered.\n";
            return false;
        }

        string requestedFood = toLower(trim(req.foodName));

        if (requestedFood.empty()) {
            cout << "MATCH REJECTED: Food name cannot be empty.\n";
            return false;
        }

        auto it = foodQueuesByName.find(requestedFood);

        if (it == foodQueuesByName.end() || it->second.empty()) {
            cout << "No available food for request: "
                 << req.foodName << endl;
            return false;
        }

        auto &pq = it->second;

        vector<FoodItem> setAside;
        bool found = false;
        FoodItem matched{};

        while (!pq.empty()) {
            FoodItem candidate = pq.top();
            pq.pop();

            // 1. Food-name condition is already guaranteed by the queue key.
            // 2. NGO's requested requirement must be satisfied.
            if (!meetsRequirement(candidate, *ngo)) {
                setAside.push_back(candidate);
                continue;
            }

            // 3. Food type condition.
            // If NGO requirement explicitly contains "veg" or "non-veg",
            // enforce it. Otherwise there is no dietary-type restriction.
            bool dietaryConflict = false;
            vector<string> reqTokens =
                splitTokens(ngo->getRequirements());

            bool wantsVeg = false;
            bool wantsNonVeg = false;

            for (const string &token : reqTokens) {
                if (token == "veg" || token == "vegetarian")
                    wantsVeg = true;
                if (token == "non-veg" || token == "nonveg" ||
                    token == "non vegetarian" || token == "non-vegetarian")
                    wantsNonVeg = true;
            }

            if ((wantsVeg && candidate.foodType != "Veg") ||
                (wantsNonVeg && candidate.foodType != "Non-Veg")) {
                dietaryConflict = true;
            }

            if (dietaryConflict) {
                setAside.push_back(candidate);
                continue;
            }

            // 4. ALLERGY SAFETY CHECK.
            if (!isAllergySafe(candidate, *ngo)) {
                cout << "Skipped FoodItem#" << candidate.id
                     << " because of an allergy conflict.\n";
                setAside.push_back(candidate);
                continue;
            }

            matched = candidate;
            found = true;
            break;
        }

        // Put rejected-but-still-available food back.
        for (const auto &item : setAside)
            pq.push(item);

        if (!found) {
            cout << "No safe/suitable food found for NGO#"
                 << req.ngoId << ". Request remains pending.\n";
            return false;
        }

        // Remove matched food from ALL active inventory structures.
        FoodItem removed;
        if (!removeFromFoodInventory(matched.id, removed)) {
            // Safety rollback: don't report a match if inventory is inconsistent.
            pq.push(matched);
            cout << "ERROR: Food inventory is inconsistent. Match cancelled.\n";
            return false;
        }

        removeFromAllergyHash(matched);

        // The matched item was already popped from the priority queue,
        // so it is NOT pushed back.

        cout << "\nMATCHED: " << matched.foodName
             << " [" << matched.foodType << "]"
             << " (Food#" << matched.id
             << ", Donor#" << matched.donorId
             << ") -> NGO#" << req.ngoId << endl;

        cout << "PICKUP ADDRESS: "
             << matched.pickupAddress << endl;

        donorToNgo[matched.donorId].push_back(req.ngoId);

        history.push_back(
            "Donor#" + to_string(matched.donorId) +
            " -> NGO#" + to_string(req.ngoId) +
            " (" + matched.foodName + ", " + matched.foodType +
            ") | Pickup: " + matched.pickupAddress
        );

        return true;
    }

public:
    FoodSurplusManagementSystem() = default;

    FoodSurplusManagementSystem(
        const FoodSurplusManagementSystem &) = delete;

    FoodSurplusManagementSystem &operator=(
        const FoodSurplusManagementSystem &) = delete;

    bool donorExists(int donorId) const {
        for (const auto &d : donors) {
            if (d.getId() == donorId)
                return true;
        }
        return false;
    }

    bool ngoExists(int ngoId) const {
        for (const auto &n : ngos) {
            if (n.getId() == ngoId)
                return true;
        }
        return false;
    }

    int registerDonor(const string &name, const string &address) {
        string cleanName = trim(name);
        string cleanAddress = trim(address);

        if (cleanName.empty() || cleanAddress.empty()) {
            cout << "Error: Donor name and address cannot be empty.\n";
            return -1;
        }

        donors.emplace_back(nextUserId, cleanName, cleanAddress);
        return nextUserId++;
    }

    int registerNGO(const string &name, const string &requirements,
                    const string &allergyRestrictions) {
        string cleanName = trim(name);

        if (cleanName.empty()) {
            cout << "Error: NGO name cannot be empty.\n";
            return -1;
        }

        ngos.emplace_back(
            nextUserId,
            cleanName,
            trim(requirements),
            trim(allergyRestrictions)
        );

        return nextUserId++;
    }

    bool donateFood(int donorId, const string &foodName,
                    int expiryDays, const string &allergens,
                    const string &foodType) {

        // REGISTRATION CHECK
        if (!donorExists(donorId)) {
            cout << "Error: Donor #" << donorId
                 << " is not registered. Donation rejected.\n";
            return false;
        }

        string cleanFood = trim(foodName);

        if (cleanFood.empty()) {
            cout << "Error: Food name cannot be empty.\n";
            return false;
        }

        if (expiryDays < 0) {
            cout << "Error: Expiry days cannot be negative.\n";
            return false;
        }

        if (foodType != "Veg" && foodType != "Non-Veg") {
            cout << "Error: Food type must be Veg or Non-Veg.\n";
            return false;
        }

        string pickupAddress;

        for (const auto &d : donors) {
            if (d.getId() == donorId) {
                pickupAddress = d.getAddress();
                break;
            }
        }

        FoodItem item{
            nextFoodId++,
            donorId,
            expiryDays,
            cleanFood,
            trim(allergens),
            foodType,
            pickupAddress
        };

        addToFoodInventory(item);

        string key = toLower(cleanFood);
        foodQueuesByName[key].push(item);

        addToAllergyHash(item);

        cout << "-> Food added to linked-list inventory.\n";
        cout << "-> Added to expiry priority queue.\n";
        cout << "-> Added to allergy hash table.\n";
        cout << "-> Pickup Address: " << pickupAddress << "\n";

        return true;
    }

    bool requestFood(int ngoId, const string &foodName) {
        // REGISTRATION CHECK
        if (!ngoExists(ngoId)) {
            cout << "Error: NGO #" << ngoId
                 << " is not registered. Request rejected.\n";
            return false;
        }

        string cleanFood = trim(foodName);

        if (cleanFood.empty()) {
            cout << "Error: Requested food name cannot be empty.\n";
            return false;
        }

        requestQueue.push(
            {nextRequestId++, ngoId, cleanFood}
        );

        cout << "-> Request accepted and added to FIFO queue.\n";
        return true;
    }

    void matchAll() {
        if (requestQueue.empty()) {
            cout << "No pending requests.\n";
            return;
        }

        bool progress = true;

        while (progress && !requestQueue.empty()) {
            progress = false;

            int rounds = static_cast<int>(requestQueue.size());

            for (int i = 0; i < rounds; ++i) {
                FoodRequest req = requestQueue.front();
                requestQueue.pop();

                if (tryMatch(req)) {
                    progress = true;
                } else {
                    requestQueue.push(req);
                }
            }
        }

        if (!requestQueue.empty()) {
            cout << "\nSome requests remain pending because no suitable "
                    "food is currently available.\n";
        }
    }

    void showAllDonors() const {
        cout << "\n--- Donors ---\n";

        if (donors.empty()) {
            cout << "  (none registered yet)\n";
            return;
        }

        for (const auto &d : donors)
            d.display();
    }

    void showAllNGOs() const {
        cout << "\n--- NGOs (Recipients) ---\n";

        if (ngos.empty()) {
            cout << "  (none registered yet)\n";
            return;
        }

        for (const auto &n : ngos)
            n.display();
    }

    void showFoodInventoryPublic() {
        showFoodInventory();
    }

    void lookupAllergyPublic(const string &allergen) {
        allergyLookup(allergen);
    }

    void showFoodQueue() const {
        cout << "\n--- Food Priority Queues (expiry priority) ---\n";

        if (foodQueuesByName.empty()) {
            cout << "  (empty)\n";
            return;
        }

        for (const auto &[name, pq] : foodQueuesByName) {
            auto copy = pq;

            if (copy.empty())
                continue;

            cout << "  [" << name << "]\n";

            while (!copy.empty()) {
                cout << "    ";
                copy.top().display();
                copy.pop();
            }
        }
    }

    void showRequestQueue() const {
        cout << "\n--- NGO Request Queue (FIFO) ---\n";

        auto copy = requestQueue;

        if (copy.empty()) {
            cout << "  (empty)\n";
            return;
        }

        while (!copy.empty()) {
            copy.front().display();
            copy.pop();
        }
    }

    void showGraph() const {
        cout << "\n--- Donor -> NGO Graph ---\n";

        if (donorToNgo.empty()) {
            cout << "  (no matches yet)\n";
            return;
        }

        for (const auto &[donorId, ngoIds] : donorToNgo) {
            cout << "  Donor#" << donorId << " helped NGO(s): ";

            for (int ngoId : ngoIds)
                cout << "#" << ngoId << " ";

            cout << endl;
        }
    }

    void showHistory() const {
        cout << "\n--- Distribution History ---\n";

        if (history.empty()) {
            cout << "  (none yet)\n";
            return;
        }

        for (const auto &line : history)
            cout << "  " << line << endl;
    }

    ~FoodSurplusManagementSystem() {
        clearFoodInventory();
    }
};

// ---------------- MAIN ----------------
int main() {
    FoodSurplusManagementSystem system;
    int choice;

    do {
        cout << "\n===== FOOD SURPLUS MANAGEMENT SYSTEM (Phase 2) =====\n"
             << "1. Register Donor\n"
             << "2. Donate Food\n"
             << "3. Register NGO\n"
             << "4. Request Food\n"
             << "5. Match all pending food/requests\n"
             << "6. Show Food Inventory (Linked List)\n"
             << "7. Show Food Priority Queue (Min-Heap)\n"
             << "8. Show Request Queue (FIFO)\n"
             << "9. Show Donor-NGO Graph\n"
             << "10. Show Distribution History\n"
             << "11. Allergy Lookup (Hash Table)\n"
             << "12. Show All Donors\n"
             << "13. Show All NGOs / Recipients\n"
             << "0. Exit\n";

        choice = readInt("Choice: ");

        string name, address, food, allergens, requirements, foodType;
        int id, days;

        switch (choice) {
        case 1: {
            cout << "Donor name: ";
            getline(cin, name);

            cout << "Donor pickup address: ";
            getline(cin, address);

            int newId = system.registerDonor(name, address);

            if (newId != -1)
                cout << "Registered! Your Donor ID = "
                     << newId << endl;

            break;
        }

        case 2: {
            id = readInt("Your Donor ID: ");

            // DO NOT accept arbitrary IDs.
            if (!system.donorExists(id)) {
                cout << "Error: Donor #" << id
                     << " is not registered. Donation rejected.\n";
                break;
            }

            cout << "Food item name: ";
            getline(cin, food);

            days = readNonNegativeInt(
                "Expires in how many days: "
            );

            while (true) {
                cout << "Food type (1 = Veg, 2 = Non-Veg): ";
                string typeChoice;
                getline(cin, typeChoice);

                if (typeChoice == "1") {
                    foodType = "Veg";
                    break;
                }

                if (typeChoice == "2") {
                    foodType = "Non-Veg";
                    break;
                }

                cout << "Please choose 1 or 2.\n";
            }

            cout << "Allergens (comma-separated, or 'none'): ";
            getline(cin, allergens);

            system.donateFood(
                id, food, days, allergens, foodType
            );

            break;
        }

        case 3: {
            cout << "NGO name: ";
            getline(cin, name);

            cout << "Food requirements "
                    "(comma-separated, or 'none'): ";
            getline(cin, requirements);

            cout << "Allergy restrictions - food this NGO "
                    "CANNOT accept "
                    "(comma-separated, or 'none'): ";
            getline(cin, allergens);

            int newId = system.registerNGO(
                name, requirements, allergens
            );

            if (newId != -1)
                cout << "Registered! Your NGO ID = "
                     << newId << endl;

            break;
        }

        case 4: {
            id = readInt("Your NGO ID: ");

            // DO NOT accept arbitrary IDs.
            if (!system.ngoExists(id)) {
                cout << "Error: NGO #" << id
                     << " is not registered. Request rejected.\n";
                break;
            }

            cout << "Food item needed: ";
            getline(cin, food);

            system.requestFood(id, food);
            break;
        }

        case 5:
            system.matchAll();
            break;

        case 6:
            system.showFoodInventoryPublic();
            break;

        case 7:
            system.showFoodQueue();
            break;

        case 8:
            system.showRequestQueue();
            break;

        case 9:
            system.showGraph();
            break;

        case 10:
            system.showHistory();
            break;

        case 11:
            cout << "Enter allergen to search: ";
            getline(cin, allergens);
            system.lookupAllergyPublic(allergens);
            break;

        case 12:
            system.showAllDonors();
            break;

        case 13:
            system.showAllNGOs();
            break;

        case 0:
            cout << "Exiting...\n";
            break;

        default:
            cout << "Invalid menu choice.\n";
        }

    } while (choice != 0);

    return 0;
}
