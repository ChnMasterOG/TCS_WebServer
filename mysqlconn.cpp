#include "mysqlconn.h"

mysql_conn::mysql_conn()
: m_host(SQL_HOST), m_user(SQL_USERNAME), m_password(SQL_PASSWORD), m_port(SQL_PORT), m_databaseName(SQL_DATABASE), m_state(SQL_UNCONNECT)
{
    mysql_init(&m_conn);
}

bool mysql_conn::connect_database()
{
    if (mysql_real_connect(&m_conn, m_host.c_str(), m_user.c_str(), m_password.c_str(), m_databaseName.c_str(), m_port, NULL, 0)) {
        m_state = SQL_CONNECT;
        return 0;
    }
    else return 1;
}

bool mysql_conn::query_sql_LOGIN(string& username, string& password, bool& is_valid)
{
    string query_sequence = "SELECT id FROM user_tbl WHERE username = \"" + username + "\" and password = \"" + password + "\";";
    if (!m_state) return 1;
    int res = mysql_query(&m_conn, query_sequence.c_str());
    if (res) {
        mysql_close(&m_conn);
        return 1;
    }
    else {
        MYSQL_RES *res_ptr;
        res_ptr = mysql_store_result(&m_conn);
        if (res_ptr) {
            if (!res_ptr->row_count) is_valid = 0;
            else is_valid = 1;
            mysql_free_result(res_ptr);
        } 
        else return 1;
    }
    return 0;
}

bool mysql_conn::query_sql_TABLE(string& username, vector<vector<string>>& m_table)
{
    string query_sequence = "SELECT name, sex, city, dt, t FROM clock_in_tbl WHERE username = \"" + username + "\";";
    if (!m_state) return 1;
    int res = mysql_query(&m_conn, query_sequence.c_str());
    if (res) {
        mysql_close(&m_conn);
        return 1;
    }
    else {
        MYSQL_RES *res_ptr;
        MYSQL_ROW record;
        res_ptr = mysql_store_result(&m_conn);
        if (res_ptr) {
            while((record = mysql_fetch_row(res_ptr))) {
                m_table.push_back(vector<string>());
                for (int i = 0; i < 5; i++) m_table.back().emplace_back(record[i]);
            }
        }
        else return 1;
    }
    return 0;
}

bool mysql_conn::query_sql_IS_CLOCKIN(string& username, bool& is_signed)
{
    string query_sequence = "SELECT id FROM clock_in_tbl WHERE username = \"" + username + "\" and DATE(dt) = CURRENT_DATE;";
    if (!m_state) return 1;
    int res = mysql_query(&m_conn, query_sequence.c_str());
    if (res) {
        mysql_close(&m_conn);
        return 1;
    }
    else {
        MYSQL_RES *res_ptr;
        res_ptr = mysql_store_result(&m_conn);
        if (res_ptr) {
            if (!res_ptr->row_count) is_signed = 0;
            else is_signed = 1;
            mysql_free_result(res_ptr);
        } 
        else return 1;
    }
    return 0;
}

bool mysql_conn::query_sql_CLOCKIN(clock_in_info& m_info)
{
    string query_sequence = "INSERT INTO clock_in_tbl(username,name,sex,city,dt,t) VALUES(\"";
    query_sequence += m_info.username + "\",\"";
    query_sequence += m_info.name + "\",";
    query_sequence += to_string(m_info.sex) + ",\"";
    query_sequence += m_info.city + "\",NOW(),";
    query_sequence += to_string(m_info.temperature) + ");";
    if (!m_state) return 1;
    int res = mysql_query(&m_conn, query_sequence.c_str());
    if (res) {
        mysql_close(&m_conn);
        return 1;
    }
    return 0;
}
