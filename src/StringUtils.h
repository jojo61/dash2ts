#pragma once

#include <string>
#include <algorithm>

void ToLower(std::string &str);
std::string& TrimLeft(std::string &str, const char* const chars);
std::string& TrimRight(std::string &str, const char* const chars);
std::string& Trim(std::string &str, const char* const chars);
