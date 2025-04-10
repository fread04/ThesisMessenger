#pragma once
#include <iostream>

bool UserExistsInDB(const std::string& username);
void RegisterUser(const std::string& username, const std::string& password_hash, const std::string& public_key);
bool VerifyPassword(const std::string& username, const std::string& inputPassword);
std::string CustomHash(const std::string& input);
std::string HashPassword(const std::string& password);
