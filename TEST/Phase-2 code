#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <limits>
#include <vector>
#include <queue>
#include <cctype>


using namespace std;
using namespace std::chrono;

// Forward declaration
class FoodRescueSystem;

// ============================================================================
// 1. INPUT VALIDATION & REAL-TIME HELPERS
// ============================================================================
int readInt(const string &prompt) {
    int value;
    while (true) {
        cout << prompt;
        if (cin >> value) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        }
        cout << "[Error] Invalid input. Enter a valid number.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

int readMenuChoice(const string &prompt, int minVal, int maxVal) {
    while (true) {
        int choice = readInt(prompt);
        if ((choice >= minVal && choice <= maxVal) || choice == 999) {
            return choice;
        }
        cout << "[Error] Choice out of range. Enter a number between " << minVal << " and " << maxVal << ".\n";
    }
}

int readPositiveInt(const string &prompt) {
    while (true) {
        int v = readInt(prompt);
        if (v > 0) return v;
        cout << "[Error] Value must be greater than 0.\n";
    }
}

string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == string::npos) return "";
    return s.substr(start, end - start + 1);
}

string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

string readNonEmptyLine(const string &prompt) {
    string s;
    while (true) {
        cout << prompt;
        getline(cin, s);
        s = trim(s);
        if (!s.empty()) return s;
        cout << "[Error] Input cannot be blank. Enter valid text.\n";
    }
}

vector<string> splitTokens(const string &str) {
    vector<string> tokens;
    stringstream ss(str);
    string token;
    while (getline(ss, token, ',')) {
        string t = toLower(trim(token));
        if (!t.empty() && t != "none") {
            tokens.push_back(t);
        }
    }
    return tokens;
}

long long getCurrentEpochSeconds() {
    return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

// ============================================================================
// 2. CORE DOMAIN SCHEMAS
// ============================================================================
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

    FoodItem() : foodID(0), name(""), category(""), quantity(0), prepTimestamp(0),
                 expiryTimestamp(0), shelfHours(0), dietType(""), ingredients(""),
                 allergens(""), spiceLevel(""), extraNotes(""), donorID(0),
                 donorAddress(""), isAssigned(false),
                 isCancelled(false), isExpired(false) {}

    FoodItem(int id, string n, string c, int q, long long pTime, long long eTime,
             int sHrs, string dType, string ing, string alg, string spice,
             string notes, int dId, string addr)
        : foodID(id), name(n), category(c), quantity(q), prepTimestamp(pTime),
          expiryTimestamp(eTime), shelfHours(sHrs), dietType(dType), ingredients(ing),
          allergens(alg), spiceLevel(spice), extraNotes(notes), donorID(dId),
          donorAddress(addr), isAssigned(false),
          isCancelled(false), isExpired(false) {}

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
        cout << "Food #" << foodID << " [" << name << " | " << dietType << " | Spice: " << spiceLevel << "]\n"
             << "       Qty: " << quantity << " servings | Remainder: "
             << getRemainingHours() << " hr(s) (" << getRemainingMinutes() << " mins)\n"
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

    OrderTransaction(int oId, int fId, int dId, int rId, int qty, double cost, string stat)
        : orderID(oId), foodID(fId), donorID(dId), recipientID(rId), quantity(qty),
          deliveryCost(cost), status(stat), donorConfirmed(false),
          recipientConfirmed(false), isSpoiledComplaint(false) {}
};

// ============================================================================
// 3. DATA STRUCTURE #1: CUSTOM SEPARATE-CHAINING HASH TABLE
// ============================================================================
class HashTable {
private:
    struct HashNode {
        string key;
        string value;
        HashNode *next;
        HashNode(string k, string v) : key(k), value(v), next(nullptr) {}
    };

    static const int TABLE_SIZE = 101;
    HashNode *table[TABLE_SIZE];

    int hashFunction(const string &key) const {
        unsigned long hash = 5381;
        for (unsigned char c : key) hash = ((hash << 5) + hash) + tolower(c);
        return hash % TABLE_SIZE;
    }

public:
    HashTable() {
        for (int i = 0; i < TABLE_SIZE; ++i) table[i] = nullptr;
    }

    ~HashTable() {
        for (int i = 0; i < TABLE_SIZE; ++i) {
            HashNode *entry = table[i];
            while (entry) {
                HashNode *prev = entry;
                entry = entry->next;
                delete prev;
            }
            table[i] = nullptr;
        }
    }

    void insert(const string &key, const string &value) {
        string k = toLower(trim(key));
        int idx = hashFunction(k);
        HashNode *entry = table[idx];
        while (entry) {
            if (entry->key == k) {
                entry->value = value;
                return;
            }
            entry = entry->next;
        }
        HashNode *newNode = new HashNode(k, value);
        newNode->next = table[idx];
        table[idx] = newNode;
    }

    string search(const string &key) const {
        string k = toLower(trim(key));
        int idx = hashFunction(k);
        HashNode *entry = table[idx];
        while (entry) {
            if (entry->key == k) return entry->value;
            entry = entry->next;
        }
        return "";
    }

