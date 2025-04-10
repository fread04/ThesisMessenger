#pragma once
#include <iostream>

#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>

#pragma comment(lib, "mysqlcppconn.lib")
#pragma comment(lib, "mysqlcppconn-static.lib")
#pragma comment(lib, "ws2_32.lib")

void SaveMessageToDB(const std::string& sender, const std::string& recipient, const std::string& message);
std::string GetMessageHistory(const std::string& username);
