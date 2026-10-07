#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <algorithm>
#include <sstream>
#include <limits>
#include <cctype>
using namespace std;

// ================= INPUT / STRING HELPERS =================
int readInt(const string &prompt) {
    int value;
    while (true) {
        cout << prompt;
        if (cin >> value) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        }
        cout << "Invalid input - enter a whole number.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

int readNonNegativeInt(const string &prompt) {
    while (true) {
        int v = readInt(prompt);
        if (v >= 0) return v;
        cout << "Value cannot be negative.\n";
    }
}

string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(),
              [](unsigned char c) {
                  return static_cast<char>(tolower(c));
              });
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
        string token = toLower(trim(part));
        if (token.empty() || token == "none") continue;

        if (find(tokens.begin(), tokens.end(), token) == tokens.end())
            tokens.push_back(token);
    }
    return tokens;
}

// ================= OOP CLASSES =================
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
        cout << "Donor #" << id
             << " - " << name
             << " | Address: " << address << endl;
    }
};

class NGO : public User {
    string requirements;          // acceptable food names/options
    string dietaryPreference;     // Any / Veg / Non-Veg
    string allergyRestrictions;   // allergens this NGO cannot accept

public:
    NGO(int id, const string &name,
        const string &requirements,
        const string &dietaryPreference,
        const string &allergyRestrictions)
        : User(id, name),
          requirements(requirements),
          dietaryPreference(dietaryPreference),
          allergyRestrictions(allergyRestrictions) {}

    string getRequirements() const { return requirements; }
    string getDietaryPreference() const { return dietaryPreference; }
    string getAllergyRestrictions() const { return allergyRestrictions; }

    void display() const override {
        cout << "NGO #" << id
             << " - " << name
             << " | Needs: "
             << (requirements.empty() ? "(any)" : requirements)
             << " | Diet: " << dietaryPreference
             << " | Cannot accept: "
             << (allergyRestrictions.empty() ? "(none)" : allergyRestrictions)
             << endl;
    }
};

// ================= FOOD + REQUEST =================
struct FoodItem {
    int id;
    int donorId;
    int expiryDays;
    string foodName;
    string allergens;
    string foodType;
    string pickupAddress;

    void display() const {
        cout << "FoodItem#" << id
             << " [" << foodName << "]"
             << " | Type: " << foodType
             << " | Expires in: " << expiryDays << " day(s)"
             << " | Donor#" << donorId
             << " | Pickup: " << pickupAddress
             << " | Allergens: "
             << (allergens.empty() ? "none" : allergens)
             << endl;
    }
};

struct FoodNode {
    FoodItem item;
    FoodNode *next;

    FoodNode(const FoodItem &item) : item(item), next(nullptr) {}
};

struct FoodRequest {
    int id;
    int ngoId;
    string foodName;

    void display() const {
        cout << "Request#" << id
             << " [" << foodName
             << "] by NGO#" << ngoId << endl;
    }
};

// ================= CUSTOM MIN HEAP =================
// Global heap: earliest expiry has highest priority.
class FoodMinHeap {
private:
    FoodItem *heap;
    int size;
    int capacity;

    bool comesBefore(const FoodItem &a, const FoodItem &b) const {
        if (a.expiryDays != b.expiryDays)
            return a.expiryDays < b.expiryDays;
        return a.id < b.id;
    }

    void swapItems(FoodItem &a, FoodItem &b) {
        FoodItem temp = a;
        a = b;
        b = temp;
    }

    void heapifyUp(int index) {
        while (index > 0) {
            int parent = (index - 1) / 2;

            if (!comesBefore(heap[index], heap[parent]))
                break;

            swapItems(heap[index], heap[parent]);
            index = parent;
        }
    }

    void heapifyDown(int index) {
        while (true) {
            int left = 2 * index + 1;
            int right = 2 * index + 2;
            int smallest = index;

            if (left < size && comesBefore(heap[left], heap[smallest]))
                smallest = left;

            if (right < size && comesBefore(heap[right], heap[smallest]))
                smallest = right;

            if (smallest == index)
                break;

            swapItems(heap[index], heap[smallest]);
            index = smallest;
        }
    }

    void resize() {
        int newCapacity = capacity * 2;
        FoodItem *newHeap = new FoodItem[newCapacity];

        for (int i = 0; i < size; ++i)
            newHeap[i] = heap[i];

        delete[] heap;
        heap = newHeap;
        capacity = newCapacity;
    }

public:
    explicit FoodMinHeap(int initialCapacity = 50)
        : heap(new FoodItem[initialCapacity]),
          size(0),
          capacity(initialCapacity) {}