    string mapIngredientsToAllergens(const string &rawIngredients) const {
        vector<string> tokens = splitTokens(rawIngredients);
        string detectedAllergens = "";

        for (const auto &tok : tokens) {
            string allergen = search(tok);
            if (!allergen.empty()) {
                if (detectedAllergens.find(allergen) == string::npos) {
                    if (!detectedAllergens.empty()) detectedAllergens += ", ";
                    detectedAllergens += allergen;
                }
            }
        }
        return detectedAllergens;
    }
};

// ============================================================================
// 4. DATA STRUCTURE #2: CUSTOM SINGLY LINKED LIST (INVENTORY)
// ============================================================================
class FoodInventoryList {
private:
    struct Node {
        FoodItem data;
        Node *next;
        Node(const FoodItem &f) : data(f), next(nullptr) {}
    };
    Node *head;

public:
    FoodInventoryList() : head(nullptr) {}

    ~FoodInventoryList() {
        clear();
    }

    void clear() {
        Node *curr = head;
        while (curr) {
            Node *temp = curr;
            curr = curr->next;
            delete temp;
        }
        head = nullptr;
    }

    FoodItem* insert(const FoodItem &item) {
        Node *newNode = new Node(item);
        newNode->next = head;
        head = newNode;
        return &(newNode->data);
    }

    FoodItem* findById(int id) {
        Node *curr = head;
        while (curr) {
            if (curr->data.foodID == id) return &(curr->data);
            curr = curr->next;
        }
        return nullptr;
    }

    void displayAll() const {
        if (!head) {
            cout << "  (Inventory is empty)\n";
            return;
        }
        Node *curr = head;
        bool any = false;
        while (curr) {
            if (!curr->data.isAssigned && !curr->data.isCancelled && !curr->data.isExpired) {
                curr->data.display();
                any = true;
            }
            curr = curr->next;
        }
        if (!any) cout << "  (No active donation items available)\n";
    }

    template<typename Func>
    void forEach(Func f) {
        Node *curr = head;
        while (curr) {
            f(curr->data);
            curr = curr->next;
        }
    }
};

// ============================================================================
// 5. DATA STRUCTURE #3: CUSTOM BINARY MIN-HEAP (URGENCY RADAR)
// ============================================================================
class MinHeapExpiryQueue {
private:
    FoodItem** heapArray;
    int capacity;
    int currentSize;

    void heapifyUp(int index) {
        while (index > 0) {
            int parent = (index - 1) / 2;
            if (heapArray[index]->expiryTimestamp < heapArray[parent]->expiryTimestamp) {
                swap(heapArray[index], heapArray[parent]);
                index = parent;
            } else break;
        }
    }

    void heapifyDown(int index) {
        while (index < currentSize) {
            int left = 2 * index + 1;
            int right = 2 * index + 2;
            int smallest = index;

            if (left < currentSize && heapArray[left]->expiryTimestamp < heapArray[smallest]->expiryTimestamp)
                smallest = left;
            if (right < currentSize && heapArray[right]->expiryTimestamp < heapArray[smallest]->expiryTimestamp)
                smallest = right;

            if (smallest != index) {
                swap(heapArray[index], heapArray[smallest]);
                index = smallest;
            } else break;
        }
    }

public:
    MinHeapExpiryQueue(int cap = 100) : capacity(cap), currentSize(0) {
        heapArray = new FoodItem*[capacity];
        for (int i = 0; i < capacity; ++i) heapArray[i] = nullptr;
    }

    ~MinHeapExpiryQueue() {
        delete[] heapArray;
    }

    bool isEmpty() const { return currentSize == 0; }

    void insert(FoodItem *item) {
        if (!item) return;
        if (currentSize == capacity) {
            int newCap = capacity * 2;
            FoodItem** newArr = new FoodItem*[newCap];
            for (int i = 0; i < currentSize; ++i) {
                newArr[i] = heapArray[i];
            }
            for (int i = currentSize; i < newCap; ++i) {
                newArr[i] = nullptr;
            }
            delete[] heapArray;
            heapArray = newArr;
            capacity = newCap;
        }
        heapArray[currentSize] = item;
        heapifyUp(currentSize);
        currentSize++;
    }

    void displayHeap() const {
        if (isEmpty()) {
            cout << "  (Urgency Heap is empty)\n";
            return;
        }
        int rank = 1;
        for (int i = 0; i < currentSize; ++i) {
            if (heapArray[i] && !heapArray[i]->isAssigned && !heapArray[i]->isCancelled && !heapArray[i]->isExpired) {
                double mins = heapArray[i]->getRemainingMinutes();
                cout << "  [" << rank++ << "] Food #" << heapArray[i]->foodID
                     << " - " << heapArray[i]->name
                     << " (Remaining: " << heapArray[i]->getRemainingHours() << " hrs / "
                     << mins << " mins)";
                if (mins <= 30.0 && mins > 0.0) {
                    cout << " *** CRITICAL: PICKUP REQUIRED WITHIN 30 MINS! ***";
                }
                cout << "\n";
            }
        }
        if (rank == 1) cout << "  (No active items in urgency radar)\n";
    }
};

// ============================================================================
// 6. OBJECT-ORIENTED ENTITY TAXONOMY
// ============================================================================
class User {
protected:
    int id;
    string name;
    string contact;
    string address;
    int penaltyPoints;
    double penaltyFines;

public:
    User(int id, string name, string contact, string addr, int pts = 0, double fines = 0.0)
        : id(id), name(name), contact(contact), address(addr),
          penaltyPoints(pts), penaltyFines(fines) {}

