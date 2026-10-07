#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <limits>
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
        cin.clear(); // clears the fail flag so cin is usable again
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // discard the bad line
    }
}

int readNonNegativeInt(const string &prompt) {
    while (true) {
        int v = readInt(prompt);
        if (v >= 0) return v;
        cout << "Value cannot be negative - spoiled/expired food can't be logged. Try again.\n";
    }
}

// ---------------- small string helpers (used for matching) ----------------
string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(), ::tolower);
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
        if (!t.empty() && t != "none") tokens.push_back(t);
    }
    return tokens;
}

// ---------------- Base class ----------------
class User {
protected:
    int id;
    string name;
public:
    User(int id, string name) : id(id), name(name) {}
    int getId() const { return id; }
    virtual void display() const {
        cout << "User #" << id << " - " << name << endl;
    }
};

class Donor : public User {
    string address;
public:
    Donor(int id, string name, string address) : User(id, name), address(address) {}
    string getAddress() const { return address; }
    void display() const override {
        cout << "Donor #" << id << " - " << name << " | Address: " << address << endl;
    }
};

class NGO : public User {
    string requirements;         // food items this recipient needs
    string allergyRestrictions;  // allergens this recipient CANNOT accept
    string foodTypePreference;   // Veg, Non-Veg, or Any
public:
    NGO(int id, string name, string requirements, string allergyRestrictions, string foodTypePreference)
        : User(id, name), requirements(requirements),
          allergyRestrictions(allergyRestrictions), foodTypePreference(foodTypePreference) {}

    string getRequirements() const { return requirements; }
    string getAllergyRestrictions() const { return allergyRestrictions; }
    string getFoodTypePreference() const { return foodTypePreference; }

    void display() const override {
        cout << "NGO #" << id << " - " << name
             << " | Needs: " << (requirements.empty() ? "(not specified)" : requirements)
             << " | Food Type: " << foodTypePreference
             << " | Cannot accept: " << (allergyRestrictions.empty() ? "(none stated)" : allergyRestrictions)
             << endl;
    }
};

// ---------------- Food item on file ----------------
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
             << " | Allergens: " << (allergens.empty() ? "none listed" : allergens) << endl;
    }
};

// Linked-list node for the complete food inventory.
struct FoodNode {
    FoodItem item;
    FoodNode* next;
    FoodNode(const FoodItem& item) : item(item), next(nullptr) {}
};

struct FoodRequest {
    int id, ngoId;
    string foodName;
    void display() const {
        cout << "Request#" << id << " [" << foodName << "] by NGO#" << ngoId << endl;
    }
};

// Min-heap by expiryDays: the food closest to expiring comes out first
struct CompareExpiry {
    bool operator()(const FoodItem &a, const FoodItem &b) {
        return a.expiryDays > b.expiryDays;
    }
};

// ---------------- Phase 2 system: intake + matching ----------------
class FoodSurplusManagementSystem {
private:
    vector<Donor> donors;
    vector<NGO> ngos;

    // Linked list for complete food inventory.
    FoodNode* foodHead = nullptr;

    // Global min-heap: the food closest to expiry has highest priority.
    priority_queue<FoodItem, vector<FoodItem>, CompareExpiry> foodExpiryQueue;

    // Hash table: allergen -> food IDs containing that allergen.
    unordered_map<string, vector<int>> allergyHashTable;

    queue<FoodRequest> requestQueue; // NGO requests, FIFO

    vector<string> history;           // log of completed matches
    map<int, vector<int>> donorToNgo; // graph: donorId -> ngoIds it has fed

    int nextUserId = 1, nextFoodId = 1, nextRequestId = 1;

    void addToFoodInventory(const FoodItem& item) {
        FoodNode* newNode = new FoodNode(item);
        if (!foodHead) { foodHead = newNode; return; }
        FoodNode* current = foodHead;
        while (current->next) current = current->next;
        current->next = newNode;
    }

    void addToAllergyHash(const FoodItem& item) {
        for (const string& allergen : splitTokens(item.allergens))
            allergyHashTable[allergen].push_back(item.id);
    }

    void clearFoodInventory() {
        while (foodHead) {
            FoodNode* temp = foodHead;
            foodHead = foodHead->next;
            delete temp;
        }
    }

