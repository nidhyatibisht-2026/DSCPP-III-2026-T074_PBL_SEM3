#ifndef FOODFLOW_UTILITIES_HPP
#define FOODFLOW_UTILITIES_HPP

#include <string>
#include <vector>

std::string trim(const std::string& s);
std::string toLower(std::string s);
std::vector<std::string> splitTokens(const std::string& str);

bool isFoodNameMatch(const std::string& requestedName,
                     const std::string& availableFoodName);

int readInt(const std::string& prompt);
int readMenuChoice(const std::string& prompt, int minVal, int maxVal);
int readPositiveInt(const std::string& prompt);
std::string readNonEmptyLine(const std::string& prompt);

#endif
