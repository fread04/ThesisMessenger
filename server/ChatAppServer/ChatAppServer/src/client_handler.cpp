#include "../include/client_handler.h"
#include "../include/auth.h"
#include "../include/db.h"

#include <tchar.h>
#include <mutex>
#include <unordered_map>

std::unordered_map<std::string, SOCKET> clients;
std::mutex clientsMutex;

void HandleClientConnection(SOCKET clientSocket) {
    char buffer[4096];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0) {
        closesocket(clientSocket);
        return;
    }

    std::string credentials(buffer, bytesReceived);
    size_t separator = credentials.find(':');

    if (separator == std::string::npos) {
        std::cerr << "Ошибка: некорректный формат данных от клиента." << std::endl;
        closesocket(clientSocket);
        return;
    }

    std::string username = credentials.substr(0, separator);
    std::string password = credentials.substr(separator + 1);

    bool isNewUser = !UserExistsInDB(username);

    if (isNewUser) {
        std::cout << "Регистрируем нового пользователя: " << username << std::endl;
        RegisterUser(username, HashPassword(password), "public_key_example");
    }
    else {
        if (!VerifyPassword(username, password)) {
            std::cerr << "Ошибка аутентификации: " << username << std::endl;
            std::string errorMsg = "Ошибка аутентификации";
            send(clientSocket, errorMsg.c_str(), errorMsg.length(), 0);
            closesocket(clientSocket);
            return;
        }
    }

    // Add client
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients[username] = clientSocket;
    }

    std::cout << username << " успешно подключился." << std::endl;

    // Send history
    std::string history = GetMessageHistory(username);
    send(clientSocket, history.c_str(), history.length(), 0);

    while (true) {
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            std::cout << username << " отключился." << std::endl;
            break;
        }

        std::string message(buffer, bytesReceived);
        std::cout << "Message from " << username << ": " << message << std::endl;

        if (message[0] == '@') {
            size_t spacePos = message.find(' ');
            if (spacePos != std::string::npos) {
                std::string recipient = message.substr(1, spacePos - 1);
                std::string msgContent = "(private) " + username + ": " + message.substr(spacePos + 1);

                std::lock_guard<std::mutex> lock(clientsMutex);
                if (clients.find(recipient) != clients.end()) {
                    send(clients[recipient], msgContent.c_str(), msgContent.length(), 0);
                    SaveMessageToDB(username, recipient, message.substr(spacePos + 1));
                }
                else {
                    std::string errorMsg = "User " + recipient + " not found.";
                    send(clientSocket, errorMsg.c_str(), errorMsg.length(), 0);
                }
                continue;
            }
        }

        SaveMessageToDB(username, "all", message);
    }

    // Delete client
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients.erase(username);
    }

    closesocket(clientSocket);
}