#include"HandleClient.h"
using namespace std;



void HandleClient::init(){
    //1.创建通信的套接字
    cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cfd == -1) {
        perror("socket");
        exit(0);
    }

    //2.连接服务器
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(10000); //大端端口
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);


    int ret = connect(cfd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret == -1) {
        perror("connect");
        exit(0);
    }
}



void * HandleClient::handle_recv(void *arg){
    int sock=*(int *)arg;
    while(1){
        char recv_buffer[1000];
        memset(recv_buffer,0,sizeof(recv_buffer));
        int len=recv(sock,recv_buffer,sizeof(recv_buffer),0);
        if(len==0)
            continue;
        if(len==-1)
            break;
        string str(recv_buffer);
        cout<<str<<endl;
    }
}

void * HandleClient::handle_send(void *arg){
    int sock=*(int *)arg;
    while(1){
        string str;
        cin>>str;
        if(str=="exit")
            break;
        if(sock>0){
            str="content:"+str;
            send(sock,str.c_str(),str.length(),0);
        }
        else if(sock<0){
            str="gr_message:"+str;
            send(-sock,str.c_str(),str.length(),0);
        }   
    }   
}




void HandleClient::run(){
    int choice;
    string name,pass,pass1;
    bool if_login=false;//记录是否登录成功
    string login_name;//记录成功登录的用户名
   
    //先检查是否存在cookie文件
    ifstream f;
    f.open("cookie.txt", ios::in | ios::out);

    string cookie_str;
    if(f.good()){
        f>>cookie_str;
        f.close();
        cookie_str="cookie:"+cookie_str;
        cout << cookie_str << endl;
        //将cookie发送到服务器
        send(cfd,cookie_str.c_str(),cookie_str.length()+1,0);
        //接收服务器答复
        char cookie_ans[100];
        memset(cookie_ans,0,sizeof(cookie_ans));
        recv(cfd,cookie_ans,sizeof(cookie_ans),0);
        //判断服务器答复是否通过
        string ans_str(cookie_ans);
        cout << ans_str << endl;
        if(ans_str!="NULL"){//redis查询到了cookie，通过
            cout << 11 << endl;
            if_login=true;
            login_name=ans_str;
        }
        
    }    
    
 
    if(!if_login){
        cout<<" ------------------\n";
        cout<<"|                  |\n";
        cout<<"| 请输入你要的选项:|\n";
        cout<<"|    0:退出        |\n";
        cout<<"|    1:登录        |\n";
        cout<<"|    2:注册        |\n";
        cout<<"|                  |\n";
        cout<<" ------------------ \n\n";
    }

    //开始处理各种事务
    while(1){
        if(if_login)
            break;
        cin>>choice;
        if(choice==0)
            break;
        //登录
        else if(choice==1&&!if_login){
            while(1){
                cout<<"用户名:";
                cin>>name;
                cout<<"密码:";
                cin>>pass;
                string str="login"+name;
                str+="pass:";
                str+=pass;
                send(cfd,str.c_str(),str.length(),0);//发送登录信息
                char buffer[1000];
                memset(buffer,0,sizeof(buffer));
                recv(cfd,buffer,sizeof(buffer),0);//接收响应
                string recv_str(buffer);
                if(recv_str.substr(0,2)=="ok"){
                    if_login=true;
                    login_name=name;

                    //本地建立cookie文件保存sessionid
                    string tmpstr=recv_str.substr(2);
                    tmpstr="cat > cookie.txt <<end \n"+tmpstr+"\nend";
                    system(tmpstr.c_str()); //system会调执行系统命令

                    cout<<"登陆成功\n\n";
                    break;
                }
                else
                    cout<<"密码或用户名错误！\n\n";
            }       
        }
        //注册
        else if(choice==2){
            cout<<"注册的用户名:";
            cin>>name;
            while(1){
                cout<<"密码:";
                cin>>pass;
                cout<<"确认密码:";
                cin>>pass1;
                if(pass==pass1)
                    break;
                else
                    cout<<"两次密码不一致!\n\n";
            }   
            name="name:"+name;
            pass="pass:"+pass;
            string str=name+pass;
            send(cfd,str.c_str(),str.length(),0);
            cout<<"注册成功！\n";
            cout<<"\n继续输入你要的选项:";     
        }
        //char sendbuf[1000];
        //while(1){
        //    cin>>sendbuf;
        //    send(cfd, sendbuf, strlen(sendbuf),0); 
        //}       
        
        if(if_login)
            break;

    }
    //登陆成功
    while(if_login&&1){
        if(if_login){
            system("clear");
            cout<<"        欢迎回来,"<<login_name<<endl;
            cout<<" -------------------------------------------\n";
            cout<<"|                                           |\n";
            cout<<"|          请选择你要的选项：               |\n";
            cout<<"|              0:退出                       |\n";
            cout<<"|              1:发起单独聊天               |\n";
            cout<<"|              2:发起群聊                   |\n";
            cout<<"|                                           |\n";
            cout<<" ------------------------------------------- \n\n";
        }
        cin>>choice;
        pthread_t send_t,recv_t;//线程ID
        void *thread_return;
        if(choice==0)
            break;
        if(choice==1){
            cout<<"请输入对方的用户名:";
            string target_name,content;
            cin>>target_name;
            string sendstr("target:"+target_name+"from:"+login_name);//标识目标用户+源用户
            send(cfd,sendstr.c_str(),sendstr.length(),0);//先向服务器发送目标用户、源用户
            cout<<"请输入你想说的话(输入exit退出)：\n";
            auto send_thread=pthread_create(&send_t,NULL,handle_send,(void *)&cfd);//创建发送线程
            auto recv_thread=pthread_create(&recv_t,NULL,handle_recv,(void *)&cfd);//创建接收线程
            pthread_join(send_t,&thread_return);
            //pthread_join(recv_t,&thread_return);
            pthread_cancel(recv_t);
            /*
            线程取消的意思就是**在某些特定情况下在一个线程中杀死另一个线程**。使用这个函数杀死一个线程需要分两步：
​	            在线程A中**调用线程取消函数pthread_cancel**，指定杀死线程B，这时候线程B是死不了的
​	            在线程B中**进程一次系统调用（从用户区切换到内核区）**，否则线程B可以一直运行。
            */
        }    
        if(choice==2){
            cout<<"请输入群号:";
            int num;
            cin>>num;
            string sendstr("group:"+to_string(num));
            send(cfd,sendstr.c_str(),sendstr.length(),0);
            cout<<"请输入你想说的话(输入exit退出)：\n";
            int cfd1=-cfd;
            auto send_thread=pthread_create(&send_t,NULL,handle_send,(void *)&cfd1);//创建发送线程
            auto recv_thread=pthread_create(&recv_t,NULL,handle_recv,(void *)&cfd);//创建接收线程
            pthread_join(send_t,&thread_return);
            pthread_cancel(recv_t);
        }
    } 
    cout << 1 << endl;

}