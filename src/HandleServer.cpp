#include "HandleServer.h"

void HandleServer::run()
{
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
    {
        perror("socket");
        exit(0);
    }

    // 2. 将socket()返回值和本地的IP端口绑定到一起
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(10000);      // 大端端口
    addr.sin_addr.s_addr = INADDR_ANY; // 这个宏的值为0 == 0.0.0.0

    int ret = bind(lfd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1)
    {
        perror("bind");
        exit(0);
    }

    // 3. 设置监听
    ret = listen(lfd, 128);
    if (ret == -1)
    {
        perror("listen");
        exit(0);
    }

    sql_pool(); // 初始化连接池

    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);
    int cfd;
    boost::asio::thread_pool tp(8); // boost线程池

    while (cfd = accept(lfd, (struct sockaddr *)&cliaddr, &clilen))
    {

        cout << "用户" << inet_ntoa(cliaddr.sin_addr) << "正在连接:\n";
        boost::asio::post(boost::bind(&HandleServer::handle_all_request, this, cfd)); // 执行函数
    }
    tp.join(); // 释放线程池
}
void HandleServer::sql_pool()
{
    // 初始化数据库连接池
    m_sql_connPool = sql_connection_pool::GetInstance();
    m_sql_connPool->init("localhost", m_user, m_passWord, m_databaseName, stoi(m_port), m_sql_num);
    bloom_init();
    

    // 初始化redis连接池
    m_nosql_connPool = nosql_connection_pool::GetInstance();
    m_nosql_connPool->init(m_nosql_ip, m_nosql_port, m_nosql_num);
}

void HandleServer::bloom_init(){
    // 先从连接池中取一个连接
    MYSQL *mysql = NULL;
    sql_connectionRAII mysqlcon(&mysql, m_sql_connPool);

    string search = "SELECT * FROM user";

    auto search_res = mysql_query(mysql, search.c_str());
    MYSQL_RES *result = mysql_store_result(mysql);
    if (result == NULL)
    {
        printf("mysql_store_result() 失败了, 原因: %s\n", mysql_error(mysql));
    }



    MYSQL_ROW row;
    while( (row = mysql_fetch_row(result)) != NULL)
    {
        string temp1(row[0]);
        bloom_filter.set(hash_func(temp1));
    }

    mysql_free_result(result);
}

void HandleServer::test_redis()
{
    // 先从连接池中取一个连接
    redisContext *redis = NULL;
    nosql_connectionRAII rediscon(&redis, m_nosql_connPool);

    redisReply *reply = (redisReply *)redisCommand(redis, "set %s %s", "foo", "hello");
    printf("set:%s\n", reply->str);
    freeReplyObject(reply);
}

size_t HandleServer::hash_func(string name){
    size_t hash = 0;
    for (auto ch : name)
    {
        hash = hash * 131 + ch;
        if (hash >= 10000000)
            hash %= 10000000;
    }
    return hash;
}
void HandleServer::login_user(string str, int cfd, bool &if_login, string &login_name)
{

    // 先从连接池中取一个连接
    MYSQL *mysql = NULL;
    sql_connectionRAII mysqlcon(&mysql, m_sql_connPool);

    redisContext *redis = NULL;
    nosql_connectionRAII rediscon(&redis, m_nosql_connPool);

    int p1 = str.find("login"), p2 = str.find("pass:");
    string name = str.substr(p1 + 5, p2 - 5);
    string pass = str.substr(p2 + 5, str.length() - p2 - 4);

    if (!bloom_filter.isExists(hash_func(name)))
    {
        cout << "布隆过滤器查询为0,登录用户名必然不存在数据库中\n";
        char str1[100] = "wrong";
        send(cfd, str1, strlen(str1), 0);
        return;
    }

    string search = "SELECT * FROM user WHERE NAME=\"";
    search += name;
    search += "\";";
    cout << "sql语句:" << search << endl;

    auto search_res = mysql_query(mysql, search.c_str());
    MYSQL_RES *result = mysql_store_result(mysql);
    if (result == NULL)
    {
        printf("mysql_store_result() 失败了, 原因: %s\n", mysql_error(mysql));
    }

    int col = mysql_num_fields(result); // 获取列数
    int row = mysql_num_rows(result);   // 获取行数

    if (search_res == 0 && row != 0)
    {
        cout << "查询成功\n";
        // auto result=mysql_store_result(con);
        // int col=mysql_num_fields(result);//获取列数
        // int row=mysql_num_rows(result);//获取行数
        auto info = mysql_fetch_row(result); // 获取一行的信息
        cout << "查询到用户名:" << info[0] << " 密码:" << info[1] << endl;
        if (info[1] == pass)
        {
            cout << "登录密码正确\n";
            string str1 = "ok";
            if_login = true;
            login_name = name;
            mutx.lock();
            name_sock_map[name] = cfd; // 记录下名字和文件描述符的对应关系
            mutx.unlock();

            // 2020.12.9新添加：随机生成sessionid并发送到客户端
            srand(time(NULL)); // 初始化随机数种子
            for (int i = 0; i < 10; i++)
            {
                int type = rand() % 3; // type为0代表数字，为1代表小写字母，为2代表大写字母
                if (type == 0)
                    str1 += '0' + rand() % 9;
                else if (type == 1)
                    str1 += 'a' + rand() % 26;
                else if (type == 2)
                    str1 += 'A' + rand() % 26;
            }
            // 将sessionid存入redis
            string redis_str = "hset " + str1.substr(2) + " name " + login_name;
            redisReply *r = (redisReply *)redisCommand(redis, redis_str.c_str());
            freeReplyObject(r);
            // 设置生存时间,默认300秒
            redis_str = "expire " + str1.substr(2) + " 300";
            r = (redisReply *)redisCommand(redis, redis_str.c_str());
            freeReplyObject(r);

            cout << "随机生成的sessionid为：" << str1.substr(2) << endl;
            // cout<<"redis指令:"<<r->str<<endl;
            send(cfd, str1.c_str(), str1.length() + 1, 0);
        }
        else
        {
            cout << "登录密码错误\n";
            char str1[100] = "wrong";
            send(cfd, str1, strlen(str1), 0);
        }
    }
    else
    {
        cout << "查询失败\n";
        char str1[100] = "wrong";
        send(cfd, str1, strlen(str1), 0);
    }
    // 8. 释放资源 - 结果集
    mysql_free_result(result);
}