    FoodMinHeap(const FoodMinHeap &) = delete;
    FoodMinHeap &operator=(const FoodMinHeap &) = delete;

    ~FoodMinHeap() {
        delete[] heap;
    }

    bool empty() const { return size == 0; }
    int getSize() const { return size; }

    void push(const FoodItem &item) {
        if (size == capacity)
            resize();

        heap[size] = item;
        heapifyUp(size);
        ++size;
    }

    FoodItem top() const {
        return heap[0];
    }

    FoodItem pop() {
        FoodItem result = heap[0];
        --size;

        if (size > 0) {
            heap[0] = heap[size];
            heapifyDown(0);
        }

        return result;
    }

    void clear() {
        size = 0;
    }

    void display() const {
        if (size == 0) {
            cout << "  (empty)\n";
            return;
        }

        // Copy to a temporary heap so displaying does not destroy the real heap.
        FoodMinHeap copy(size + 1);
        for (int i = 0; i < size; ++i)
            copy.push(heap[i]);

        while (!copy.empty()) {
            cout << "  ";
            copy.pop().display();
        }
    }
};

// ================= CUSTOM HASH TABLE =================
// Separate chaining: allergen -> linked list of food IDs.
struct AllergyNode {
    string allergen;
    int foodId;
    AllergyNode *next;

    AllergyNode(const string &allergen, int foodId)
        : allergen(allergen), foodId(foodId), next(nullptr) {}
};

class AllergyHashTable {
private:
    static const int TABLE_SIZE = 101;
    AllergyNode *buckets[TABLE_SIZE];

    int hashFunction(const string &key) const {
        unsigned long hash = 5381;

        for (unsigned char c : key)
            hash = ((hash << 5) + hash) + c;

        return static_cast<int>(hash % TABLE_SIZE);
    }

public:
    AllergyHashTable() {
        for (int i = 0; i < TABLE_SIZE; ++i)
            buckets[i] = nullptr;
    }

    AllergyHashTable(const AllergyHashTable &) = delete;
    AllergyHashTable &operator=(const AllergyHashTable &) = delete;

    void insert(const string &allergen, int foodId) {
        string key = toLower(trim(allergen));
        if (key.empty() || key == "none")
            return;

        int index = hashFunction(key);

        AllergyNode *current = buckets[index];
        while (current) {
            if (current->allergen == key && current->foodId == foodId)
                return;
            current = current->next;
        }

        AllergyNode *node = new AllergyNode(key, foodId);
        node->next = buckets[index];
        buckets[index] = node;
    }

    bool containsFood(const string &allergen, int foodId) const {
        string key = toLower(trim(allergen));
        int index = hashFunction(key);

        AllergyNode *current = buckets[index];

        while (current) {
            if (current->allergen == key && current->foodId == foodId)
                return true;
            current = current->next;
        }

        return false;
    }

    void remove(const string &allergen, int foodId) {
        string key = toLower(trim(allergen));
        int index = hashFunction(key);

        AllergyNode *current = buckets[index];
        AllergyNode *previous = nullptr;

        while (current) {
            if (current->allergen == key && current->foodId == foodId) {
                if (previous)
                    previous->next = current->next;
                else
                    buckets[index] = current->next;

                delete current;
                return;
            }

            previous = current;
            current = current->next;
        }
    }

    void lookup(const string &allergen) const {
        string key = toLower(trim(allergen));

        if (key.empty() || key == "none") {
            cout << "Please enter a valid allergen.\n";
            return;
        }

        int index = hashFunction(key);
        AllergyNode *current = buckets[index];
        bool found = false;

        cout << "\n--- Allergy Hash Table Lookup ---\n";
        cout << "Allergen: " << key << "\n";
        cout << "Food IDs: ";

        while (current) {
            if (current->allergen == key) {
                cout << "#" << current->foodId << " ";
                found = true;
            }
            current = current->next;
        }

        if (!found)
            cout << "none";

        cout << endl;
    }

    ~AllergyHashTable() {
        for (int i = 0; i < TABLE_SIZE; ++i) {
            AllergyNode *current = buckets[i];

            while (current) {
                AllergyNode *temp = current;
                current = current->next;
                delete temp;
            }

            buckets[i] = nullptr;
        }
    }
};

