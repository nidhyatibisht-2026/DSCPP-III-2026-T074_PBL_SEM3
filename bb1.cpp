#include <iostream>
#include <string>
#include <vector>
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

// ---------------- Phase 1 system: pure intake ----------------
class FoodSurplusManagementSystem
{
private:
    vector<Donor> donors;
    vector<NGO> ngos;
    vector<FoodItem> foodItems;

    int nextUserId = 1, nextFoodId = 1;

public:
    int registerDonor(const string &name)
    {
        donors.emplace_back(nextUserId, name);
        return nextUserId++;
    }

    int registerNGO(const string &name, const string &requirements)
    {
        ngos.emplace_back(nextUserId, name, requirements);
        return nextUserId++;
    }

    void donateFood(int donorId, const string &foodName, int expiryDays, const string &allergens)
    {
        foodItems.push_back({nextFoodId++, donorId, expiryDays, foodName, allergens});
        cout << "-> Food item recorded.\n";
    }

    void showAllDonors()
    {
        cout << "\n--- Donors ---\n";
        if (donors.empty()) { cout << "  (none registered yet)\n"; return; }
        for (auto &d : donors) d.display();
    }

    void showAllNGOs()
    {
        cout << "\n--- NGOs (Recipients) ---\n";
        if (ngos.empty()) { cout << "  (none registered yet)\n"; return; }
        for (auto &n : ngos) n.display();
    }

    void showAllFood()
    {
        cout << "\n--- Food Available ---\n";
        if (foodItems.empty()) { cout << "  (none logged yet)\n"; return; }
        for (auto &f : foodItems) f.display();
    }
};

// ---------------- MAIN: simple menu ----------------
int main()
{
    FoodSurplusManagementSystem system;
    int choice;

    do {
        cout << "\n===== FOOD SURPLUS MANAGEMENT SYSTEM (Phase 1: Intake) =====\n"
             << "1. Register Donor\n"
             << "2. Donate Food (Donor form)\n"
             << "3. Register NGO (Recipient)\n"
             << "4. Show All Donors\n"
             << "5. Show All NGOs\n"
             << "6. Show All Food Available\n"
             << "0. Exit\n"
             << "Choice: ";
        cin >> choice;
        cin.ignore();

        string name, food, extra;
        int id, days;

        switch (choice)
        {
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
            case 4: system.showAllDonors(); break;
            case 5: system.showAllNGOs(); break;
            case 6: system.showAllFood(); break;
        }
    } while (choice != 0);

    return 0;
}
