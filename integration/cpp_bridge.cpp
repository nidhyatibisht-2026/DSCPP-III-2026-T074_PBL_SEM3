#include "FoodRescueSystem.hpp"

#include <memory>
#include <string>

#if defined(_WIN32)
#define FOODFLOW_API extern "C" __declspec(dllexport)
#else
#define FOODFLOW_API extern "C" __attribute__((visibility("default")))
#endif

namespace {
std::unique_ptr<FoodRescueSystem> systemInstance;

const char* safe(const char* value) {
    return value ? value : "";
}
}

FOODFLOW_API int ff_initialize() {
    try {
        if (!systemInstance) {
            systemInstance = std::make_unique<FoodRescueSystem>();
            systemInstance->setInteractiveMode(false);
        }
        return 1;
    } catch (...) {
        systemInstance.reset();
        return 0;
    }
}

FOODFLOW_API void ff_shutdown() {
    systemInstance.reset();
}

FOODFLOW_API void ff_purge_expired() {
    if (systemInstance) systemInstance->autoPurgeExpiredFood();
}

FOODFLOW_API int ff_register_user(
    const char* role, const char* name, const char* contact,
    const char* address, const char* subtype) {
    if (!systemInstance) return -1;

    const std::string userRole = safe(role);
    if (userRole == "donor") {
        return systemInstance->registerDonor(
            safe(name), safe(contact), safe(address), safe(subtype));
    }
    if (userRole == "ngo") {
        return systemInstance->registerNGO(
            safe(name), safe(contact), safe(address), safe(subtype));
    }
    if (userRole == "shelter") {
        return systemInstance->registerAnimalShelter(
            safe(name), safe(contact), safe(address), safe(subtype));
    }
    return -1;
}

FOODFLOW_API int ff_donate_food(
    int donorId, const char* name, const char* category, int quantity,
    int shelfHours, const char* dietType, const char* ingredients,
    const char* spice, const char* notes) {
    if (!systemInstance) return -1;
    return systemInstance->donateFood(
        donorId, safe(name), safe(category), quantity, shelfHours,
        safe(dietType), safe(ingredients), safe(spice), safe(notes));
}

FOODFLOW_API int ff_request_food(
    int recipientId, const char* foodName, int quantity,
    const char* allergies) {
    if (!systemInstance) return -1;
    return systemInstance->requestFood(
        recipientId, safe(foodName), quantity, safe(allergies));
}

FOODFLOW_API int ff_cancel_donation(int foodId, int donorId) {
    if (!systemInstance) return 0;
    return systemInstance->cancelDonation(foodId, donorId) ? 1 : 0;
}

FOODFLOW_API int ff_confirm_delivery(int orderId, int userId, int isDonor) {
    if (!systemInstance) return 0;
    systemInstance->confirmDelivery(orderId, userId, isDonor != 0);
    return 1;
}

FOODFLOW_API int ff_file_complaint(
    int orderId, int recipientId, const char* details) {
    if (!systemInstance) return 0;
    systemInstance->fileSpoiledFoodComplaint(
        orderId, recipientId, safe(details));
    return 1;
}
