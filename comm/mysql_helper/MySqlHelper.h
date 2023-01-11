//
// Created by haichao.zhang on 2022/4/2.
//

#ifndef TRADEBEE_MYSQLHELPER_H
#define TRADEBEE_MYSQLHELPER_H

#include "jdbc/mysql_driver.h"
#include "jdbc/mysql_connection.h"
#include "jdbc/cppconn/driver.h"
#include "jdbc/cppconn/statement.h"
#include "jdbc/cppconn/prepared_statement.h"
#include "jdbc/cppconn/metadata.h"
#include "jdbc/cppconn/exception.h"
#include <utility>
#include <vector>

class MySqlHelper {
public:
    MySqlHelper(std::string host, std::string user,std::string password)
    :host(std::move(host)), user(std::move(user)), password(std::move(password))
    {


    }

    ~MySqlHelper()
    {
        conn->close();
    }

    std::shared_ptr<sql::Connection> InitMysqlClientConn()
    {
        auto driver = sql::mysql::get_mysql_driver_instance();
        if (!driver)
        {
            printf("create mysql conn fail: driver nullptr\n");
            return nullptr;
        }
        conn =  std::shared_ptr<sql::Connection>(driver->connect(host, user, password));
        if (!conn)
        {
            printf("create mysql conn fail: conn nullptr\n");
        }
        return conn;
    }


private:
    std::string host;
    std::string user;
    std::string password;

    std::shared_ptr<sql::Connection> conn;




};


#endif //TRADEBEE_MYSQLHELPER_H
