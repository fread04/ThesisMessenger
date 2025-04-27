#include "../include/client_handler.h"
#include "../include/auth.h"
#include "../include/db.h"

#include <tchar.h>
#include <mutex>
#include <unordered_map>
#include "../include/db_utils.h"
#include "../include/config.h"

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
                std::string msgContent = message.substr(spacePos + 1);

                // Сохраняем сообщение в БД
                SaveMessageToDB(username, recipient, msgContent);

                // Получаем сохраненное сообщение в нужном формате
                std::string formattedMessage = GetLastMessageFor(username, recipient);

                // Отправляем сообщение получателю
                std::lock_guard<std::mutex> lock(clientsMutex);
                if (clients.find(recipient) != clients.end()) {
                    send(clients[recipient], formattedMessage.c_str(), formattedMessage.length(), 0);
                }

                // Отправляем сообщение обратно отправителю
                send(clientSocket, formattedMessage.c_str(), formattedMessage.length(), 0);
                continue;
            }
        }

        // Для групповых сообщений (если нужно)
        SaveMessageToDB(username, "all", message);
        std::string formattedMessage = GetLastMessageFor(username, "all");

        // Рассылка всем клиентам
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (auto& client : clients) {
            send(client.second, formattedMessage.c_str(), formattedMessage.length(), 0);
        }
    }

    // Delete client
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients.erase(username);
    }

    closesocket(clientSocket);
}

std::string GetLastMessageFor(const std::string& sender, const std::string& recipient) {
    try {
        std::unique_ptr<sql::Connection> con = GetDBConnection();
        con->setSchema(config.get("Database", "database", "default_database"));

        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
            "SELECT m.timestamp, u1.username as sender, u1.id as sender_id, "
            "u2.username as receiver, u2.id as receiver_id, m.message "
            "FROM messages m "
            "JOIN users u1 ON m.sender_id = u1.id "
            "JOIN users u2 ON m.receiver_id = u2.id "
            "WHERE u1.username = ? AND u2.username = ? "
            "ORDER BY m.timestamp DESC LIMIT 1"));

        pstmt->setString(1, sender);
        pstmt->setString(2, recipient);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        if (res->next()) {
            return res->getString("timestamp") + " | " +
                res->getString("sender") + "(" + std::to_string(res->getInt("sender_id")) + ") -> " +
                res->getString("receiver") + "(" + std::to_string(res->getInt("receiver_id")) + "): " +
                res->getString("message");
        }
        return "";
    }
    catch (sql::SQLException& e) {
        std::cerr << "Ошибка MySQL: " << e.what() << std::endl;
        return "";
    }
}