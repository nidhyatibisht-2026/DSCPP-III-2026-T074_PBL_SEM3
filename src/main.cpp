#include "FoodRescueSystem.hpp"
#include "Utilities.hpp"
#include <iostream>
#include <string>
#include <cstdlib>
using namespace std;

int main() {
    FoodRescueSystem system;
    int choice;

    do {
        cout << "\n============================================\n";
        cout << "  FOOD SURPLUS MANAGEMENT AND REDISTRIBUTION\n";
        cout << "============================================\n";
        cout << "1. Register Donor\n";
        cout << "2. Register NGO\n";
        cout << "3. Register Animal Shelter\n";
        cout << "4. Donate Food\n";
        cout << "5. Request Food\n";
        cout << "6. Cancel Donation\n";
        cout << "7. Confirm Delivery\n";
        cout << "8. Report Spoiled Food\n";
        cout << "9. View Notifications\n";
        cout << "10. View Order Transaction Ledger\n";
        cout << "0. Exit\n";
        cout << "Choice (999 for Admin): ";

        choice = readMenuChoice("Enter choice: ", 0, 10);

    if (choice == 999) {
        const char* adminUsername = std::getenv("FOODFLOW_ADMIN_USERNAME");
        const char* adminPassword = std::getenv("FOODFLOW_ADMIN_PASSWORD");

        if (adminUsername == nullptr || adminPassword == nullptr) {
            cout << "[Access Denied] Admin credentials are not configured.\n";
            continue;
       }

        string enteredUsername = readNonEmptyLine("Admin username: ");
        string enteredPassword = readNonEmptyLine("Admin password: ");

        Admin admin(
            0,
            "System Administrator",
            "",
            "",
            adminUsername,
            adminPassword
        );

        if (!admin.authenticate(enteredUsername, enteredPassword)) {
            cout << "[Access Denied] Invalid administrator credentials.\n";
            continue;
        }

        cout << "\n[Access Granted] Welcome, Administrator.\n";

        int adminChoice;

            do {
                cout << "\n========== ADMIN MENU ==========\n";
                cout << "1. Process Matching Pipeline\n";
                cout << "2. Display Food Inventory\n";
                cout << "3. Display Expiry Min Heap\n";
                cout << "4. Display Custom FIFO Queue\n";
                cout << "5. Display Registered Users\n";
                cout << "6. Run Expiry Sweep\n";
                cout << "7. Remove Food Item\n";
                cout << "8. Cancel Recipient Request\n";
                cout << "9. Display Complete Food History\n";
                cout << "10. Display Order Transaction History\n";
                cout << "11. Deactivate / Reactivate User\n";
                cout << "12. Display Complete Food Request History\n";
                cout << "0. Return to Main Menu\n";

                adminChoice = readMenuChoice("Admin choice: ", 0, 12);

                switch (adminChoice) {
                    case 1:
                        system.processMatchingPipeline();
                        break;

                    case 2:
                        system.adminDisplayInventory();
                        break;

                    case 3:
                        system.adminDisplayUrgencyHeap();
                        break;

                    case 4:
                        system.adminDisplayRequestQueue();
                        break;

                    case 5:
                        system.adminDisplayUsers();
                        break;

                    case 6:
                        system.autoPurgeExpiredFood();
                        break;

                    case 7: {
                        int foodId = readPositiveInt("Food ID to remove: ");
                        system.adminRemoveFood(foodId);
                        break;
                    }

                    case 8: {
                        int requestId = readPositiveInt("Request ID to cancel: ");
                        system.adminCancelRequest(requestId);
                        break;
                    }

                    case 9:
                        system.adminDisplayFoodHistory();
                        break;

                    case 10:
                        system.displayOrderTransactions();
                        break;

                    case 11: {
                       int userId = readMenuChoice("Enter User ID: ", 1, 1000000000);

                       cout << "\n1. Deactivate User\n";
                       cout << "2. Reactivate User\n";

                       int statusChoice = readMenuChoice("Choose action: ", 1, 2);

                       system.adminSetUserActive(userId, statusChoice == 2);
                       break;
                    }

                    case 12:
    system.adminDisplayRequestHistory();
    break;

                    case 0:
                        break;
                }

            } while (adminChoice != 0);

            continue;
        }

        switch (choice) {
            case 0:
                cout << "Exiting FoodFlow. Saving records...\n";
                break;

            case 1: {
                string name = readNonEmptyLine("Donor name: ");
                string phone = readValidPhoneNumber("10-digit phone: ", system);
                string address = readNonEmptyLine("Address: ");
                string type = readNonEmptyLine("Donor type: ");
                int id = system.registerDonor(name, phone, address, type);
                cout << "Donor registered. ID: " << id << '\n';
                break;
            }

            case 2: {
                string name = readNonEmptyLine("NGO name: ");
                string phone = readValidPhoneNumber("10-digit phone: ", system);
                string address = readNonEmptyLine("Address: ");
                string diet = readNonEmptyLine("Accepted diet (veg/non-veg): ");
                int id = system.registerNGO(name, phone, address, diet);
                cout << "NGO registered. ID: " << id << '\n';
                break;
            }

            case 3: {
                string name = readNonEmptyLine("Shelter name: ");
                string phone = readValidPhoneNumber("10-digit phone: ", system);
                string address = readNonEmptyLine("Address: ");
                string species = readNonEmptyLine("Animal species: ");
                int id = system.registerAnimalShelter(name, phone, address, species);
                cout << "Animal shelter registered. ID: " << id << '\n';
                break;
            }

            case 4: {
                int donorId = readPositiveInt("Donor ID: ");
                if (!system.userExists(donorId)) {
                    cout << "[Error] User ID not found.\n";
                    break;
                }

                string name = readNonEmptyLine("Food name: ");
                string category = readNonEmptyLine("Food category: ");
                int quantity = readPositiveInt("Quantity: ");
                int hours = readPositiveInt("Hours until expiry: ");
                string diet = readNonEmptyLine("Diet type (veg/non-veg/plain): ");
                string ingredients = readNonEmptyLine("Ingredients (comma-separated, or none): ");
                string spice = readNonEmptyLine("Spice level: ");
                string notes = readNonEmptyLine("Additional notes: ");

                system.donateFood(donorId, name, category, quantity,
                                  hours, diet, ingredients, spice, notes);
                break;
            }

            case 5: {
                int recipientId = readPositiveInt("Recipient ID: ");
                if (!system.recipientExists(recipientId)) {
                    cout << "[Error] Recipient ID not found.\n";
                    break;
                }

                string foodName = readNonEmptyLine("Requested food: ");
                int quantity = readPositiveInt("Requested quantity: ");
                string allergies = readNonEmptyLine("Allergies (comma-separated, or none): ");

                system.requestFood(recipientId, foodName, quantity, allergies);
                break;
            }
            case 6: {
                int foodId = readPositiveInt("Food ID to cancel: ");
                int donorId = readPositiveInt("Donor ID: ");
                system.cancelDonation(foodId, donorId);
                break;
            }

            case 7: {
                int orderId = readPositiveInt("Order ID: ");
                int userId = readPositiveInt("Your user ID: ");
                cout << "Confirming as: 1. Donor  2. Recipient\n";
                int roleChoice = readMenuChoice("Role: ", 1, 2);
                string role = (roleChoice == 1) ? "Donor" : "Recipient";
                system.confirmDelivery(orderId, userId, roleChoice == 1);
                break;
            }

            case 8: {
                int orderId = readPositiveInt("Order ID: ");
                int recipientId = readPositiveInt("Recipient ID: ");
                string details = readNonEmptyLine("Complaint details: ");
                system.fileSpoiledFoodComplaint(orderId, recipientId, details);
                break;
            }

            case 9: {
                int userId = readPositiveInt("User ID: ");
                system.displayUserNotifications(userId);
                break;
            }

            case 10:
                system.displayOrderTransactions();
                break;

            default:
                cout << "[Error] Invalid menu choice.\n";
                break;
        }
    } while (choice != 0);

    return 0;
}