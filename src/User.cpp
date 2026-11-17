#include "User.hpp"
#include "Utilities.hpp"

#include <iostream>
#include <vector>

using namespace std;

User::User(int id, string name, string contact, string addr,
           int pts, double fines)
        : id(id), name(name), contact(contact), address(addr),
      penaltyPoints(pts), penaltyFines(fines), active(true) {}

int User::getId() const { return id; }
string User::getName() const { return name; }
string User::getContact() const { return contact; }
string User::getAddress() const { return address; }
int User::getPenaltyPoints() const { return penaltyPoints; }
double User::getPenaltyFines() const { return penaltyFines; }

bool User::isActive() const { return active; }

void User::setActive(bool status) {
    active = status;
}

void User::addPenalty(double fine) {
    penaltyPoints++;
    penaltyFines += fine;
}

void User::display() const {
    cout << "User #" << id << ": " << name
         << " | Contact: " << contact
         << " | Addr: " << address
         << " | Penalties: " << penaltyPoints
         << " (Rs. " << penaltyFines << ")\n";
}

Admin::Admin(
    int id,
    string name,
    string contact,
    string addr,
    string adminUsername,
    string adminPassword
)
    : User(id, name, contact, addr),
      username(adminUsername),
      password(adminPassword) {}

bool Admin::authenticate(
    const string& enteredUsername,
    const string& enteredPassword
) const {
    return username == enteredUsername &&
           password == enteredPassword;
}

void Admin::display() const {
    cout << "[Administrator #" << id << "] " << name
         << " | Username: " << username << '\n';
}

Donor::Donor(int id, string name, string contact, string addr,
             string type, int pts, double fines)
    : User(id, name, contact, addr, pts, fines),
      donorType(type) {}

string Donor::getDonorType() const {
    return donorType;
}

void Donor::display() const {
    cout << "[Donor #" << id << "] " << name
         << " (" << donorType << ") | Addr: " << address
         << " | Ph: " << contact
         << " | Penalty Strikes: " << penaltyPoints << "\n";
}

Recipient::Recipient(int id, string name, string contact, string addr,
                     string diet, int pts, double fines)
    : User(id, name, contact, addr, pts, fines),
      acceptedDietType(toLower(trim(diet))) {}

string Recipient::getAcceptedDietType() const {
    return acceptedDietType;
}

NGO::NGO(int id, string name, string contact, string addr,
         string diet, int pts, double fines)
    : Recipient(id, name, contact, addr, diet, pts, fines) {}

bool NGO::isCompatible(const FoodItem& food,
                       string& rejectionReason) const {
    if (acceptedDietType == "veg" &&
        toLower(food.dietType) == "non-veg") {
        rejectionReason =
            "Dietary mismatch (NGO requires strictly Vegetarian meals)";
        return false;
    }
    return true;
}

void NGO::display() const {
    cout << "[NGO #" << id << "] " << name
         << " | Diet: " << acceptedDietType << "\n"
         << "       Addr: " << address << "\n";
}

AnimalShelter::AnimalShelter(int id, string name, string contact,
                             string addr, string spec,
                             int pts, double fines)
    : Recipient(id, name, contact, addr, "plain", pts, fines),
      species(spec) {}

string AnimalShelter::getSpecies() const {
    return species;
}

bool AnimalShelter::isCompatible(const FoodItem& food,
                                 string& rejectionReason) const {
    if (toLower(food.dietType) != "plain") {
        rejectionReason =
            "Animal Safety Violation: Cooked human food contains unsafe spices/oil";
        return false;
    }

    const string raw = toLower(food.ingredients);
    const vector<string> toxicList = {
        "onion", "garlic", "chocolate", "xylitol",
        "chili", "pepper", "raisin", "grape"
    };

    for (const auto& toxic : toxicList) {
        if (raw.find(toxic) != string::npos) {
            rejectionReason =
                "TOXICITY ALERT: Contains " + toxic +
                " which is lethal to animals!";
            return false;
        }
    }

    return true;
}

void AnimalShelter::display() const {
    cout << "[Animal Shelter #" << id << "] " << name
         << " (" << species << ") | Addr: " << address << "\n";
}
