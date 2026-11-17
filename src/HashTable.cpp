#include "HashTable.hpp"
#include "Utilities.hpp"

#include <cctype>
#include <string>
#include <vector>

HashTable::HashTable() {
    for (int i = 0; i < TABLE_SIZE; ++i) {
        table[i] = nullptr;
    }
}

HashTable::~HashTable() {
    for (int i = 0; i < TABLE_SIZE; ++i) {
        HashNode* entry = table[i];

        while (entry) {
            HashNode* previous = entry;
            entry = entry->next;
            delete previous;
        }

        table[i] = nullptr;
    }
}

int HashTable::hashFunction(const std::string& key) const {
    unsigned long hash = 5381;

    for (unsigned char ch : key) {
        hash = ((hash << 5) + hash)
             + static_cast<unsigned char>(std::tolower(ch));
    }

    return static_cast<int>(hash % TABLE_SIZE);
}

void HashTable::insert(
    const std::string& key,
    const std::string& value
) {
    const std::string normalizedKey = toLower(trim(key));
    const int index = hashFunction(normalizedKey);
    HashNode* entry = table[index];

    while (entry) {
        if (entry->key == normalizedKey) {
            entry->value = value;
            return;
        }
        entry = entry->next;
    }

    HashNode* newNode = new HashNode(normalizedKey, value);
    newNode->next = table[index];
    table[index] = newNode;
}

std::string HashTable::search(const std::string& key) const {
    const std::string normalizedKey = toLower(trim(key));
    const int index = hashFunction(normalizedKey);
    HashNode* entry = table[index];

    while (entry) {
        if (entry->key == normalizedKey) {
            return entry->value;
        }
        entry = entry->next;
    }

    return "";
}

std::string HashTable::mapIngredientsToAllergens(
    const std::string& rawIngredients
) const {
    const std::vector<std::string> tokens = splitTokens(rawIngredients);
    std::string detectedAllergens;

    for (const auto& token : tokens) {
        const std::string allergen = search(token);

        if (!allergen.empty() &&
            detectedAllergens.find(allergen) == std::string::npos) {
            if (!detectedAllergens.empty()) {
                detectedAllergens += ", ";
            }
            detectedAllergens += allergen;
        }
    }

    return detectedAllergens;
}
