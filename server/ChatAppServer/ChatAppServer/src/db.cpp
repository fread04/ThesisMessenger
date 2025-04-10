#include "../include/db.h"
#include "../include/config.h"
#include "../include/db_utils.h"

void SaveMessageToDB(const std::string& sender, const std::string& recipient, const std::string& message) {
    try {
        std::unique_ptr<sql::Connection> con = GetDBConnection();
        con->setSchema(config.get("Database", "database", "default_database"));

        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("SELECT id FROM users WHERE username = ?"));
        pstmt->setString(1, sender);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        int sender_id = -1;
        if (res->next()) {
            sender_id = res->getInt("id");
        }
        else {
            std::cerr << "Ошибка: отправитель " << sender << " не найден в БД" << std::endl;
            return;
        }

        pstmt.reset(con->prepareStatement("SELECT id FROM users WHERE username = ?"));
        pstmt->setString(1, recipient);
        res.reset(pstmt->executeQuery());

        int receiver_id = -1;
        if (res->next()) {
            receiver_id = res->getInt("id");
        }
        else {
            std::cerr << "Ошибка: получатель " << recipient << " не найден в БД" << std::endl;
            return;
        }

        pstmt.reset(con->prepareStatement("INSERT INTO messages (sender_id, receiver_id, message) VALUES (?, ?, ?)"));
        pstmt->setInt(1, sender_id);
        pstmt->setInt(2, receiver_id);
        pstmt->setString(3, message);
        pstmt->executeUpdate();

        std::cout << "Сообщение сохранено: " << sender << " -> " << recipient << " : " << message << std::endl;
    }
    catch (sql::SQLException& e) {
        std::cerr << "Ошибка MySQL: " << e.what() << std::endl;
    }
}

std::string GetMessageHistory(const std::string& username) {
    try {
        std::unique_ptr<sql::Connection> con = GetDBConnection();
        con->setSchema(config.get("Database", "database", "default_database"));

        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
            "SELECT sender_id, receiver_id, message, timestamp FROM messages WHERE sender_id = (SELECT id FROM users WHERE username = ?) OR receiver_id = (SELECT id FROM users WHERE username = ?) ORDER BY timestamp ASC"));
        pstmt->setString(1, username);
        pstmt->setString(2, username);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        std::string history;
        while (res->next()) {
            // Get usernames by their IDs
            int sender_id = res->getInt("sender_id");
            int receiver_id = res->getInt("receiver_id");

            std::unique_ptr<sql::PreparedStatement> senderStmt(con->prepareStatement("SELECT username FROM users WHERE id = ?"));
            senderStmt->setInt(1, sender_id);
            std::unique_ptr<sql::ResultSet> senderRes(senderStmt->executeQuery());
            std::string sender_name = senderRes->next() ? senderRes->getString("username") : "Unknown";

            std::unique_ptr<sql::PreparedStatement> receiverStmt(con->prepareStatement("SELECT username FROM users WHERE id = ?"));
            receiverStmt->setInt(1, receiver_id);
            std::unique_ptr<sql::ResultSet> receiverRes(receiverStmt->executeQuery());
            std::string receiver_name = receiverRes->next() ? receiverRes->getString("username") : "Unknown";

            // Form history
            history += res->getString("timestamp") + " | " + sender_name + "(" + std::to_string(sender_id) + ") -> "
                + receiver_name + "(" + std::to_string(receiver_id) + "): "
                + res->getString("message") + "\n";
        }
        return history;
    }
    catch (sql::SQLException& e) {
        return "Ошибка загрузки истории: " + std::string(e.what());
    }
}