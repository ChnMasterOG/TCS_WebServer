#ifndef _MYSQLCONN_H
#define _MYSQLCONN_H

#include <iostream>
#include <mysql/mysql.h>
#include <vector>

#define SQL_HOST "localhost"
#define SQL_USERNAME "root"
#define SQL_PASSWORD "KE.yan_418"
#define SQL_DATABASE "WebServer"
#define SQL_PORT 3306

#define SQL_UNCONNECT 0
#define SQL_CONNECT 1

using namespace std;

struct clock_in_info;

class mysql_conn {
public:

    mysql_conn();
    ~mysql_conn() {}

    bool connect_database();
    bool query_sql_LOGIN(string& username, string& password, bool& is_valid);
    bool query_sql_TABLE(string& username, vector<vector<string>>& m_table);
    bool query_sql_IS_CLOCKIN(string& username, bool& is_signed);
    bool query_sql_CLOCKIN(clock_in_info& m_info);

    MYSQL m_conn;
	string m_host;
	int m_port;
	string m_user;
	string m_password;
	string m_databaseName;
    bool m_state;

};

struct clock_in_info {

    string username;
    string name;
    string city;
    float temperature;
    int sex;

};

#endif