// ================= MAIN SYSTEM =================
class FoodSurplusManagementSystem {
private:
    vector<Donor> donors;
    vector<NGO> ngos;

    // Complete active inventory.
    FoodNode *foodHead = nullptr;

    // ONE GLOBAL custom min-heap.
    FoodMinHeap foodMinHeap;

    // Custom allergen hash table.
    AllergyHashTable allergyHashTable;

    // FIFO request queue.
    queue<FoodRequest> requestQueue;

    vector<string> history;

    // Graph represented as donor -> NGO adjacency list.
    map<int, vector<int>> donorToNgo;

    int nextUserId = 1;
    int nextFoodId = 1;
    int nextRequestId = 1;

    void addToFoodInventory(const FoodItem &item) {
        FoodNode *node = new FoodNode(item);

        if (!foodHead) {
            foodHead = node;
            return;
        }

        FoodNode *current = foodHead;
        while (current->next)
            current = current->next;

        current->next = node;
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

    void addToAllergyHash(const FoodItem &item) {
        for (const string &allergen : splitTokens(item.allergens))
            allergyHashTable.insert(allergen, item.id);
    }

    void removeFromAllergyHash(const FoodItem &item) {
        for (const string &allergen : splitTokens(item.allergens))
            allergyHashTable.remove(allergen, item.id);
    }

    NGO *findNgoById(int ngoId) {
        for (auto &ngo : ngos)
            if (ngo.getId() == ngoId)
                return &ngo;

        return nullptr;
    }

    bool meetsFoodRequirement(const FoodItem &food,
                              const NGO &ngo) const {
        vector<string> requirements =
            splitTokens(ngo.getRequirements());

        if (requirements.empty())
            return true;

        string foodName = toLower(trim(food.foodName));

        for (const string &required : requirements) {
            if (foodName == required)
                return true;
        }

        return false;
    }

    bool meetsDietaryPreference(const FoodItem &food,
                                const NGO &ngo) const {
        string preference =
            toLower(trim(ngo.getDietaryPreference()));

        if (preference.empty() || preference == "any")
            return true;

        if (preference == "veg" ||
            preference == "vegetarian")
            return food.foodType == "Veg";

        if (preference == "non-veg" ||
            preference == "nonveg" ||
            preference == "non vegetarian" ||
            preference == "non-vegetarian")
            return food.foodType == "Non-Veg";

        return false;
    }

    // Uses the CUSTOM HASH TABLE for the actual allergy gate.
    bool isAllergySafe(const FoodItem &food,
                       const NGO &ngo) const {
        vector<string> restrictions =
            splitTokens(ngo.getAllergyRestrictions());

        for (const string &restricted : restrictions) {
            if (allergyHashTable.containsFood(restricted, food.id))
                return false;
        }

        return true;
    }

    bool suitable(const FoodItem &food, const NGO &ngo,
                  const string &requestedFood) const {
        if (toLower(trim(food.foodName)) !=
            toLower(trim(requestedFood)))
            return false;

        if (!meetsFoodRequirement(food, ngo))
            return false;

        if (!meetsDietaryPreference(food, ngo))
            return false;

        if (!isAllergySafe(food, ngo))
            return false;

        return true;
    }

    bool tryMatch(const FoodRequest &request) {
        NGO *ngo = findNgoById(request.ngoId);

        if (!ngo) {
            cout << "MATCH REJECTED: NGO #"
                 << request.ngoId
                 << " is not registered.\n";
            return false;
        }

        string requestedFood = trim(request.foodName);

        if (requestedFood.empty()) {
            cout << "MATCH REJECTED: Food name is empty.\n";
            return false;
        }

        // Global heap: inspect items in expiry order.
        FoodMinHeap temporary;
        FoodItem matched{};
        bool found = false;

        while (!foodMinHeap.empty()) {
            FoodItem candidate = foodMinHeap.pop();

            if (!found &&
                suitable(candidate, *ngo, requestedFood)) {
                matched = candidate;
                found = true;

                // Do NOT put matched food back.
                break;
            }

            temporary.push(candidate);
        }

        // Restore every active item that was not matched.
        while (!temporary.empty())
            foodMinHeap.push(temporary.pop());

        if (!found) {
            cout << "No suitable food found for NGO#"
                 << request.ngoId
                 << ". Request remains pending.\n";
            return false;
        }

        // Remove matched food from linked-list inventory.
        FoodItem removed;
        if (!removeFromFoodInventory(matched.id, removed)) {
            // Roll back heap state if inventory is inconsistent.
            foodMinHeap.push(matched);
            cout << "ERROR: Inventory inconsistency. Match cancelled.\n";
            return false;
        }

        // Remove matched food from custom allergen hash.
        removeFromAllergyHash(matched);

        cout << "\nMATCHED: " << matched.foodName
             << " [" << matched.foodType << "]"
             << " (Food#" << matched.id
             << ", Donor#" << matched.donorId
             << ") -> NGO#" << request.ngoId << endl;

        cout << "PICKUP ADDRESS: "
             << matched.pickupAddress << endl;

        donorToNgo[matched.donorId].push_back(request.ngoId);

        history.push_back(
            "Donor#" + to_string(matched.donorId) +
            " -> NGO#" + to_string(request.ngoId) +
            " (" + matched.foodName + ", " +
            matched.foodType + ") | Pickup: " +
            matched.pickupAddress
        );

        return true;
    }

    void clearFoodInventory() {
        while (foodHead) {
            FoodNode *temp = foodHead;
            foodHead = foodHead->next;
            delete temp;
        }
    }

public:
    FoodSurplusManagementSystem() = default;

    FoodSurplusManagementSystem(
        const FoodSurplusManagementSystem &) = delete;

    FoodSurplusManagementSystem &operator=(
        const FoodSurplusManagementSystem &) = delete;

    bool donorExists(int donorId) const {
        for (const auto &donor : donors)
            if (donor.getId() == donorId)
                return true;

        return false;
    }

    bool ngoExists(int ngoId) const {
        for (const auto &ngo : ngos)
            if (ngo.getId() == ngoId)
                return true;
        return false;
    }

    int registerDonor(const string &name,
                      const string &address) {
        string cleanName = trim(name);
        string cleanAddress = trim(address);

        if (cleanName.empty() || cleanAddress.empty()) {
            cout << "Error: Donor name/address cannot be empty.\n";
            return -1;
        }

        donors.emplace_back(
            nextUserId, cleanName, cleanAddress
        );

        return nextUserId++;
    }

    int registerNGO(const string &name,
                    const string &requirements,
                    const string &dietaryPreference,
                    const string &allergyRestrictions) {
        string cleanName = trim(name);
        string diet = toLower(trim(dietaryPreference));
        string normalizedDiet;

        if (cleanName.empty()) {
            cout << "Error: NGO name cannot be empty.\n";
            return -1;
        }

        if (diet != "any" &&
            diet != "veg" &&
            diet != "non-veg" &&
            diet != "nonveg") {
            cout << "Error: Dietary preference must be "
                    "Any, Veg, or Non-Veg.\n";
            return -1;
        }

        if (diet == "nonveg")
            normalizedDiet = "Non-Veg";
        else if (diet == "veg")
            normalizedDiet = "Veg";
        else
            normalizedDiet = "Any";

        ngos.emplace_back(
            nextUserId,
            cleanName,
            trim(requirements),
            normalizedDiet,
            trim(allergyRestrictions)
        );

        return nextUserId++;
    }

    bool donateFood(int donorId,
                    const string &foodName,
                    int expiryDays,
                    const string &allergens,
                    const string &foodType) {
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

        if (foodType != "Veg" &&
            foodType != "Non-Veg") {
            cout << "Error: Food type must be Veg or Non-Veg.\n";
            return false;
        }

        string pickupAddress;

        for (const auto &donor : donors) {
            if (donor.getId() == donorId) {
                pickupAddress = donor.getAddress();
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
        foodMinHeap.push(item);
        addToAllergyHash(item);

        cout << "-> Added to Linked List inventory.\n";
        cout << "-> Added to GLOBAL expiry Min-Heap.\n";
        cout << "-> Added to CUSTOM allergy Hash Table.\n";
        cout << "-> Pickup Address: "
             << pickupAddress << endl;

        return true;
    }

    bool requestFood(int ngoId,
                     const string &foodName) {
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

        cout << "-> Request added to FIFO queue.\n";
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

            int rounds =
                static_cast<int>(requestQueue.size());

            for (int i = 0; i < rounds; ++i) {
                FoodRequest request =
                    requestQueue.front();

                requestQueue.pop();

                if (tryMatch(request)) {
                    progress = true;
                } else {
                    requestQueue.push(request);
                }
            }
        }

        if (!requestQueue.empty()) {
            cout << "\nSome requests remain pending because "
                    "no suitable food is currently available.\n";
        }
    }

    void showAllDonors() const {
        cout << "\n--- Donors ---\n";

        if (donors.empty()) {
            cout << "  (none registered yet)\n";
            return;
        }

        for (const auto &donor : donors)
            donor.display();
    }

    void showAllNGOs() const {
        cout << "\n--- NGOs / Recipients ---\n";

        if (ngos.empty()) {
            cout << "  (none registered yet)\n";
            return;
        }

        for (const auto &ngo : ngos)
            ngo.display();
    }

    void showFoodInventory() const {
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

    void showFoodHeap() const {
        cout << "\n--- GLOBAL Food Min-Heap (Expiry Priority) ---\n";
        foodMinHeap.display();
    }

    void showRequestQueue() const {
        cout << "\n--- NGO Request Queue (FIFO) ---\n";

        queue<FoodRequest> copy = requestQueue;

        if (copy.empty()) {
            cout << "  (empty)\n";
            return;
        }

        while (!copy.empty()) {
            copy.front().display();
            copy.pop();
        }
    }

    void lookupAllergy(const string &allergen) const {
        allergyHashTable.lookup(allergen);
    }

    void showGraph() const {
        cout << "\n--- Donor -> NGO Graph ---\n";

        if (donorToNgo.empty()) {
            cout << "  (no matches yet)\n";
            return;
        }

        for (const auto &entry : donorToNgo) {
            cout << "  Donor#" << entry.first
                 << " helped NGO(s): ";

            for (int ngoId : entry.second)
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

        for (const string &line : history)
            cout << "  " << line << endl;
    }

    ~FoodSurplusManagementSystem() {
        clearFoodInventory();
    }
};

// ================= MAIN =================
int main() {
    FoodSurplusManagementSystem system;
    int choice;

    do {
        cout << "\n===== FOOD SURPLUS MANAGEMENT SYSTEM (Phase 2) =====\n"
             << "1. Register Donor\n"
             << "2. Donate Food\n"
             << "3. Register NGO / Recipient\n"
             << "4. Request Food\n"
             << "5. Match All Pending Requests\n"
             << "6. Show Food Inventory (Linked List)\n"
             << "7. Show GLOBAL Food Min-Heap\n"
             << "8. Show Request Queue (FIFO)\n"
             << "9. Show Donor-NGO Graph\n"
             << "10. Show Distribution History\n"
             << "11. Allergy Lookup (Custom Hash Table)\n"
             << "12. Show All Donors\n"
             << "13. Show All NGOs / Recipients\n"
             << "0. Exit\n";

        choice = readInt("Choice: ");

        string name, address, food, allergens;
        string requirements, dietaryPreference, foodType;
        int id, days;

        switch (choice) {
        case 1: {
            cout << "Donor name: ";
            getline(cin, name);

            cout << "Donor pickup address: ";
            getline(cin, address);

            int newId =
                system.registerDonor(name, address);

            if (newId != -1)
                cout << "Registered! Donor ID = "
                     << newId << endl;

            break;
        }

        case 2: {
            id = readInt("Your Donor ID: ");

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
                string type;
                getline(cin, type);

                if (type == "1") {
                    foodType = "Veg";
                    break;
                }

                if (type == "2") {
                    foodType = "Non-Veg";
                    break;
                }

                cout << "Please choose 1 or 2.\n";
            }

            cout << "Allergens "
                    "(comma-separated, or 'none'): ";
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

            while (true) {
                cout << "Dietary preference "
                        "(1 = Any, 2 = Veg, 3 = Non-Veg): ";
                string choiceDiet;
                getline(cin, choiceDiet);

                if (choiceDiet == "1") {
                    dietaryPreference = "Any";
                    break;
                }

                if (choiceDiet == "2") {
                    dietaryPreference = "Veg";
                    break;
                }

                if (choiceDiet == "3") {
                    dietaryPreference = "Non-Veg";
                    break;
                }

                cout << "Please choose 1, 2, or 3.\n";
            }

            cout << "Allergy restrictions - food this NGO "
                    "CANNOT accept "
                    "(comma-separated, or 'none'): ";
            getline(cin, allergens);

            int newId = system.registerNGO(
                name,
                requirements,
                dietaryPreference,
                allergens
            );

            if (newId != -1)
                cout << "Registered! NGO ID = "
                     << newId << endl;

            break;
        }

        case 4: {
            id = readInt("Your NGO ID: ");

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
            system.showFoodInventory();
            break;

        case 7:
            system.showFoodHeap();
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
            system.lookupAllergy(allergens);
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
