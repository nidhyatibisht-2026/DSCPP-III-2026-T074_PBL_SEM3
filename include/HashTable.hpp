#ifndef FOODFLOW_HASHTABLE_HPP
#define FOODFLOW_HASHTABLE_HPP

#include <string>

class HashTable {
private:
    struct HashNode {
        std::string key;
        std::string value;
        HashNode* next;

        HashNode(std::string k, std::string v)
            : key(k), value(v), next(nullptr) {}
    };

    static const int TABLE_SIZE = 101;
    HashNode* table[TABLE_SIZE];

    int hashFunction(const std::string& key) const;

public:
    HashTable();
    ~HashTable();

    HashTable(const HashTable&) = delete;
    HashTable& operator=(const HashTable&) = delete;

    void insert(const std::string& key, const std::string& value);
    std::string search(const std::string& key) const;
    std::string mapIngredientsToAllergens(
        const std::string& rawIngredients
    ) const;
};

#endif
