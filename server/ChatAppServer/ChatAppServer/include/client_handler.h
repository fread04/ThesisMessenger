#pragma once
#include <iostream>

#include <winsock2.h>

void HandleClientConnection(SOCKET clientSocket);
std::string GetLastMessageFor(const std::string& sender, const std::string& recipient);