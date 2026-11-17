#include "Utilities.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <limits>
#include <sstream>

using namespace std;

string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == string::npos) return "";
    return s.substr(start, end - start + 1);
}

string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return static_cast<char>(tolower(c)); });
    return s;
}

vector<string> splitTokens(const string& str) {
    vector<string> tokens;
    stringstream ss(str);
    string token;

    while (getline(ss, token, ',')) {
        string t = toLower(trim(token));
        if (!t.empty() && t != "none") {
            tokens.push_back(t);
        }
    }
    return tokens;
}

bool isFoodNameMatch(const string& requestedName,
                     const string& availableFoodName) {
    vector<string> requestedTokens;
    vector<string> availableTokens;

    auto tokenizeWords = [](const string& name) {
        vector<string> words;
        string word;

        for (unsigned char ch : toLower(name)) {
            if (isalnum(ch)) {
                word += static_cast<char>(ch);
            } else if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }

        if (!word.empty()) {
            words.push_back(word);
        }

        return words;
    };

    requestedTokens = tokenizeWords(requestedName);
    availableTokens = tokenizeWords(availableFoodName);

    if (requestedTokens.empty() || availableTokens.empty()) {
        return false;
    }

    // Every requested word must appear in the available food name.
    for (const string& requestedWord : requestedTokens) {
        bool found = false;

        for (const string& availableWord : availableTokens) {
            if (requestedWord == availableWord) {
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }
    }

    return true;
}

int readInt(const string& prompt) {
    int value;

    while (true) {
        cout << prompt;
        if (cin >> value) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        }

        cout << "[Error] Invalid input. Enter a valid number.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

int readMenuChoice(const string& prompt, int minVal, int maxVal) {
    while (true) {
        int choice = readInt(prompt);
        if ((choice >= minVal && choice <= maxVal) || choice == 999) {
            return choice;
        }

        cout << "[Error] Choice out of range. Enter a number between "
             << minVal << " and " << maxVal << ".\n";
    }
}

int readPositiveInt(const string& prompt) {
    while (true) {
        int value = readInt(prompt);
        if (value > 0) return value;
        cout << "[Error] Value must be greater than 0.\n";
    }
}

string readNonEmptyLine(const string& prompt) {
    string s;

    while (true) {
        cout << prompt;
        getline(cin, s);
        s = trim(s);

        if (!s.empty()) return s;
        cout << "[Error] Input cannot be blank. Enter valid text.\n";
    }
}
