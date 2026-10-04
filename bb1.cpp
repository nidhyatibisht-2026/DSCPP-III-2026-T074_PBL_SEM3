#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <map>
using namespace std;

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
public:
    Donor(int id, string name) : User(id, name) {}
    void display() const override
    {
        cout << "Donor #" << id << " - " << name << endl;
    }
};

class NGO : public User
{
    string requirements;
public:
    NGO(int id, string name, string requirements)
        : User(id, name), requirements(requirements) {}
    void display() const override
    {
        cout << "NGO #" << id << " - " << name
             << " | Needs: " << (requirements.empty() ? "(not specified)" : requirements) << endl;
    }
};

// ---------------- Food item on file ----------------
struct FoodItem {
    int id, donorId, expiryDays;
    string foodName;
    string allergens;
    void display() const
    {
        cout << "FoodItem#" << id << " [" << foodName << "] expires in "
             << expiryDays << " day(s), donated by Donor#" << donorId
             << " | Allergens: " << (allergens.empty() ? "none listed" : allergens) << endl;
    }
};

// ---------------- FIX: this struct was missing, causing a compile error
//                   wherever queue<FoodRequest> / FoodRequest was used ----------------
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

    priority_queue<FoodItem, vector<FoodItem>, CompareExpiry> foodQueue; // urgent food first
    queue<FoodRequest> requestQueue;                                    // NGO requests, FIFO

    vector<string> history;           // log of completed matches
    map<int, vector<int>> donorToNgo; // graph: donorId -> ngoIds it has fed

    int nextUserId = 1, nextFoodId = 1, nextRequestId = 1;

public:
    // ---- Registration (Phase 1) ----
    int registerDonor(const string &name) {
        donors.emplace_back(nextUserId, name);
        return nextUserId++;
    }

    int registerNGO(const string &name, const string &requirements) {
        ngos.emplace_back(nextUserId, name, requirements);
        return nextUserId++;
    }

    // ---- Donor side: donate food -> goes into the priority queue ----
    void donateFood(int donorId, const string &foodName, int expiryDays, const string &allergens) {
        foodQueue.push({nextFoodId++, donorId, expiryDays, foodName, allergens});
        cout << "-> Added to priority queue.\n";
    }

    // ---- NGO side: request food -> goes into the FIFO queue ----
    void requestFood(int ngoId, const string &foodName) {
        requestQueue.push({nextRequestId++, ngoId, foodName});
        cout << "-> Added to request queue.\n";
    }

    // ---- Matching: most urgent food + oldest request, paired together ----
    void matchOne() {
        if (foodQueue.empty())    { cout << "No food available right now.\n"; return; }
        if (requestQueue.empty()) { cout << "No pending requests right now.\n"; return; }

        FoodItem food = foodQueue.top();        foodQueue.pop();
        FoodRequest req = requestQueue.front(); requestQueue.pop();

        cout << "MATCHED: " << food.foodName << " (Donor#" << food.donorId
             << ") -> NGO#" << req.ngoId << endl;

        donorToNgo[food.donorId].push_back(req.ngoId); // record edge in graph
        history.push_back("Donor#" + to_string(food.donorId) +
                           " -> NGO#" + to_string(req.ngoId) +
                           " (" + food.foodName + ")");
    }

    void matchAll() {
        while (!foodQueue.empty() && !requestQueue.empty()) matchOne();
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

    void showFoodQueue() {
        cout << "\n--- Food Priority Queue (most urgent first) ---\n";
        auto copy = foodQueue;
        if (copy.empty()) { cout << "  (empty)\n"; return; }
        while (!copy.empty()) { copy.top().display(); copy.pop(); }
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
};

// ---------------- MAIN: simple menu ----------------
int main() {
    FoodSurplusManagementSystem system;
    int choice;

    do {
        cout << "\n===== FOOD SURPLUS MANAGEMENT SYSTEM (Phase 2: Matching) =====\n"
             << "1. Register Donor\n"
             << "2. Donate Food (Donor form)\n"
             << "3. Register NGO (Recipient)\n"
             << "4. Request Food (NGO form)\n"
             << "5. Match all pending food/requests\n"
             << "6. Show Food Queue\n"
             << "7. Show Request Queue\n"
             << "8. Show Donor-NGO Graph\n"
             << "9. Show Distribution History\n"
             << "10. Show All Donors\n"
             << "11. Show All NGOs\n"
             << "0. Exit\n"
             << "Choice: ";
        cin >> choice;
        cin.ignore();

        string name, food, extra;
        int id, days;

        switch (choice) {
            case 1:
                cout << "Donor name: "; getline(cin, name);
                cout << "Registered! Your Donor ID = " << system.registerDonor(name) << endl;
                break;
            case 2:
                cout << "Your Donor ID: "; cin >> id; cin.ignore();
                cout << "Food item name: "; getline(cin, food);
                cout << "Expires in how many days: "; cin >> days; cin.ignore();
                cout << "Allergens (comma-separated, or 'none'): "; getline(cin, extra);
                system.donateFood(id, food, days, extra);
                break;
            case 3:
                cout << "NGO name: "; getline(cin, name);
                cout << "Food requirements (comma-separated, or 'none'): "; getline(cin, extra);
                cout << "Registered! Your NGO ID = " << system.registerNGO(name, extra) << endl;
                break;
            case 4:
                cout << "Your NGO ID: "; cin >> id; cin.ignore();
                cout << "Food item needed: "; getline(cin, food);
                system.requestFood(id, food);
                break;
            case 5:  system.matchAll(); break;
            case 6:  system.showFoodQueue(); break;
            case 7:  system.showRequestQueue(); break;
            case 8:  system.showGraph(); break;
            case 9:  system.showHistory(); break;
            case 10: system.showAllDonors(); break;
            case 11: system.showAllNGOs(); break;
        }
    } while (choice != 0);

    return 0;
}
