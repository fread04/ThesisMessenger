#pragma once

#include <cppconn/connection.h>

std::unique_ptr<sql::Connection> GetDBConnection();
