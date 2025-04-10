#include "../include/auth.h"
#include "../include/db.h"
#include "../include/config.h"
#include "../include/db_utils.h"

bool UserExistsInDB(const std::string& username) {
    try {
        std::unique_ptr<sql::Connection> con = GetDBConnection();
        con->setSchema(config.get("Database", "database", "default_database"));

        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("SELECT id FROM users WHERE username = ?"));
        pstmt->setString(1, username);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        return res->next();
    }
    catch (sql::SQLException& e) {
        std::cerr << "Ошибка MySQL: " << e.what() << std::endl;
        return false;
    }
}

void RegisterUser(const std::string& username, const std::string& password_hash, const std::string& public_key) {
    try {
        std::unique_ptr<sql::Connection> con = GetDBConnection();
        con->setSchema(config.get("Database", "database", "default_database"));

        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("SELECT id FROM users WHERE username = ?"));
        pstmt->setString(1, username);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        if (res->next()) {
            std::cerr << "Ошибка: пользователь " << username << " уже существует в БД" << std::endl;
            return;
        }

        pstmt.reset(con->prepareStatement("INSERT INTO users (username, password_hash, public_key) VALUES (?, ?, ?)"));
        pstmt->setString(1, username);
        pstmt->setString(2, password_hash);
        pstmt->setString(3, public_key);
        pstmt->executeUpdate();
        std::cout << "Пользователь " << username << " зарегистрирован в БД" << std::endl;
    }
    catch (sql::SQLException& e) {
        std::cerr << "Ошибка MySQL: " << e.what() << std::endl;
    }
}

std::string CustomHash(const std::string& input) {
    std::string hash = input;
    char key = 0x5A;
    for (char& c : hash) {
        c ^= key;
    }
    return hash;
}

std::string HashPassword(const std::string& password) {
    return CustomHash(password);
}

bool VerifyPassword(const std::string& username, const std::string& inputPassword) {
    try {
        std::unique_ptr<sql::Connection> con = GetDBConnection();
        con->setSchema(config.get("Database", "database", "default_database"));

        std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement("SELECT password_hash FROM users WHERE username = ?"));
        pstmt->setString(1, username);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        if (res->next()) {
            std::string storedHash = res->getString("password_hash");
            std::string inputHash = CustomHash(inputPassword); // Hash password before comparison
            return storedHash == inputHash;
        }
    }
    catch (sql::SQLException& e) {
        std::cerr << "Ошибка MySQL: " << e.what() << std::endl;
    }
    return false;
}