    void showFoodInventory() {
        cout << "\n--- Food Inventory (Linked List) ---\n";
        if (!foodHead) { cout << "  (empty)\n"; return; }
        FoodNode* current = foodHead;
        while (current) { cout << "  "; current->item.display(); current = current->next; }
    }

    void allergyLookup(const string& allergen) {
        string key = toLower(trim(allergen));
        cout << "\n--- Allergy Hash Table Lookup ---\n";
        auto it = allergyHashTable.find(key);
        if (it == allergyHashTable.end()) {
            cout << "No food items found containing allergen: " << key << endl;
            return;
        }
        cout << "Food item IDs containing " << key << ": ";
        for (int id : it->second) cout << "#" << id << " ";
        cout << endl;
    }

    NGO *findNgoById(int ngoId) {
        for (auto &n : ngos) if (n.getId() == ngoId) return &n;
        return nullptr;
    }

    // true if none of the food's allergens appear in the NGO's restriction list
    bool isRequirementSatisfied(const string &foodName, const string &requirements) {
        vector<string> required = splitTokens(requirements);
        if (required.empty()) return true;
        string key = toLower(trim(foodName));
        for (const string &r : required)
            if (key == r) return true;
        return false;
    }

    bool isAllergySafe(const string &itemAllergens, const string &ngoRestrictions) {
        vector<string> restricted = splitTokens(ngoRestrictions);
        if (restricted.empty()) return true; // NGO stated no restrictions

        vector<string> present = splitTokens(itemAllergens);
        for (auto &a : present)
            for (auto &r : restricted)
                if (a == r) return false; // conflict found

        return true;
    }

    bool tryMatch(const FoodRequest &req) {
        NGO *ngo = findNgoById(req.ngoId);
        if (!ngo) return false;

        // 1. NGO requirement must include the requested food (unless no requirement was specified).
        if (!isRequirementSatisfied(req.foodName, ngo->getRequirements())) {
            cout << "No match: " << req.foodName
                 << " is not in NGO#" << req.ngoId << " requirements.\n";
            return false;
        }

        string requestedFood = toLower(trim(req.foodName));
        string preferredType = ngo->getFoodTypePreference();
        string restrictions = ngo->getAllergyRestrictions();

        vector<FoodItem> setAside;
        bool found = false;
        FoodItem matched{};

        // Global min-heap: inspect food from earliest expiry first.
        while (!foodExpiryQueue.empty()) {
            FoodItem candidate = foodExpiryQueue.top();
            foodExpiryQueue.pop();

            bool sameFood = (toLower(trim(candidate.foodName)) == requestedFood);
            bool typeOkay = (preferredType == "Any" || candidate.foodType == preferredType);
            bool allergyOkay = isAllergySafe(candidate.allergens, restrictions);

            if (sameFood && typeOkay && allergyOkay) {
                matched = candidate;
                found = true;
                break;
            }

            // Not suitable for this request, so keep it available for other requests.
            setAside.push_back(candidate);
        }

        // Restore all food that was not matched.
        for (auto &item : setAside) foodExpiryQueue.push(item);

        if (!found) return false;

        // Remove the matched food from the linked-list inventory as well.
        FoodNode *current = foodHead;
        FoodNode *previous = nullptr;
        while (current) {
            if (current->item.id == matched.id) {
                if (previous) previous->next = current->next;
                else foodHead = current->next;
                delete current;
                break;
            }
            previous = current;
            current = current->next;
        }

        // Remove the matched food from the allergen lookup table.
        for (const string &allergen : splitTokens(matched.allergens)) {
            auto it = allergyHashTable.find(allergen);
            if (it != allergyHashTable.end()) {
                auto &ids = it->second;
                ids.erase(remove(ids.begin(), ids.end(), matched.id), ids.end());
                if (ids.empty()) allergyHashTable.erase(it);
            }
        }

        cout << "MATCHED: " << matched.foodName
             << " [" << matched.foodType << "]"
             << " (Donor#" << matched.donorId
             << ") -> NGO#" << req.ngoId << endl;
        cout << "EXPIRY PRIORITY: " << matched.expiryDays << " day(s) remaining\n";
        cout << "PICKUP ADDRESS: " << matched.pickupAddress << endl;

        donorToNgo[matched.donorId].push_back(req.ngoId);
        history.push_back("Donor#" + to_string(matched.donorId) +
                           " -> NGO#" + to_string(req.ngoId) +
                           " (" + matched.foodName + ", " + matched.foodType +
                           ") | Pickup: " + matched.pickupAddress);
        return true;
    }

public:
    bool donorExists(int donorId) {
        for (auto &d : donors) if (d.getId() == donorId) return true;
        return false;
    }

