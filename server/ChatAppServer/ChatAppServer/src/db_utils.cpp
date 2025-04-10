#include "../include/db_utils.h"
#include "../include/config.h"

#include <memory>
#include <mysql_driver.h>
#include <mysql_connection.h>

std::unique_ptr<sql::Connection> GetDBConnection() {
    std::string server_host = config.get("Server", "host", "default_server_host");
    std::string server_port = config.get("Server", "port", "default_server_port");
    
    std::string db_host = config.get("Database", "host", "default_db_host");
    std::string db_port = config.get("Database", "port", "default_db_port");
    std::string db_user = config.get("Database", "user", "default_db_user");
    std::string db_password = config.get("Database", "password", "default_db_password");

    std::string db_url = "tcp://" + db_host + ":" + db_port;

    sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
    return std::unique_ptr<sql::Connection>(driver->connect(db_url, db_user, db_password));
}
