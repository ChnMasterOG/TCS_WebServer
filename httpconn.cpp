
#include "httpconn.h"
#include "server.h"
#include "mysqlconn.h"
#include "md5.h"

extern mysql_conn mysqlconn;
extern map<string, string> md5_username;
extern map<string, string> username_md5;

void addfd(int epollfd, int fd, int TRIGMode, bool one_shot)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;

    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
}

void modfd(int epollfd, int fd, int ev, int TRIGMode, bool one_shot)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = ev | EPOLLET | EPOLLRDHUP;
    else
        event.events = ev | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;

    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

http_conn::http_conn(int fd, int epfd)
: m_TRIGMode(0), m_read_p(0), m_sockfd(fd), m_epollfd(epfd), m_state(STA_RECV), m_md5_flag(0), m_table_flag(0)
{
    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
    memset(m_fp_buf, '\0', FP_SIZE);
}

bool http_conn::_read()
{
    if (m_read_p >= READ_BUFFER_SIZE) return 1; //over buffersize
    int bytes_read = 0;

    if (0 == m_TRIGMode) {   //LT mode
        bytes_read = recv(m_sockfd, m_read_buf + m_read_p, READ_BUFFER_SIZE - m_read_p, 0);
        m_read_p += bytes_read; //move pointer
        if (bytes_read <= 0) return 1;
        return 0;
    }
    else {  //ET mode
        while (1) {
            bytes_read = recv(m_sockfd, m_read_buf + m_read_p, READ_BUFFER_SIZE - m_read_p, 0);
            if (bytes_read == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                return 1;
            }
            else if (bytes_read == 0) {
                return 1;
            }
            m_read_p += bytes_read;
        }
        return 0;
    }
}

bool http_conn::send_html()
{
    if (!strlen(m_fp_buf)) return 1;
	int html_fd = open(m_fp_buf, O_RDONLY);
	int send_size = sendfile(m_sockfd, html_fd, 0, 2048);	//zero copy
    memset(m_fp_buf, '\0', FP_SIZE);
	close(html_fd);
    return 0;
}

bool http_conn::send_response()
{
    int except_send_size = strlen(m_write_buf);
	int send_size = send(m_sockfd, m_write_buf, except_send_size, 0);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
	if (send_size != except_send_size) return 1;
	return 0;
}

bool http_conn::send_table()
{
    if (!strlen(m_fp_buf)) return 1;
    int html_fd = open(m_fp_buf, O_RDONLY);
    char m_fbuf[2048] = {0};
    read(html_fd, m_fbuf, 2048);
    string _buf(m_fbuf);
    int _pos = _buf.find("<!--insert-->");
    if (_pos == _buf.npos) return 1;
    string _table;
    _table = "<div align=\"center\"><table border=\"1\"><tr><th>Name</th><th>Sex(0-man, 1-woman)</th><th>City</th><th>Date</th><th>Temperature</th></tr>";
    for (auto _row : m_table) {
        _table += "<tr>";
        for (string _element : _row) {
            _table += "<td>" + _element + "</td>";
        }
        _table += "</tr>";
    }
    _table += "</table></div>";
    _buf.replace(_pos, 13, _table);
    int send_size = send(m_sockfd, _buf.c_str(), _buf.size(), 0);
    if (send_size != _buf.size()) return 1;
	return 0;
}

bool http_conn::check_cookies(string& _username)
{
    char _cookies_str[] = "sid=";
    char *p = strstr(m_read_buf, _cookies_str);
    if (p == nullptr) {
        _username = "";
        return 0;
    }
    p += strlen(_cookies_str);
    char* _cookies_end = strpbrk(p, "\r\n");
    *_cookies_end = '\0';
    string _md5 = p;
    *_cookies_end = '\n';
    if (md5_username.find(_md5) == md5_username.end()) return 1;
    _username = md5_username[_md5];
    return 0;
}

char* http_conn::search_configuration(char* _buf, string field, string& value)
{
    char *p = strstr(_buf, field.c_str());
    p += field.size();
    char *p_end = strpbrk(p, "&\r\n");
    char temp;
    if (p_end) {
        temp = *p_end;
        *p_end = '\0';
    }
    value = p;
    if (p_end) *p_end++ = temp;
    return p_end;
}