void HandleServer::register_user(string str, int cfd)
{
    // 先从连接池中取一个连接
    MYSQL *mysql = NULL;
    sql_connectionRAII mysqlcon(&mysql, m_sql_connPool);

    int p1 = str.find("name:"), p2 = str.find("pass:");
    string name = str.substr(p1 + 5, p2 - 5);
    string pass = str.substr(p2 + 5, str.length() - p2 - 4);
    string search = "INSERT INTO user VALUES (\"";
    search += name;
    search += "\",\"";
    search += pass;
    search += "\");";
    cout << endl
         << "sql语句:" << search << endl;
    int ret = mysql_query(mysql, search.c_str());
    if (ret != 0)
    {
        printf("mysql_query() 注册失败了, 原因: %s\n", mysql_error(mysql));
    }

    //设置布隆过滤器
    bloom_filter.set(hash_func(name));
}

void HandleServer::exits_user(string str, int cfd)
{
    // 先从连接池中取一个连接
    redisContext *redis = NULL;
    nosql_connectionRAII rediscon(&redis, m_nosql_connPool);

    string cookie = str.substr(7);
    string redis_str = "hget " + cookie + " name";

    redisReply *r = (redisReply *)redisCommand(redis, redis_str.c_str());
    string send_res;
    if (r->str)
    {
        cout << "查询redis结果：" << r->str << endl;
        send_res = r->str;
        // cout<<sizeof(r->str)<<endl;
        //  cout<<send_res.length()<<endl;
    }
    else
        send_res = "NULL";

    freeReplyObject(r);
    send(cfd, send_res.c_str(), send_res.length() + 1, 0);
}

void HandleServer::handle_all_request(int arg)
{

    int conn = arg;
    int target_conn = -1;
    char buffer[1000];
    string name, pass;
    bool if_login = false; // 记录当前服务对象是否成功登录
    string login_name;     // 记录当前服务对象的名字
    string target_name;    // 记录发送信息时目标用户的名字
    int group_num;         // 记录群号

    while (1)
    {
        cout << "-----------------------------\n";
        memset(buffer, 0, sizeof(buffer));
        int len = recv(conn, buffer, sizeof(buffer), 0);

        // 断开了连接或者发生了异常
        if (len == 0 || len == -1)
            break;

        string str(buffer);

        if (str.find("cookie:") != str.npos)
        {
            exits_user(str, conn);
        }

        // 登录
        else if (str.find("login") != str.npos)
        {
            login_user(str, conn, if_login, login_name);
        }

        // 注册
        else if (str.find("name:") != str.npos)
        {
            register_user(str, conn);
        }

        // 设定目标的文件描述符
        else if (str.find("target:") != str.npos)
        {
            int pos1 = str.find("from");
            string target = str.substr(7, pos1 - 7), from = str.substr(pos1 + 4);
            target_name = target;
            if (name_sock_map.find(target) == name_sock_map.end())
                cout << "源用户为" << login_name << ",目标用户" << target_name << "仍未登陆，无法发起私聊\n";
            else
            {
                cout << "源用户" << login_name << "向目标用户" << target_name << "发起的私聊即将建立";
                cout << ",目标用户的套接字描述符为" << name_sock_map[target] << endl;
                target_conn = name_sock_map[target];
            }
        }

        // 接收到消息，转发
        else if (str.find("content:") != str.npos)
        {
            if (target_conn == -1)
            {
                cout << "找不到目标用户" << target_name << "的套接字，将尝试重新寻找目标用户的套接字\n";
                if (name_sock_map.find(target_name) != name_sock_map.end())
                {
                    target_conn = name_sock_map[target_name];
                    cout << "重新查找目标用户套接字成功\n";
                }
                else
                {
                    cout << "查找仍然失败，转发失败！\n";
                    continue;
                }
            }
            string recv_str(str);
            string send_str = recv_str.substr(8);
            cout << "用户" << login_name << "向" << target_name << "发送:" << send_str << endl;
            send_str = "[" + login_name + "]:" + send_str;
            send(target_conn, send_str.c_str(), send_str.length(), 0);
        }

        // 绑定群聊号
        else if (str.find("group:") != str.npos)
        {
            string recv_str(str);
            string num_str = recv_str.substr(6);
            group_num = stoi(num_str);
            cout << "用户" << login_name << "绑定群聊号为：" << num_str << endl;
            group_mutx.lock();
            // group_map[group_num].push_back(conn);
            group_map[group_num].insert(conn);
            group_mutx.unlock();
        }

        // 广播群聊信息
        else if (str.find("gr_message:") != str.npos)
        {
            string send_str(str);
            send_str = send_str.substr(11);
            send_str = "[" + login_name + "]:" + send_str;
            cout << "群聊信息：" << send_str << endl;
            for (auto i : group_map[group_num])
            {
                if (i != conn)
                    send(i, send_str.c_str(), send_str.length(), 0);
            }
        }
    }

    close(conn);
}