    bool ngoExists(int ngoId) {
        return findNgoById(ngoId) != nullptr;
    }

    // ---- Registration ----
    int registerDonor(const string &name, const string &address) {
        donors.emplace_back(nextUserId, name, address);
        return nextUserId++;
    }

    int registerNGO(const string &name, const string &requirements, const string &allergyRestrictions, const string &foodTypePreference) {
        ngos.emplace_back(nextUserId, name, requirements, allergyRestrictions, foodTypePreference);
        return nextUserId++;
    }

    // ---- Donor side: donate food ----
    bool donateFood(int donorId, const string &foodName, int expiryDays,
                    const string &allergens, const string &foodType) {
        if (!donorExists(donorId)) {
            cout << "Error: Donor #" << donorId << " is not registered. Donation rejected.\n";
            return false;
        }
        if (expiryDays < 0) {
            cout << "Error: Expiry days cannot be negative. Donation rejected.\n";
            return false;
        }
        if (foodType != "Veg" && foodType != "Non-Veg") {
            cout << "Error: Food type must be Veg or Non-Veg.\n";
            return false;
        }

        string pickupAddress;
        for (const auto& d : donors)
            if (d.getId() == donorId) { pickupAddress = d.getAddress(); break; }

        FoodItem item{nextFoodId++, donorId, expiryDays, foodName, allergens, foodType, pickupAddress};

        // 1. Linked-list inventory
        addToFoodInventory(item);
        // 2. Global min-heap for expiry priority
        foodExpiryQueue.push(item);
        // 3. Hash table for allergy lookup
        addToAllergyHash(item);

        cout << "-> Food added to linked-list inventory.\n";
        cout << "-> Added to expiry priority queue for \"" << foodName << "\".\n";
        cout << "-> Pickup Address: " << pickupAddress << "\n";
        return true;
    }

    // ---- NGO side: request food -> goes into the FIFO queue ----
    bool requestFood(int ngoId, const string &foodName) {
        if (!ngoExists(ngoId)) {
            cout << "Error: NGO #" << ngoId << " is not registered. Request rejected.\n";
            return false;
        }
        requestQueue.push({nextRequestId++, ngoId, foodName});
        cout << "-> Added to request queue.\n";
        return true;
    }

    // ---- Matching ----
    void matchAll() {
        bool progress = true;
        while (progress) {
            progress = false;
            int rounds = requestQueue.size();
            for (int i = 0; i < rounds; i++) {
                FoodRequest req = requestQueue.front(); requestQueue.pop();
                if (tryMatch(req)) progress = true;
                else requestQueue.push(req); // still unfulfilled, keep it in the queue
            }
        }
    }

    // ---- Display helpers ----
    void showAllDonors() {
        cout << "\n--- Donors ---\n";
        if (donors.empty()) { cout << "  (none registered yet)\n"; return; }
        for (auto &d : donors) d.display();
    }

    void showAllNGOs() {
        cout << "\n--- NGOs (Recipients) ---\n";
        if (ngos.empty()) { cout << "  (none registered yet)\n"; return; }
        for (auto &n : ngos) n.display();
    }

    void showFoodInventoryPublic() { showFoodInventory(); }
    void lookupAllergyPublic(const string& allergen) { allergyLookup(allergen); }

    void showFoodQueue() {
        cout << "\n--- Global Food Priority Queue (Min-Heap: earliest expiry first) ---\n";
        auto copy = foodExpiryQueue;
        if (copy.empty()) { cout << "  (empty)\n"; return; }
        while (!copy.empty()) { cout << "  "; copy.top().display(); copy.pop(); }
    }

    void showRequestQueue() {
        cout << "\n--- NGO Request Queue (FIFO) ---\n";
        auto copy = requestQueue;
        if (copy.empty()) { cout << "  (empty)\n"; return; }
        while (!copy.empty()) { copy.front().display(); copy.pop(); }
    }