http_conn::HTTP_CODE http_conn::check_head(http_conn::METHOD& m_method, string& m_url)
{
    //analyze method
    char* p_start = m_read_buf;
    char* p = strpbrk(p_start, " \t");
    *p = '\0';
    if (strcasecmp(p_start, "GET") == 0) m_method = GET;
    else if (strcasecmp(p_start, "POST") == 0) m_method = POST;
    else return BAD_REQUEST;
    *p++ = ' ';
    //analyze url
    p_start = p;
    p = strpbrk(p_start, " \t");
    *p = '\0';
    if (strcasecmp(p_start, "/") == 0 || strcasecmp(p_start, "/login") == 0 || 
    strcasecmp(p_start, "/platform_b") == 0 || strcasecmp(p_start, "/platform_d") == 0) {
        m_url = p_start;
    }
    else return NO_RESOURCE;
    *p++ = ' ';
    //analyze version
    p_start = p;
    p = strpbrk(p_start, " \t\r\n");
    *p = '\0';
    if (strcasecmp(p_start, "HTTP/1.1") != 0) return BAD_REQUEST;
    *p++ = ' ';
    return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::do_request()
{
    METHOD m_method;
    string m_url;
    HTTP_CODE head_info = check_head(m_method, m_url);
    if (head_info != NO_REQUEST) return head_info;
    string m_username;  //cookies username
    bool cookies_invalid = check_cookies(m_username);
    if (m_url == "/") {
        if (m_method == GET) {
            strcpy(m_fp_buf, "log.html");
            return NO_REQUEST;
        }
    }
    else if (m_url == "/login") {
        if (m_method == POST) {
            string _username;   //new username
            string _password;
            char* p = search_configuration(m_read_buf, "user=", _username);
            p = search_configuration(p, "password=", _password);
            bool _is_valid = 0;
            if (mysqlconn.query_sql_LOGIN(_username, _password, _is_valid)) return BAD_REQUEST;
            else {
                if (_is_valid) {
                    //add cookies
                    time_t _now = time(0);
                    tm *ltm = localtime(&_now);
                    int _year, _month, _day;
                    _year = 1900 + ltm->tm_year;
                    _month = 1 + ltm->tm_mon;
                    _day = ltm->tm_mday;
                    string md5_value = MD5(_username + "-" + to_string(_year) + "-" + to_string(_month) + "-" + to_string(_day)).toString();
                    if (username_md5.find(_username) != username_md5.end()) {
                        auto it = md5_username.find(username_md5[_username]);
                        if (it != md5_username.end()) md5_username.erase(it);
                    }
                    md5_username[md5_value] = _username;
                    username_md5[_username] = md5_value;
                    m_md5 = md5_value;
                    //handle data
                    bool _is_clockin = 0;
                    if (mysqlconn.query_sql_IS_CLOCKIN(_username, _is_clockin)) return BAD_REQUEST;
                    else {
                        if (!_is_clockin) strcpy(m_fp_buf, "platform_a.html");
                        else strcpy(m_fp_buf, "platform_b.html");
                        m_md5_flag = 1;
                    }
                }
                else strcpy(m_fp_buf, "logError.html");
            }
            return NO_REQUEST;
        }
    }
    else if (m_url == "/platform_b") {
        if (m_method == POST) {
            if (cookies_invalid) return BAD_REQUEST;
            bool _is_clockin = 0;
            if (mysqlconn.query_sql_IS_CLOCKIN(m_username, _is_clockin)) return BAD_REQUEST;
            else {
                if (_is_clockin) strcpy(m_fp_buf, "platform_b.html");
                else {
                    clock_in_info m_info;
                    string _sex;
                    string _temperature;
                    m_info.username = m_username;
                    char* p = search_configuration(m_read_buf, "name=", m_info.name);
                    p = search_configuration(p, "sex=", _sex);
                    m_info.sex = stoi(_sex);
                    p = search_configuration(p, "city=", m_info.city);
                    p = search_configuration(p, "temperature=", _temperature);
                    m_info.temperature = stoi(_temperature);
                    if (mysqlconn.query_sql_CLOCKIN(m_info)) return BAD_REQUEST;
                    strcpy(m_fp_buf, "platform_c.html");
                }
            }
            return NO_REQUEST;
        }
    }
    else if (m_url == "/platform_d") {
        if (m_method == POST) {
            strcpy(m_fp_buf, "platform_d.html");
            m_table.clear();
            if (mysqlconn.query_sql_TABLE(m_username, m_table)) return BAD_REQUEST;
            else m_table_flag = 1;
            return NO_REQUEST;
        }
    }
    return FORBIDDEN_REQUEST;
}

bool http_conn::do_response(http_conn::HTTP_CODE http_code)
{
    switch (http_code)
    {
        case NO_REQUEST:
            strcpy(m_write_buf, _HTTP_200_and_Close);
            return 0;
        case BAD_REQUEST:
            strcpy(m_write_buf, _HTTP_400_and_Close);
            return 0;
        case FORBIDDEN_REQUEST:
            strcpy(m_write_buf, _HTTP_403_and_Close);
            return 0;
        case NO_RESOURCE:
            strcpy(m_write_buf, _HTTP_404_and_Close);
            return 0;
        default:
            return 1;
    }
}

bool http_conn::process_read()
{
    bool _is_fail = _read();
    cout << "OK: received" << endl;
    m_read_p = 0;
    return _is_fail;
}

bool http_conn::process_write()
{
    send_response();
    if (m_table_flag) send_table();
    else send_html();
    cout << "OK: send" << endl;
    close_connection(m_epollfd, m_sockfd);
    return 0;
}

void http_conn::process()
{
    if (m_state == STA_RECV) {
        process_read();
        //cout << m_read_buf << endl;
        HTTP_CODE http_code = do_request();
        if (!m_md5_flag) {
            do_response(http_code);
        }
        else {  //add cookies
            string _buf = "HTTP/1.1 200 OK\r\nset-Cookie:sid=" + m_md5 + ";path=/\r\nconnection: close\r\n\r\n";
            strcpy(m_write_buf, _buf.c_str());
        }
        memset(m_read_buf, '\0', READ_BUFFER_SIZE);
        m_state = STA_SEND;
        modfd(m_epollfd, m_sockfd, EPOLLOUT, m_TRIGMode);
    }
    else {
        process_write();
    }
}

