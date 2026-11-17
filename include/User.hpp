#ifndef FOODFLOW_USER_HPP
#define FOODFLOW_USER_HPP

#include "Models.hpp"

#include <string>

class User {
protected:
    int id;
    std::string name;
    std::string contact;
    std::string address;
    int penaltyPoints;
    double penaltyFines;
    bool active;

public:
    User(int id, std::string name, std::string contact,
         std::string addr, int pts = 0, double fines = 0.0);
    virtual ~User() = default;

    int getId() const;
    std::string getName() const;
    std::string getContact() const;
    std::string getAddress() const;
    int getPenaltyPoints() const;
    double getPenaltyFines() const;
    bool isActive() const;
    void setActive(bool status);

    void addPenalty(double fine);
    virtual void display() const;
};

class Admin : public User {
private:
    std::string username;
    std::string password;

public:
    Admin(
        int id,
        std::string name,
        std::string contact,
        std::string addr,
        std::string adminUsername,
        std::string adminPassword
    );

    bool authenticate(
        const std::string& enteredUsername,
        const std::string& enteredPassword
    ) const;

    void display() const override;
};

class Donor : public User {
private:
    std::string donorType;

public:
    Donor(int id, std::string name, std::string contact,
          std::string addr, std::string type,
          int pts = 0, double fines = 0.0);

    std::string getDonorType() const;
    void display() const override;
};

class Recipient : public User {
protected:
    std::string acceptedDietType;

public:
    Recipient(int id, std::string name, std::string contact,
              std::string addr, std::string diet,
              int pts = 0, double fines = 0.0);

    std::string getAcceptedDietType() const;

    virtual bool isCompatible(
        const FoodItem& food,
        std::string& rejectionReason
    ) const = 0;
};

class NGO : public Recipient {
public:
    NGO(int id, std::string name, std::string contact,
        std::string addr, std::string diet,
        int pts = 0, double fines = 0.0);

    bool isCompatible(
        const FoodItem& food,
        std::string& rejectionReason
    ) const override;

    void display() const override;
};

class AnimalShelter : public Recipient {
private:
    std::string species;

public:
    AnimalShelter(int id, std::string name, std::string contact,
                  std::string addr, std::string spec,
                  int pts = 0, double fines = 0.0);

    std::string getSpecies() const;

    bool isCompatible(
        const FoodItem& food,
        std::string& rejectionReason
    ) const override;

    void display() const override;
};

#endif