    void showGraph() {
        cout << "\n--- Donor -> NGO Graph ---\n";
        if (donorToNgo.empty()) { cout << "  (no matches yet)\n"; return; }
        for (auto &[donorId, ngoIds] : donorToNgo) {
            cout << "  Donor#" << donorId << " helped NGO(s): ";
            for (int ngoId : ngoIds) cout << "#" << ngoId << " ";
            cout << endl;
        }
    }

    void showHistory() {
        cout << "\n--- Distribution History ---\n";
        if (history.empty()) { cout << "  (none yet)\n"; return; }
        for (auto &line : history) cout << "  " << line << endl;
    }

    ~FoodSurplusManagementSystem() { clearFoodInventory(); }
};

// ---------------- MAIN: simple menu ----------------
int main() {
    FoodSurplusManagementSystem system;
    int choice;

   do
{
    cout << "\n===== FOOD SURPLUS MANAGEMENT SYSTEM (Phase 2: Matching) =====\n"
         << "1. Register Donor\n"
         << "2. Donate Food (Donor form)\n"
         << "3. Register NGO (Recipient)\n"
         << "4. Request Food (NGO form)\n"
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

        string name, address, food, extra, extra2, foodType;
        int id, days;

        switch (choice) {
            case 1:
                cout << "Donor name: "; getline(cin, name);
                cout << "Donor pickup address: "; getline(cin, address);
                cout << "Registered! Your Donor ID = " << system.registerDonor(name, address) << endl;
                break;
            case 2:
                id = readInt("Your Donor ID: ");
                while (!system.donorExists(id)) {
                    cout << "Error: Donor #" << id << " is not registered.\n";
                    id = readInt("Your Donor ID: ");
                }
                cout << "Food item name: "; getline(cin, food);
                days = readNonNegativeInt("Expires in how many days: ");
                do {
                    cout << "Food type (1 = Veg, 2 = Non-Veg): ";
                    getline(cin, foodType);
                    if (foodType == "1") foodType = "Veg";
                    else if (foodType == "2") foodType = "Non-Veg";
                    else cout << "Please choose 1 or 2.\n";
                } while (foodType != "Veg" && foodType != "Non-Veg");
                cout << "Allergens (comma-separated, or 'none'): "; getline(cin, extra);
                system.donateFood(id, food, days, extra, foodType);
                break;
            case 3:
                cout << "NGO name: "; getline(cin, name);
                cout << "Food requirements (comma-separated, or 'none'): "; getline(cin, extra);
                cout << "Allergy restrictions - food this NGO CANNOT accept (comma-separated, or 'none'): ";
                getline(cin, extra2);
                do {
                    cout << "Preferred food type (1 = Veg, 2 = Non-Veg, 3 = Any): ";
                    getline(cin, foodType);
                    if (foodType == "1") foodType = "Veg";
                    else if (foodType == "2") foodType = "Non-Veg";
                    else if (foodType == "3") foodType = "Any";
                    else cout << "Please choose 1, 2 or 3.\n";
                } while (foodType != "Veg" && foodType != "Non-Veg" && foodType != "Any");
                cout << "Registered! Your NGO ID = " << system.registerNGO(name, extra, extra2, foodType) << endl;
                break;
            case 4:
                id = readInt("Your NGO ID: ");
                while (!system.ngoExists(id)) {
                    cout << "Error: NGO #" << id << " is not registered.\n";
                    id = readInt("Your NGO ID: ");
                }
                cout << "Food item needed: "; getline(cin, food);
                system.requestFood(id, food);
                break;
            case 5:  system.matchAll(); break;
            case 6:  system.showFoodInventoryPublic(); break;
            case 7:  system.showFoodQueue(); break;
            case 8:  system.showRequestQueue(); break;
            case 9:  system.showGraph(); break;
            case 10: system.showHistory(); break;
            case 11:
                cout << "Enter allergen to search: "; getline(cin, extra);
                system.lookupAllergyPublic(extra);
                break;
            case 12: system.showAllDonors(); break;
            case 13: system.showAllNGOs(); break;
        }
    } while (choice != 0);

    return 0;
}
