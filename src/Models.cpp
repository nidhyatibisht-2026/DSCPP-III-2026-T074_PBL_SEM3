#include "Models.hpp"

long long getCurrentEpochSeconds() {
    return duration_cast<seconds>(
        system_clock::now().time_since_epoch()
    ).count();
}