    virtual ~User() = default;
    int getId() const { return id; }
    string getName() const { return name; }
    string getContact() const { return contact; }
    string getAddress() const { return address; }
    int getPenaltyPoints() const { return penaltyPoints; }
    double getPenaltyFines() const { return penaltyFines; }

    void addPenalty(double fine) {
        penaltyPoints++;
        penaltyFines += fine;
    }

    virtual void display() const {
        cout << "User #" << id << ": " << name << " | Contact: " << contact
             << " | Addr: " << address
             << " | Penalties: " << penaltyPoints << " (Rs. " << penaltyFines << ")\n";
    }
};

class Donor : public User {
private:
    string donorType;

public:
    Donor(int id, string name, string contact, string addr, string type, int pts = 0, double fines = 0.0)
        : User(id, name, contact, addr, pts, fines), donorType(type) {}

    string getDonorType() const { return donorType; }

    void display() const override {
        cout << "[Donor #" << id << "] " << name << " (" << donorType
             << ") | Addr: " << address
             << " | Ph: " << contact << " | Penalty Strikes: " << penaltyPoints << "\n";
    }
};

class Recipient : public User {
protected:
    string acceptedDietType;

public:
    Recipient(int id, string name, string contact, string addr, string diet, int pts = 0, double fines = 0.0)
        : User(id, name, contact, addr, pts, fines), acceptedDietType(toLower(trim(diet))) {}

    string getAcceptedDietType() const { return acceptedDietType; }

    virtual bool isCompatible(const FoodItem &food, string &rejectionReason) const = 0;
};

class NGO : public Recipient {
public:
    NGO(int id, string name, string contact, string addr, string diet, int pts = 0, double fines = 0.0)
        : Recipient(id, name, contact, addr, diet, pts, fines) {}

    bool isCompatible(const FoodItem &food, string &rejectionReason) const override {
        if (acceptedDietType == "veg" && toLower(food.dietType) == "non-veg") {
            rejectionReason = "Dietary mismatch (NGO requires strictly Vegetarian meals)";
            return false;
        }
        return true;
    }

    void display() const override {
        cout << "[NGO #" << id << "] " << name << " | Diet: " << acceptedDietType << "\n"
             << "       Addr: " << address << "\n";
    }
};

class AnimalShelter : public Recipient {
private:
    string species;

public:
    AnimalShelter(int id, string name, string contact, string addr, string spec, int pts = 0, double fines = 0.0)
        : Recipient(id, name, contact, addr, "plain", pts, fines), species(spec) {}

    string getSpecies() const { return species; }

    bool isCompatible(const FoodItem &food, string &rejectionReason) const override {
        if (toLower(food.dietType) != "plain") {
            rejectionReason = "Animal Safety Violation: Cooked human food contains unsafe spices/oil";
            return false;
        }

        string raw = toLower(food.ingredients);
        vector<string> toxicList = {"onion", "garlic", "chocolate", "xylitol", "chili", "pepper", "raisin", "grape"};

        for (const auto &tox : toxicList) {
            if (raw.find(tox) != string::npos) {
                rejectionReason = "TOXICITY ALERT: Contains " + tox + " which is lethal to animals!";
                return false;
            }
        }
        return true;
    }

    void display() const override {
        cout << "[Animal Shelter #" << id << "] " << name << " (" << species
             << ") | Addr: " << address << "\n";
    }
};

// ============================================================================
// 7. SYSTEM ORCHESTRATION ENGINE
// ============================================================================
class FoodRescueSystem {
private:
    HashTable allergenDictionary;
    FoodInventoryList inventory;
    MinHeapExpiryQueue urgencyHeap;
    queue<FoodRequest> requestQueue;

    vector<User*> userRegistry;
    vector<Notification> notifications;
    vector<OrderTransaction> orderHistory;

    int nextUserId = 1;
    int nextFoodId = 1;
    int nextRequestId = 1;
    int nextOrderId = 1;

    void seedAllergenDictionary() {
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

public:
    FoodRescueSystem() {
        seedAllergenDictionary();
        loadFromDisk();
        autoPurgeExpiredFood();
    }

    ~FoodRescueSystem() {
        autoPurgeExpiredFood();
        saveToDisk();
        for (User *u : userRegistry) delete u;
    }

    bool isPhoneRegistered(const string &phone) const {
        for (User *u : userRegistry) {
            if (u->getContact() == phone) return true;
        }
        return false;
    }

    void autoPurgeExpiredFood() {
        inventory.forEach([&](FoodItem &item) {
            if (!item.isAssigned && !item.isCancelled && !item.isExpired) {
                if (item.getRemainingMinutes() <= 0.0) {
                    item.isExpired = true;
                    string alertMsg = "[AUTOMATIC EXPIRY NOTICE] Donation Food #" +
                                      to_string(item.foodID) + " (" + item.name +
                                      ") expired without recipient requests. Automatically marked EXPIRED and removed from donation radar.";
                    addNotification(item.donorID, alertMsg);
                }
            }
        });
    }

    User* findUser(int id) const {
        for (User *u : userRegistry) {
            if (u->getId() == id) return u;
        }
        return nullptr;
    }

    Recipient* findRecipient(int id) const {
        User *u = findUser(id);
        return dynamic_cast<Recipient*>(u);
    }

    Donor* findDonor(int id) const {
        User *u = findUser(id);
        return dynamic_cast<Donor*>(u);
    }

    bool userExists(int id) const { return findUser(id) != nullptr; }
    bool recipientExists(int id) const { return findRecipient(id) != nullptr; }

    void addNotification(int uId, const string &msg) {
        notifications.push_back({uId, msg, getCurrentEpochSeconds()});
    }

    int registerDonor(string name, string contact, string addr, string type) {
        Donor *d = new Donor(nextUserId++, name, contact, addr, type);
        userRegistry.push_back(d);
        return d->getId();
    }

    int registerNGO(string name, string contact, string addr, string diet) {
        NGO *n = new NGO(nextUserId++, name, contact, addr, diet);
        userRegistry.push_back(n);
        return n->getId();
    }

    int registerAnimalShelter(string name, string contact, string addr, string species) {
        AnimalShelter *a = new AnimalShelter(nextUserId++, name, contact, addr, species);
        userRegistry.push_back(a);
        return a->getId();
    }

    int donateFood(int donorId, string name, string category, int qty, int bestBeforeHrs,
                   string dietType, string ingredients, string spice, string extraNotes) {
        autoPurgeExpiredFood();

        User *u = findUser(donorId);
        if (!u) {
            cout << "[Error] Donor/Entity #" << donorId << " is not registered.\n";
            return -1;
        }

        string identifiedAllergens = allergenDictionary.mapIngredientsToAllergens(ingredients);

        long long prepTime = getCurrentEpochSeconds();
        long long expiryTime = prepTime + (bestBeforeHrs * 3600LL);

        FoodItem item(nextFoodId++, name, category, qty, prepTime, expiryTime,
                      bestBeforeHrs, dietType, ingredients, identifiedAllergens,
                      spice, extraNotes, donorId, u->getAddress());

        FoodItem* storedPtr = inventory.insert(item);
        urgencyHeap.insert(storedPtr);

        cout << "\n>>> Item added for donation successfully.\n";

        if (storedPtr->getRemainingMinutes() <= 30.0) {
            cout << ">>> [URGENT WINDOW ALERT] Food has 30 mins or less remaining! Flagged for quick pickup.\n";
        }

        addNotification(donorId, "Food #" + to_string(storedPtr->foodID) + " (" + name + ") successfully logged for donation.");

        processMatchingPipeline();

        return storedPtr->foodID;
    }

    int requestFood(int recipientId, string foodName, int qty, string allergies) {
        autoPurgeExpiredFood();

        Recipient *r = findRecipient(recipientId);
        if (!r) {
            cout << "[Error] Recipient #" << recipientId << " not found in registry.\n";
            return -1;
        }

        FoodRequest req{nextRequestId++, recipientId, foodName, qty, allergies, "Pending", getCurrentEpochSeconds()};
        requestQueue.push(req);
        cout << "\n>>> Request #" << req.requestID << " placed into queue.\n";
        addNotification(recipientId, "Request #" + to_string(req.requestID) + " for " + foodName + " placed into queue.");

        processMatchingPipeline();

        return req.requestID;
    }

    bool cancelDonation(int foodId, int donorId) {
        autoPurgeExpiredFood();
        FoodItem *item = inventory.findById(foodId);
        if (!item) {
            cout << "[Error] Food item not found.\n";
            return false;
        }
        if (item->donorID != donorId) {
            cout << "[Error] You do not own this donation.\n";
            return false;
        }
        if (item->isAssigned) {
            cout << "[Error] Cancellation window closed: Food has already been matched or dispatched.\n";
            return false;
        }
        if (item->isExpired) {
            cout << "[Error] Item has already expired.\n";
            return false;
        }
        item->isCancelled = true;
        addNotification(donorId, "Donation of Food #" + to_string(foodId) + " was cancelled successfully.");
        cout << ">>> Food donation cancelled successfully.\n";
        return true;
    }

    // ========================================================================
    // ADVANCED SMART MATCHING ENGINE
    // ========================================================================
    void processMatchingPipeline() {
        autoPurgeExpiredFood();

        if (requestQueue.empty()) return;

        vector<FoodRequest> pendingRequests;
        while (!requestQueue.empty()) {
            pendingRequests.push_back(requestQueue.front());
            requestQueue.pop();
        }

        for (size_t rIdx = 0; rIdx < pendingRequests.size(); ++rIdx) {
            FoodRequest &req = pendingRequests[rIdx];
            if (req.status != "Pending" || req.requestedQuantity <= 0) continue;

            Recipient *rec = findRecipient(req.recipientID);
            if (!rec) continue;

            vector<FoodItem*> compatibleLots;
            int totalAvailableQty = 0;

            inventory.forEach([&](FoodItem &food) {
                if (food.isAssigned || food.isCancelled || food.isExpired || food.getRemainingMinutes() <= 0.0) return;
                if (food.donorID == rec->getId()) return;

                if (toLower(food.name).find(toLower(req.foodName)) != string::npos ||
                    toLower(req.foodName).find(toLower(food.name)) != string::npos) {

                    string reason;
                    if (rec->isCompatible(food, reason)) {
                        bool allergenConflict = false;
                        if (!req.allergyRestrictions.empty()) {
                            vector<string> restricted = splitTokens(req.allergyRestrictions);
                            vector<string> present = splitTokens(food.allergens);
                            for (const auto &p : present) {
                                for (const auto &rst : restricted) {
                                    if (p == rst || p == rst + "s" || rst == p + "s") {
                                        allergenConflict = true;
                                        break;
                                    }
                                }
                                if (allergenConflict) break;
                            }
                        }

                        if (!allergenConflict) {
                            compatibleLots.push_back(&food);
                            totalAvailableQty += food.quantity;
                        }
                    }
                }
            });

            if (compatibleLots.empty()) continue;

            // Case A: Total available food is LESS than requested quantity
            if (totalAvailableQty < req.requestedQuantity) {
                cout << "\n======================================================\n";
                cout << "QUANTITY NOTICE FOR RECIPIENT #" << rec->getId() << " (" << rec->getName() << ")\n";
                cout << "Requested Dish: " << req.foodName << "\n";
                cout << "Requested Quantity: " << req.requestedQuantity << " servings\n";
                cout << "Total Currently Available: " << totalAvailableQty << " servings\n";
                cout << "Are you willing to confirm the order for the available " << totalAvailableQty << " servings?\n";
                cout << "  1. Yes (Accept Partial Order)\n";
                cout << "  2. No (Wait for full quantity)\n";
                int userDecision = readMenuChoice("Enter choice (1-2): ", 1, 2);

                if (userDecision == 2) {
                    cout << ">>> Order remains in queue awaiting additional surplus.\n";
                    cout << "======================================================\n";
                    continue;
                }

                req.requestedQuantity -= totalAvailableQty;
                if (req.requestedQuantity == 0) req.status = "Matched";

                for (FoodItem *foodPtr : compatibleLots) {
                    foodPtr->isAssigned = true;
                    double deliveryCost = 30.0;

                    int currentOrderId = nextOrderId++;
                    cout << "\nOrder ID: " << currentOrderId << "\n";
                    cout << "Food ID: " << foodPtr->foodID << "\n";
                    cout << "Dispatched: [" << foodPtr->name << "] ("
                         << foodPtr->quantity << " servings) to " << rec->getName() << "\n";
                    cout << "Donor Address: " << foodPtr->donorAddress << "\n";
                    cout << "Standard Delivery Fee: Rs. " << deliveryCost << "\n";

                    addNotification(foodPtr->donorID, "Order #" + to_string(currentOrderId) + ": Food #" + to_string(foodPtr->foodID) +
                                    " matched to " + rec->getName() + ". Please prepare for pickup.");
                    addNotification(rec->getId(), "Order #" + to_string(currentOrderId) + ": " + to_string(foodPtr->quantity) + " servings of " +
                                    foodPtr->name + " from Donor #" + to_string(foodPtr->donorID) + " at " + foodPtr->donorAddress + " is dispatched.");

                    orderHistory.emplace_back(currentOrderId, foodPtr->foodID, foodPtr->donorID, rec->getId(),
                                              foodPtr->quantity, deliveryCost, "Dispatched");
                }
                cout << "======================================================\n";
            }
            // Case B: Available food is EQUAL TO OR GREATER than requested quantity
            else {
                int needed = req.requestedQuantity;
                vector<pair<FoodItem*, int>> matchedAllocations;

                for (FoodItem *foodPtr : compatibleLots) {
                    if (needed <= 0) break;
                    int take = min(foodPtr->quantity, needed);
                    matchedAllocations.push_back({foodPtr, take});
                    needed -= take;
                }

                req.status = "Matched";
                req.requestedQuantity = 0;

                cout << "\n======================================================\n";
                for (auto &alloc : matchedAllocations) {
                    FoodItem *foodPtr = alloc.first;
                    int allocatedQty = alloc.second;

                    if (foodPtr->quantity == allocatedQty) {
                        foodPtr->isAssigned = true;
                    } else {
                        foodPtr->quantity -= allocatedQty;
                    }

                    double deliveryCost = 30.0;
                    int currentOrderId = nextOrderId++;
                    cout << "Order ID: " << currentOrderId << "\n";
                    cout << "Food ID: " << foodPtr->foodID << "\n";
                    cout << "Dispatched: [" << foodPtr->name << "] ("
                         << allocatedQty << " servings) to " << rec->getName() << "\n";
                    cout << "Donor Address: " << foodPtr->donorAddress << "\n";
                    cout << "Delivery Fee: Rs. " << deliveryCost << "\n\n";

                    addNotification(foodPtr->donorID, "Order #" + to_string(currentOrderId) + ": Food #" + to_string(foodPtr->foodID) +
                                    " (" + to_string(allocatedQty) + " servings) matched to " + rec->getName() + ".");
                    addNotification(rec->getId(), "Order #" + to_string(currentOrderId) + ": " + to_string(allocatedQty) + " servings of " +
                                    foodPtr->name + " from Donor #" + to_string(foodPtr->donorID) + " at " + foodPtr->donorAddress + " dispatched.");

                    orderHistory.emplace_back(currentOrderId, foodPtr->foodID, foodPtr->donorID, rec->getId(),
                                              allocatedQty, deliveryCost, "Dispatched");
                }
                cout << "======================================================\n";
            }
        }

        for (const auto &r : pendingRequests) {
            if (r.status == "Pending" && r.requestedQuantity > 0) {
                requestQueue.push(r);
            }
        }
    }

    void confirmDelivery(int orderId, int userId, bool isDonor) {
        for (auto &ord : orderHistory) {
            if (ord.orderID == orderId) {
                if (isDonor && ord.donorID == userId) {
                    ord.donorConfirmed = true;
                    cout << ">>> Donor delivery dispatch confirmed.\n";
                } else if (!isDonor && ord.recipientID == userId) {
                    ord.recipientConfirmed = true;
                    cout << ">>> Recipient order receipt confirmed.\n";
                } else {
                    cout << "[Error] User ID does not match order record.\n";
                    return;
                }

                if (ord.donorConfirmed && ord.recipientConfirmed) {
                    ord.status = "Delivered/Fulfilled";
                    cout << ">>> SUCCESS: Two-Way Delivery Handshake Completed. Order fully closed.\n";
                    addNotification(ord.donorID, "Order #" + to_string(orderId) + " marked delivered by recipient.");
                    addNotification(ord.recipientID, "Order #" + to_string(orderId) + " receipt finalized.");
                }
                return;
            }
        }
        cout << "[Error] Order ID not found.\n";
    }

    void fileSpoiledFoodComplaint(int orderId, int recipientId, const string &details) {
        for (auto &ord : orderHistory) {
            if (ord.orderID == orderId) {
                if (ord.recipientID != recipientId) {
                    cout << "[Error] You are not the recipient of this order.\n";
                    return;
                }
                if (ord.isSpoiledComplaint) {
                    cout << "[Error] Complaint already registered for this order.\n";
                    return;
                }

                ord.isSpoiledComplaint = true;
                ord.status = "Complaint: Spoiled Food";

                User *donor = findUser(ord.donorID);
                if (donor) {
                    donor->addPenalty(250.0);
                    cout << "\n>>> COMPLAINT RECORDED: Food was found spoilt.\n";
                    cout << ">>> PENALTY IMPOSED: Donor #" << donor->getId() << " penalized Rs. 250 and received 1 strike.\n";
                    cout << ">>> Reason: " << details << "\n";

                    addNotification(donor->getId(), "PENALTY NOTICE: Order #" + to_string(orderId) +
                                    " was reported SPOILED. Rs. 250 fine and 1 penalty strike applied.");
                    addNotification(recipientId, "Your spoiled food complaint for Order #" + to_string(orderId) + " has been processed.");
                }
                return;
            }
        }
        cout << "[Error] Order ID not found.\n";
    }

    void displayUserNotifications(int userId) const {
        cout << "\n--- NOTIFICATIONS FOR USER #" << userId << " ---\n";
        bool hasAny = false;
        for (const auto &n : notifications) {
            if (n.userId == userId) {
                cout << "  * " << n.message << "\n";
                hasAny = true;
            }
        }
        if (!hasAny) cout << "  (No notifications)\n";
    }

    void displayOrderTransactions() const {
        cout << "\n--- ORDER TRANSACTIONS & DISPATCH STATUS ---\n";
        if (orderHistory.empty()) {
            cout << "  (No orders recorded)\n";
            return;
        }
        for (const auto &o : orderHistory) {
            cout << "Order #" << o.orderID << " | Food #" << o.foodID << " | Donor #" << o.donorID
                 << " -> Recipient #" << o.recipientID << " | Qty: " << o.quantity
                 << " | Status: " << o.status
                 << " [Donor Confirmed: " << (o.donorConfirmed ? "Yes" : "No")
                 << " | Recipient Confirmed: " << (o.recipientConfirmed ? "Yes" : "No") << "]\n";
        }
    }

    void saveToDisk() {
        ofstream uout("users.dat");
        if (uout) {
            uout << userRegistry.size() << "\n";
            for (User *u : userRegistry) {
                if (Donor *d = dynamic_cast<Donor*>(u)) {
                    uout << "DONOR\n" << d->getId() << "\n" << d->getName() << "\n"
                         << d->getContact() << "\n" << d->getAddress() << "\n"
                         << d->getDonorType() << "\n"
                         << d->getPenaltyPoints() << "\n" << d->getPenaltyFines() << "\n";
                } else if (NGO *n = dynamic_cast<NGO*>(u)) {
                    uout << "NGO\n" << n->getId() << "\n" << n->getName() << "\n"
                         << n->getContact() << "\n" << n->getAddress() << "\n"
                         << n->getAcceptedDietType() << "\n"
                         << n->getPenaltyPoints() << "\n" << n->getPenaltyFines() << "\n";
                } else if (AnimalShelter *a = dynamic_cast<AnimalShelter*>(u)) {
                    uout << "SHELTER\n" << a->getId() << "\n" << a->getName() << "\n"
                         << a->getContact() << "\n" << a->getAddress() << "\n"
                         << a->getSpecies() << "\n"
                         << a->getPenaltyPoints() << "\n" << a->getPenaltyFines() << "\n";
                }
            }
        }
    }

    void loadFromDisk() {
        ifstream uin("users.dat");
        if (!uin) return;
        int count;
        if (uin >> count) {
            uin.ignore(numeric_limits<streamsize>::max(), '\n');
            for (int i = 0; i < count; ++i) {
                string role;
                getline(uin, role);
                role = trim(role);

                int id;
                string name, contact, addr;

                uin >> id;
                uin.ignore(numeric_limits<streamsize>::max(), '\n');
                getline(uin, name);
                getline(uin, contact);
                getline(uin, addr);

                if (role == "DONOR") {
                    string type;
                    int pts; double fines;
                    getline(uin, type);
                    uin >> pts >> fines;
                    uin.ignore(numeric_limits<streamsize>::max(), '\n');
                    userRegistry.push_back(new Donor(id, name, contact, addr, type, pts, fines));
                } else if (role == "NGO") {
                    string diet;
                    int pts; double fines;
                    getline(uin, diet);
                    uin >> pts >> fines;
                    uin.ignore(numeric_limits<streamsize>::max(), '\n');
                    userRegistry.push_back(new NGO(id, name, contact, addr, diet, pts, fines));
                } else if (role == "SHELTER") {
                    int pts; double fines;
                    string species;
                    getline(uin, species);
                    uin >> pts >> fines;
                    uin.ignore(numeric_limits<streamsize>::max(), '\n');
                    userRegistry.push_back(new AnimalShelter(id, name, contact, addr, species, pts, fines));
                }
                if (id >= nextUserId) nextUserId = id + 1;
            }
        }
    }

    void adminDisplayInventory() {
        autoPurgeExpiredFood();
        cout << "\n--- Master Food Inventory (Custom Singly Linked List) ---\n";
        inventory.displayAll();
    }

    void adminDisplayUrgencyHeap() {
        autoPurgeExpiredFood();
        cout << "\n--- Expiry Urgency Radar (Custom Binary Min-Heap) ---\n";
        urgencyHeap.displayHeap();
    }

    void adminDisplayRequestQueue() {
        autoPurgeExpiredFood();
        cout << "\n--- Recipient FIFO Request Pipeline (std::queue) ---\n";
        if (requestQueue.empty()) {
            cout << "  (Request Queue is empty)\n";
            return;
        }
        int sz = static_cast<int>(requestQueue.size());
        for (int i = 0; i < sz; ++i) {
            FoodRequest req = requestQueue.front();
            requestQueue.pop();
            cout << "  Request #" << req.requestID << " for \"" << req.foodName
                 << "\" (" << req.requestedQuantity << " servings) by Recipient #"
                 << req.recipientID << " [Status: " << req.status << "]\n";
            requestQueue.push(req);
        }
    }

    void adminDisplayUsers() const {
        cout << "\n--- Registered Ecosystem Entities ---\n";
        if (userRegistry.empty()) {
            cout << "  (No registered users)\n";
            return;
        }
        for (User *u : userRegistry) u->display();
    }
};

// ============================================================================
// PHONE NUMBER VALIDATION HELPER (10 DIGITS & UNIQUE)
// ============================================================================
string readValidPhoneNumber(const string &prompt, const FoodRescueSystem &system) {
    while (true) {
        string phone = readNonEmptyLine(prompt);

        bool allDigits = (phone.length() == 10);
        if (allDigits) {
            for (char c : phone) {
                if (!isdigit(static_cast<unsigned char>(c))) {
                    allDigits = false;
                    break;
                }
            }
        }

        if (!allDigits) {
            cout << "[Error] Phone number must contain exactly 10 digits (0-9). Try again.\n";
            continue;
        }

        if (system.isPhoneRegistered(phone)) {
            cout << "[Error] This phone number is already registered in the system. Use another phone number.\n";
            continue;
        }

        return phone;
    }
}

// ============================================================================
// 8. INTERACTIVE SYSTEM CONTROLLER
// ============================================================================
int main() {
    FoodRescueSystem system;
    int choice;

    do {
        cout << "\n=======================================================\n"
             << "   FOODRESCUE: SMART SURPLUS MANAGEMENT SYSTEM        \n"
             << "=======================================================\n"
             << "1. Register Donor\n"
             << "2. Register Recipient (NGO)\n"
             << "3. Register Animal Shelter\n"
             << "4. Donate Food Surplus\n"
             << "5. Submit Food Request\n"
             << "6. Cancel Active Food Donation\n"
             << "7. Two-Way Delivery Confirmation\n"
             << "8. File Spoiled Food Complaint\n"
             << "9. View Notifications and Inbox\n"
             << "10. View Order Tracking Ledger\n"
             << "0. Exit and Save\n";

        choice = readMenuChoice("Choice: ", 0, 10);

        switch (choice) {
            case 1: {
                string name = readNonEmptyLine("Donor Org / Name: ");
                string contact = readValidPhoneNumber("Contact Phone (10 digits): ", system);
                string addr = readNonEmptyLine("Physical Street Address: ");
                string type = readNonEmptyLine("Donor Type (Restaurant/Mess/Catering): ");
                int id = system.registerDonor(name, contact, addr, type);
                cout << "Registered. Assigned Donor ID = " << id << "\n";
                break;
            }
            case 2: {
                string name = readNonEmptyLine("NGO / Shelter Name: ");
                string contact = readValidPhoneNumber("Contact Phone (10 digits): ", system);
                string addr = readNonEmptyLine("Physical Street Address: ");
                string diet = readNonEmptyLine("Diet Requirement (veg / all): ");
                int id = system.registerNGO(name, contact, addr, diet);
                cout << "Registered. Assigned Recipient ID = " << id << "\n";
                break;
            }
            case 3: {
                string name = readNonEmptyLine("Animal Shelter Name: ");
                string contact = readValidPhoneNumber("Contact Phone (10 digits): ", system);
                string addr = readNonEmptyLine("Physical Street Address: ");
                string species = readNonEmptyLine("Species (e.g. Dogs / Cats): ");
                int id = system.registerAnimalShelter(name, contact, addr, species);
                cout << "Registered. Assigned Animal Shelter ID = " << id << "\n";
                break;
            }
            case 4: {
                int dId = readPositiveInt("Enter Registered Entity ID: ");
                if (!system.userExists(dId)) {
                    cout << "[Error] Entity ID #" << dId << " is not registered. Please register first.\n";
                    break;
                }
                string foodName = readNonEmptyLine("Food Dish Name: ");
                string cat = readNonEmptyLine("Category (Prepared Meal / Bakery): ");
                int qty = readPositiveInt("Servings Quantity: ");
                int hrs = readPositiveInt("Expiry Shelf-Life (hours remaining): ");
                string diet = readNonEmptyLine("Diet Type ('veg', 'non-veg', or 'plain'): ");
                string ing = readNonEmptyLine("List Raw Ingredients (comma-separated): ");
                string spice = readNonEmptyLine("Spice Level ('Mild', 'Medium', 'Spicy'): ");
                string notes = readNonEmptyLine("Extra Info / Prep Notes: ");
                system.donateFood(dId, foodName, cat, qty, hrs, diet, ing, spice, notes);
                break;
            }
            case 5: {
                int rId = readPositiveInt("Enter Registered Recipient ID: ");
                if (!system.recipientExists(rId)) {
                    cout << "[Error] Recipient #" << rId << " is not registered. Please register first.\n";
                    break;
                }
                string foodName = readNonEmptyLine("Food Dish Requested: ");
                int qty = readPositiveInt("Servings Needed: ");
                cout << "Allergen Restrictions (e.g., 'Dairy, Peanuts, Gluten' or 'none'): ";
                string allergies;
                getline(cin, allergies);
                allergies = trim(allergies);
                if (allergies.empty()) allergies = "none";

                system.requestFood(rId, foodName, qty, allergies);
                break;
            }
            case 6: {
                int fId = readPositiveInt("Enter Food Item ID to cancel: ");
                int dId = readPositiveInt("Enter your Donor ID: ");
                system.cancelDonation(fId, dId);
                break;
            }
            case 7: {
                int ordId = readPositiveInt("Enter Order ID: ");
                int uId = readPositiveInt("Enter your User ID: ");
                int role = readMenuChoice("Are you (1) Donor or (2) Recipient? ", 1, 2);
                system.confirmDelivery(ordId, uId, (role == 1));
                break;
            }
            case 8: {
                int ordId = readPositiveInt("Enter Order ID with spoiled food: ");
                int rId = readPositiveInt("Enter your Recipient ID: ");
                if (!system.recipientExists(rId)) {
                    cout << "[Error] Recipient ID not found.\n";
                    break;
                }
                string details = readNonEmptyLine("Describe condition of spoiled food: ");
                system.fileSpoiledFoodComplaint(ordId, rId, details);
                break;
            }
            case 9: {
                int uId = readPositiveInt("Enter your User ID: ");
                system.displayUserNotifications(uId);
                break;
            }
            case 10:
                system.displayOrderTransactions();
                break;

            case 999: {
                cout << "\n[ADMIN ACCESS GRANTED] Select internal diagnostic action:\n";
                cout << "  1. Run Matching Engine Manually\n";
                cout << "  2. View Active Inventory (Linked List)\n";
                cout << "  3. View Urgency Radar (Min-Heap)\n";
                cout << "  4. View Request Pipeline (std::queue)\n";
                cout << "  5. View Registered Entities (Taxonomy)\n";
                cout << "  6. Trigger Manual Expiry Sweep\n";
                int aChoice = readMenuChoice("Admin choice (1-6): ", 1, 6);
                switch (aChoice) {
                    case 1: system.processMatchingPipeline(); break;
                    case 2: system.adminDisplayInventory(); break;
                    case 3: system.adminDisplayUrgencyHeap(); break;
                    case 4: system.adminDisplayRequestQueue(); break;
                    case 5: system.adminDisplayUsers(); break;
                    case 6: {
                        system.autoPurgeExpiredFood();
                        cout << ">>> Expiry sweep completed.\n";
                        break;
                    }
                }
                break;
            }

            case 0:
                cout << "\nTerminating session. Backing up persistent data...\n";
                break;
            default:
                cout << "[Error] Invalid choice.\n";
                break;
        }
    } while (choice != 0);

    return 0;
}
