#include <sys/ioctl.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <ev.h>
#include <hiredis/hiredis.h>
#include <tr1/unordered_map>
#include <atomic>
#include <string>
#include <algorithm>
#include <set>
#include <iostream>
#include <fstream>
#include "nolockqueue.h"
using std::cout;
using std::cin;
using std::cerr;
using std::endl;
struct userinfo
{
	bool login_flag;																			//是否已经登录
	bool status;																				//准备好还是没有准备好
	time_t last_update_time;																	//心跳时间
	struct ev_io *writer;																		//写watcher
	struct ev_io *reader;																		//读watcher
	std::string user_str;																		//用户登录的名字
	std::string room_str;																		//用户所在的房间
	std::shared_ptr<nolockqueue<char>> messages;												//用户的待发送数据
};

struct roominfo
{
	std::atomic_uint total_num;																	//总人数
	std::atomic_uint ready_num;																	//ok的人的数量
	std::atomic_uint unready_num;																//不ok的人的数量
	std::set<int> room_users;																	//房间中的用户
};

static void time_cb(struct ev_loop *loop,struct ev_timer *timer,int events);					//定时器回调
static void read_cb(struct ev_loop *loop,struct ev_io *watcher,int events);						//可读回调
static void write_cb(struct ev_loop *loop,struct ev_io *watcher,int events);					//可写回调
static void accept_cb(struct ev_loop* loop,struct ev_io* watcher,int events);					//新连接到来时的回调
static void signal_cb(struct ev_loop * loop,struct ev_signal* siger,int events);				//监听信号回调
static void stdin_cb(struct ev_loop* loop,struct ev_io* watcher,int events);					//标准输入可读的回调

void logout(struct ev_loop* loop,int fd);														//玩家下线

void set_socket();																				//初始化主线程中的套接字，以及注册accept

void set_redis_connection();																	//设置与redis的连接

bool set_no_block(int&fd);																		//设置套接字非阻塞

void split_username_passwd(char*,int len,std::string &,std::string &);							//将用户名和密码分开，存入参数3和4

void create_room_number(std::string &);															//生成随机房间号

std::tr1::unordered_map<int,std::shared_ptr<userinfo>> users;									//用户列表fd---用户信息

std::tr1::unordered_map<std::string,std::shared_ptr<roominfo>> rooms;							//房间列表

std::set<std::string> login_users;																//登录的用户

redisContext * redis_c;																			//与redis的连接

redisReply * redis_r;																			//redis的回复

std::string notice_str;																			//公告信息

std::string version;																			//版本号
std::string ip;																					//ip地址
unsigned int port = 0;																			//端口号
float update_time = 0.5f;
enum config
{
	VERSION = 1,
	IP,
	PORT,
	TIME,
};
int main()
{
	//主函数
	std::ifstream infile;
	infile.open("AYO.conf",std::ios::in);
	std::string temp_str;
	int tag = 0;
	while(infile>>temp_str)
	{
		if(temp_str == "version")
			tag = VERSION;
		else if(temp_str == "ip")
			tag = IP;
		else if(temp_str == "port")
			tag = PORT;
		else if(temp_str == "time")
			tag = TIME;
		else
		{
			switch(tag)
			{
				case VERSION:version = temp_str;break;
				case IP:ip = temp_str;break;
				case PORT:port = std::stoi(temp_str);break;
				case TIME:update_time = std::stof(temp_str);break;
				default:
					break;
			}
		}
	}
	infile.close();
	cout<<"version: "<<version<<endl<<"ip: "<<ip<<endl<<"port: "<<port<<endl<<"update_time: "<<update_time<<endl;
	set_socket();
	return 0;
}
static void stdin_cb(struct ev_loop* loop,struct ev_io* watcher,int events)
{
	//读标准输入
	if(events & EV_READ)
	{
		//清除以前的通知消息
		char temp[2];
		if(read(watcher->fd,&temp[0],1) && read(watcher->fd,&temp[1],1))
		{
			if(temp[0] == '-')
			{
				switch(temp[1])
				{
					case 'm':
						{
							//发送通知消息
							char head;
							notice_str.clear();
							while(1)
							{
								//读取所有标准输入
								int tag = true;
								if(read(watcher->fd,&head,1) && head!=EOF && head!='\n')
								{
									//跳过开头的所有空格
									if(!tag || head != ' ')
									{
										tag = false;
										notice_str+=temp;
									}
								}
								else
									break;
							}
							//有通知消息要发送
							if(notice_str.size())
							{
								for(auto it = users.begin();it!=users.end();it++)
								{
									std::shared_ptr<char> temp_message_self = std::make_shared<char>('8');
									it->second->messages->push(temp_message_self);
									if(!ev_is_active(it->second->writer) && !ev_is_pending(it->second->writer))
										ev_io_start(loop,it->second->writer);
								}
							}
							else
								cout<<"message is empty!"<<endl;
						}
						break;
					case 'r':
						{
							//通知服务器重新读取配置
							char head;
							while(read(watcher->fd,&head,1) && head!=EOF && head!='\n')
							{
								//丢弃标准输入的剩下的全部输入
							}
							//重新读取
							std::ifstream infile;
							infile.open("AYO.conf",std::ios::in);
							std::string temp_str;
							int tag = 0;
							while(infile>>temp_str)
							{
								if(temp_str == "version")
									tag = VERSION;
								else if(temp_str == "ip")
									tag = IP;
								else if(temp_str == "port")
									tag = PORT;
								else if(temp_str == "time")
									tag = TIME;
								else
								{
									switch(tag)
									{
										case VERSION:version = temp_str;break;
										case IP:ip = temp_str;break;
										case PORT:port = std::stoi(temp_str);break;
										case TIME:update_time = std::stof(temp_str);break;
										default:
											break;
									}
								}
							}
							infile.close();
							cout<<"reload config!"<<endl;
							cout<<"version: "<<version<<endl<<"ip: "<<ip<<endl<<"port: "<<port<<endl<<"update_time: "<<update_time<<endl;
						}
						break;
					case 'p':
						{
							//查询有多少人在线
							char head;
							while(read(watcher->fd,&head,1) && head!=EOF && head!='\n')
							{
								//丢弃标准输入的剩下的全部输入
							}
							cout<<"Online User : "<<users.size()<<endl;
						}
						break;
					default:
						{
							//格式不正确
							cerr<<"read from stdin with wrong style!"<<endl;
							char head;
							while(read(watcher->fd,&head,1) && head!=EOF && head!='\n')
							{
								//丢弃所有的数据
							}
						}
						break;
				}
			}
			else
			{
				//格式不正确
				cerr<<"read from stdin with wrong style!"<<endl;
				char head;
				while(read(watcher->fd,&head,1) && head!=EOF && head!='\n')
				{
					//丢弃所有的数据
				}
			}
		}
	}
	else
		cerr<<"error happened on stdin_cb: wrong event!"<<endl;
}
static void time_cb(struct ev_loop * loop,struct ev_timer * timer,int events)
{
	//定时器回调，给每个在房间中的用户发送更新数据消息
	if(events &EV_TIMER)
	{
	for(auto it = users.begin();it!=users.end();)
	{
		time_t now_time = time(0);
		if((now_time - it->second->last_update_time)>=(time_t)10)
		{
			//超时断开该用户连接
			logout(loop,it->first);
		}
		else
		{
			//没有超时，如果该玩家在房间中,并且找到该房间
			if(it->second->room_str.size() && rooms.find(it->second->room_str)!=rooms.end())
			{
				std::shared_ptr<char> temp_message_self = std::make_shared<char>('9');
				it->second->messages->push(temp_message_self);
				if(!ev_is_active(it->second->writer) && !ev_is_pending(it->second->writer))
					ev_io_start(loop,it->second->writer);
			}
			it++;
		}
	}
	timer->repeat = update_time;
	ev_timer_again(loop,timer);
	}
	else
		cerr<<"error happened on time_cb: wrong event!"<<endl;
}
static void read_cb(struct ev_loop* loop,struct ev_io* watcher,int events)
{
	//read回调
	if(events & EV_READ)
	{
		char head;
		int recv_num = recv(watcher->fd,&head,1,MSG_PEEK);
		if(!recv_num)
		{
			//客户端下线
			cout<<"recv return 0,logout normally!"<<endl;
			logout(loop,watcher->fd);
		}
		else if(recv_num < 0)
		{
			//recv出错
			if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
			{
				cerr<<"error happened on recv!recv return < 0 and errno == EINTR || EWOULDBLOCK || EAGAIN!Ignore Ignore!"<<endl;
			}
			else if( errno == ECONNRESET || errno == EPIPE)
			{
				//收到RST或者产生SIGPIPE
				cerr<<"recv RST Or EPipe!"<<endl;
				logout(loop,watcher->fd);
			}
			else
			{
				cerr<<"error happened on recv!recv return < 0!Unknow error!Exit Exit!"<<endl;
				exit(1);
			}
		}
		else
		{
			//正常读消息
			switch(head)
			{
			case '0':
			{
				//消息头是‘0’,从准备好到没准备好
				recv(watcher->fd,&head,1,0);
				if(users.find(watcher->fd)!=users.end())
				{
					//更新心跳
					users[watcher->fd]->last_update_time = time(0);
					std::shared_ptr<userinfo> temp_userinfo_self = users[watcher->fd];
					//如果该用户在一个房间中，那么获得了房间
					if(temp_userinfo_self->room_str.size() && rooms.find(temp_userinfo_self->room_str)!=rooms.end())
					{
						std::shared_ptr<roominfo> temp_roominfo = rooms[temp_userinfo_self->room_str];
						if(temp_userinfo_self->status == true)
						{
							temp_roominfo->ready_num--;
							temp_roominfo->unready_num++;
							temp_userinfo_self->status = false;
							//用户自己
							std::shared_ptr<char> temp_message_self = std::make_shared<char>('0');
							temp_userinfo_self->messages->push(temp_message_self);
							//启动监听写
							if(!ev_is_active(temp_userinfo_self->writer) && !ev_is_pending(temp_userinfo_self->writer))
								ev_io_start(loop,temp_userinfo_self->writer);
						}
						else
							cerr<<"useless message!"<<endl;
					}
					else
						cerr<<"error happened on head 0!room does not exist!"<<endl;
				}
				else
					cerr<<"error happened on head 0!user does not exist!"<<endl;
			}
			break;
			case '1':
			{
				//消息头是‘1’,从没准备好到准备好
				recv(watcher->fd,&head,1,0);
				if(users.find(watcher->fd)!=users.end())
				{
					//更新心跳
					users[watcher->fd]->last_update_time = time(0);
					std::shared_ptr<userinfo> temp_userinfo_self = users[watcher->fd];
					//玩家有房间，并且找到了玩家所在的room
					if(temp_userinfo_self->room_str.size() && rooms.find(temp_userinfo_self->room_str)!=rooms.end())
					{
						std::shared_ptr<roominfo> temp_roominfo = rooms[temp_userinfo_self->room_str];
						if(temp_userinfo_self->status == false)
						{
							temp_roominfo->ready_num++;
							temp_roominfo->unready_num--;
							temp_userinfo_self->status = true;
							//用户自己
							std::shared_ptr<char> temp_message_self = std::make_shared<char>('1');
							temp_userinfo_self->messages->push(temp_message_self);
							//监听写
							if(!ev_is_active(temp_userinfo_self->writer) && !ev_is_pending(temp_userinfo_self->writer))
								ev_io_start(loop,temp_userinfo_self->writer);
						}
						else
							cerr<<"useless message!"<<endl;
					}
					else
						cerr<<"error happened on head 1!room does not exist!"<<endl;
				}
				else
					cerr<<"error happened on head 1!user does not exist!"<<endl;
			}
			break;
			case '2':
			{
				//消息头是'2',加入房间
				unsigned long len;
				ioctl(watcher->fd,FIONREAD,&len);
				if(len>=17)
				{
					//不存在粘包
					//存在这个用户
					if(users.find(watcher->fd)!=users.end())
					{
						//更新心跳
						users[watcher->fd]->last_update_time = time(0);
						std::shared_ptr<userinfo> temp_userinfo_self = users[watcher->fd];
						char temp_str[17];
						recv(watcher->fd,&head,1,0);
						recv(watcher->fd,temp_str,16,0);
						temp_str[16]='\0';
						std::string str = temp_str;
						//玩家没有房间，并且要求加入的房间存在
						if((!temp_userinfo_self->room_str.size()) && rooms.find(str)!=rooms.end()) 
						{
							std::shared_ptr<roominfo> temp_roominfo = rooms[str];
							if(temp_roominfo->room_users.insert(watcher->fd).second)
							{
								cout<<watcher->fd<<" join room "<<str.c_str()<<" success!"<<endl;
								//加入成功
								temp_userinfo_self->room_str = str;
								temp_roominfo->total_num++;
								temp_roominfo->ready_num++;
								temp_userinfo_self->status = true;
								//用户自己
								std::shared_ptr<char> temp_message_self = std::make_shared<char>('3');
								temp_userinfo_self->messages->push(temp_message_self);
								//启动监听 写
								if(!ev_is_active(temp_userinfo_self->writer) && !ev_is_pending(temp_userinfo_self->writer))
									ev_io_start(loop,temp_userinfo_self->writer);
							}
							else
							{
								//加入失败
								//将加入失败的消息发送出去
								cout<<watcher->fd<<" join room "<<str.c_str()<<" failed!insert into room failed!"<<endl;
								std::shared_ptr<char> temp_message_self = std::make_shared<char>('2');
								temp_userinfo_self->messages->push(temp_message_self);
								//启动监听 写
								if(!ev_is_active(temp_userinfo_self->writer) && !ev_is_pending(temp_userinfo_self->writer))
									ev_io_start(loop,temp_userinfo_self->writer);
							}
						}
						else
						{
							//加入失败
							cout<<watcher->fd<<" join room "<<str.c_str()<<" failed!already in a room or room doesn't exist!"<<endl;
							std::shared_ptr<char> temp_message_self = std::make_shared<char>('2');
							temp_userinfo_self->messages->push(temp_message_self);
							//启动监听 写
							if(!ev_is_active(temp_userinfo_self->writer) && !ev_is_pending(temp_userinfo_self->writer))
								ev_io_start(loop,temp_userinfo_self->writer);
						}
					}
					else
						cerr<<"error happened on head 2!user does not exist!"<<endl;
				}
			}
			break;
			case '3':
			{
				//消息头是‘3’，登录消息
				if(users.find(watcher->fd)!=users.end())
				{	//更新心跳
					users[watcher->fd]->last_update_time = time(0);
					char temp_len[2];
					if(recv(watcher->fd,temp_len,2,MSG_PEEK) == 2)
					{
						int len;
						ioctl(watcher->fd,FIONREAD,&len);
						if(len >= (temp_len[1] + 2))
						{
							//不存在粘包
							char username_passwd[temp_len[1]];
							recv(watcher->fd,temp_len,2,0);
							recv(watcher->fd,username_passwd,temp_len[1],0);
							std::string username;
							std::string passwd;
							split_username_passwd(username_passwd,sizeof(username_passwd),username,passwd);
							//查询redis帐号密码是否正确
							redis_r = (redisReply*)redisCommand(redis_c,"GET %s",username.c_str());
							if(!redis_r)
							{
								//redis查询出错
								cerr<<"redis-replay error!"<<endl;
								//清除出错的redis连接
								if(redis_c)
								{
									redisFree(redis_c);
									cerr<<"clean redis-connection!"<<endl;
								}
								//重建与redis的连接
								set_redis_connection();
								std::shared_ptr<userinfo> temp_userinfo_self = users[watcher->fd];
								//发送登录失败消息
								temp_userinfo_self->login_flag = false;
								std::shared_ptr<char> temp_message_self = std::make_shared<char>('5');
								temp_userinfo_self->messages->push(temp_message_self);
								//监听写
								if(!ev_is_active(temp_userinfo_self->writer) && !ev_is_pending(temp_userinfo_self->writer))
									ev_io_start(loop,temp_userinfo_self->writer);
							}
							else
							{
								//redis查询正确
								std::string temp_str(redis_r->str,redis_r->len);
								//该帐号还没登录
								if(login_users.find(username) == login_users.end())
								{
									//释放redisreplay对象
									freeReplyObject(redis_r);
									if(temp_str == passwd)
									{
										//验证成功
										cout<<"user login : "<<username.c_str()<<endl;
										std::shared_ptr<userinfo> temp_userinfo_self = users[watcher->fd];
										temp_userinfo_self->login_flag = true;
										temp_userinfo_self->user_str = username;
										login_users.insert(username);
										std::shared_ptr<char> temp_message_self = std::make_shared<char>('4');
										temp_userinfo_self->messages->push(temp_message_self);
										//监听写
										if(!ev_is_active(temp_userinfo_self->writer) && !ev_is_pending(temp_userinfo_self->writer))
											ev_io_start(loop,temp_userinfo_self->writer);
									}
									else
									{
										//验证失败，用户名密码错误
										cout<<"wrong username -- passwd!"<<endl;
										std::shared_ptr<userinfo> temp_userinfo_self = users[watcher->fd];
										//用户在线
										temp_userinfo_self->login_flag = false;
										std::shared_ptr<char> temp_message_self = std::make_shared<char>('5');
										temp_userinfo_self->messages->push(temp_message_self);
										//监听写
										if(!ev_is_active(temp_userinfo_self->writer) && !ev_is_pending(temp_userinfo_self->writer))
											ev_io_start(loop,temp_userinfo_self->writer);
									}
								}
								//该帐号已经登录
								else
								{
									cout<<"username was already login!"<<endl;
									freeReplyObject(redis_r);
									std::shared_ptr<userinfo> temp_userinfo_self = users[watcher->fd];
									temp_userinfo_self->login_flag = false;
									std::shared_ptr<char> temp_message_self = std::make_shared<char>('5');
									temp_userinfo_self->messages->push(temp_message_self);
									//监听写
									if(!ev_is_active(temp_userinfo_self->writer) && !ev_is_pending(temp_userinfo_self->writer))
										ev_io_start(loop,temp_userinfo_self->writer);
								}
							}
						}
					}
				}
				else
					cerr<<"error happened on head 3!user does not exist!"<<endl;
			}
			break;
			case '4':
			{
				//消息头是‘4’,注册消息
				if(users.find(watcher->fd)!=users.end())
				{
					//更新心跳
					users[watcher->fd]->last_update_time = time(0);
					char temp_len[2];
					if(recv(watcher->fd,temp_len,2,MSG_PEEK) == 2)
					{
						int len;
						ioctl(watcher->fd,FIONREAD,&len);
						if(len >= (temp_len[1] + 2))
						{
							//不存在粘包
							char username_passwd[temp_len[1]];
							recv(watcher->fd,temp_len,2,0);
							recv(watcher->fd,username_passwd,temp_len[1],0);
							std::string username;
							std::string passwd;
							split_username_passwd(username_passwd,sizeof(username_passwd),username,passwd);
							//查询redis帐号是否存在
							redis_r = (redisReply*)redisCommand(redis_c,"EXISTS %s",username.c_str());
							if(!redis_r)
							{
								//redis查询失败
								cerr<<"redis-replay error!"<<endl;
								//清除出错的redis连接
								if(redis_c)
								{
									redisFree(redis_c);
									cerr<<"clean redis-connection!"<<endl;
								}
								//重新建立redis连接
								set_redis_connection();
								std::shared_ptr<userinfo> temp_userinfo_self = users[watcher->fd];
								temp_userinfo_self->login_flag = false;
								std::shared_ptr<char> temp_message_self = std::make_shared<char>('6');
								temp_userinfo_self->messages->push(temp_message_self);
								//监听写
								if(!ev_is_active(temp_userinfo_self->writer) && !ev_is_pending(temp_userinfo_self->writer))
									ev_io_start(loop,temp_userinfo_self->writer);
							}
							else
							{
								//redis查询成功
								if(redis_r->integer == 1L)
								{
									//不可以注册,用户名已经存在
									cout<<"reg failed!name already exist!"<<endl;
									freeReplyObject(redis_r);
									std::shared_ptr<userinfo> temp_userinfo_self = users[watcher->fd];
									temp_userinfo_self->login_flag = false;
									std::shared_ptr<char> temp_message_self = std::make_shared<char>('6');
									temp_userinfo_self->messages->push(temp_message_self);
									//监听写
									if(!ev_is_active(temp_userinfo_self->writer) && !ev_is_pending(temp_userinfo_self->writer))
										ev_io_start(loop,temp_userinfo_self->writer);
								}
								else
								{	
									//可以注册,用户名还未注册
									cout<<"name can be regged!"<<endl;
									freeReplyObject(redis_r);
									//向redis申请注册
									redis_r = (redisReply*)redisCommand(redis_c,"SET %s %s",username.c_str(),passwd.c_str());
									if(!redis_r)
									{
										//reids查询失败
										cerr<<"redis-reply error!"<<endl;
										//清除出错的redis连接
										if(redis_c)
										{
											redisFree(redis_c);
											cerr<<"clean redis-connection!"<<endl;
										}
										//重建与redis的连接
										set_redis_connection();
										std::shared_ptr<userinfo> temp_userinfo_self = users[watcher->fd];
										temp_userinfo_self->login_flag = false;
										std::shared_ptr<char> temp_message_self = std::make_shared<char>('6');
										temp_userinfo_self->messages->push(temp_message_self);
										//监听写
										if(!ev_is_active(temp_userinfo_self->writer) && !ev_is_pending(temp_userinfo_self->writer))
											ev_io_start(loop,temp_userinfo_self->writer);
									}
									else
									{
										//redis查询成功
										std::string temp_str(redis_r->str,redis_r->len);
										//格式化返回字符串
										for(unsigned long i = 0;i<(temp_str.size()-1);i++)
										{
											temp_str[i] = toupper(temp_str[i]);
										}
										if(redis_r->type == REDIS_REPLY_STATUS && temp_str == "OK")
										{
											//创建成功
											cout<<"new user created!username : "<<username.c_str()<<" passwd : "<<passwd.c_str()<<endl;
											freeReplyObject(redis_r);
											std::shared_ptr<userinfo> temp_userinfo_self = users[watcher->fd];
											temp_userinfo_self->login_flag = true;
											temp_userinfo_self->user_str = username;
											login_users.insert(username);
											std::shared_ptr<char> temp_message_self = std::make_shared<char>('4');
											temp_userinfo_self->messages->push(temp_message_self);
											//监听写
											if(!ev_is_active(temp_userinfo_self->writer) && !ev_is_pending(temp_userinfo_self->writer))
												ev_io_start(loop,temp_userinfo_self->writer);
										}
										else
										{
											//创建用户失败，redis返回错误
											freeReplyObject(redis_r);
											cerr<<"error happened on reg user to redis!"<<endl;
											std::shared_ptr<userinfo> temp_userinfo_self = users[watcher->fd];
											temp_userinfo_self->login_flag = false;
											std::shared_ptr<char> temp_message_self = std::make_shared<char>('6');
											temp_userinfo_self->messages->push(temp_message_self);
											//监听写
											if(!ev_is_active(temp_userinfo_self->writer) && !ev_is_pending(temp_userinfo_self->writer))
												ev_io_start(loop,temp_userinfo_self->writer);
										}
									}
								}
							}
						}
					}
				}
				else
					cerr<<"error happened on head 4!user does not exist!"<<endl;
			}
			break;
			case '5':
			{
				//消息头是'5',创建房间消息
				recv(watcher->fd,&head,1,0);
				//找到用户
				if(users.find(watcher->fd)!=users.end())
				{
					//更新心跳
					users[watcher->fd]->last_update_time = time(0);
					std::shared_ptr<userinfo> temp_userinfo_self = users[watcher->fd];
					//用户处于登录状态
					if(temp_userinfo_self->login_flag)
					{
						std::string room_number;
						while(1)
						{
							create_room_number(room_number);
							//room可创建
							if(room_number.size() == 16 && rooms.find(room_number) == rooms.end())
							{
								//初始化一个房间
								std::shared_ptr<roominfo> temp_roominfo = std::make_shared<roominfo>();
								if(rooms.insert(std::pair<std::string,std::shared_ptr<roominfo>>(room_number,temp_roominfo)).second)
								{
									//成功将房间加入房间列表
									if(temp_roominfo->room_users.insert(watcher->fd).second)
									{
										//设置用户状态为准备好
										temp_userinfo_self->status = true;
										//加入房间成功
										//配置房间信息
										temp_roominfo->total_num=1;
										temp_roominfo->ready_num=1;
										temp_roominfo->unready_num=0;
										//设置用户房间字符串
										temp_userinfo_self->room_str = room_number;
										//给用户发消息
										std::shared_ptr<char> temp_message_self = std::make_shared<char>('7');
										temp_userinfo_self->messages->push(temp_message_self);
										//启动监听 写
										if(!ev_is_active(temp_userinfo_self->writer) && !ev_is_pending(temp_userinfo_self->writer))
											ev_io_start(loop,temp_userinfo_self->writer);
										cout<<"create room success!room number : "<<room_number.c_str()<<endl;
										break;
									}
									else
									{
										cerr<<"error happened on head 5!insert user to new room failed!"<<endl;
										//加入房间失败
										if(!rooms.erase(room_number))
											cerr<<"error happened on head 5!erase room failed after insert failed!"<<endl;
									}
								}
								else
									//加入房间失败
									cerr<<"error happened on head 5!insert room failed!"<<endl;
							}
							else
								cerr<<"error happened on head 5!wrong room number or room exist!"<<endl;
						}
					}
					else
						cerr<<"error happened on head 5!unlogin user want to create room!"<<endl;
				}
				else
					cerr<<"error happened on head 5!user does not exist!"<<endl;
			}
			break;
			case '6':
			{
				//消息头时‘6’，用户请求数据同步，同时维持心跳
				recv(watcher->fd,&head,1,0);
				if(users.find(watcher->fd)!=users.end())
					//更新心跳
					users[watcher->fd]->last_update_time = time(0);
				else
					cerr<<"error happened on head 5!user does not exist!"<<endl;
			}
			break;
			default:
			{
				//消息头不可识别，强制将人踢下线
				cout<<"error happened on read_cb!unknow head!"<<endl;
				logout(loop,watcher->fd);
				break;
			}
			}
		}
	}
	else
		cerr<<"error happened on read_cb: wrong event!"<<endl;
}
static void write_cb(struct ev_loop* loop,struct ev_io* watcher,int events)
{
	//write回调
	if(events & EV_WRITE)
	{
		//用户在线
		if(users.find(watcher->fd)!=users.end())
		{
			std::shared_ptr<userinfo> temp_userinfo_self = users[watcher->fd];
			std::shared_ptr<char> temp_message_self;
			while(1)
			{
				int rt = temp_userinfo_self->messages->pop(temp_message_self);
				if(rt>0)
				{
					//获取到消息
					switch (*temp_message_self) 
					{
						case '0':
						case '1':
						case '3':
						case '9'://头+ready+unready
							{
								if(rooms.find(temp_userinfo_self->room_str)!=rooms.end())
								{
									std::shared_ptr<roominfo> temp_roominfo = rooms[temp_userinfo_self->room_str];
									unsigned int ready_i = temp_roominfo->ready_num;
									unsigned int unready_i = temp_roominfo->unready_num;
									uint32_t ready = htonl(ready_i);
									uint32_t unready = htonl(unready_i);
									char buffer[9];
									buffer[0] = *temp_message_self;
									bcopy(&ready,&buffer[1],4);
									bcopy(&unready,&buffer[5],4);
									if(send(watcher->fd,buffer,9,0)<0)
										cerr<<"error at send to "<<watcher->fd<<" in 0139!"<<endl;
								}
								else
									cerr<<"error happened on head 0139 in write_cb!room does not exist!"<<endl;
							}
							break;
						case '2':
						case '4':
						case '5':
						case '6'://头
							{
								char buffer = *temp_message_self;
								send(watcher->fd,&buffer,1,0);
							}
							break;
						case '7'://创建房间消息
							{
								if(rooms.find(temp_userinfo_self->room_str)!=rooms.end())
								{
									std::shared_ptr<roominfo> temp_roominfo = rooms[temp_userinfo_self->room_str];
									char buffer[17];
									buffer[0]= *temp_message_self;
									for(int i=1;i<17;i++)
										buffer[i] = temp_userinfo_self->room_str[i-1];
									if(send(watcher->fd,&buffer,17,0)<0)
										cerr<<"error at send to "<<watcher->fd<<" in '7'!"<<endl;
								}
								else
									cerr<<"error happened on head 7 in write_cb!room does not exist!"<<endl;
							}
							break;
						case '8'://发送广播消息
							{
								unsigned int length = notice_str.size();
								uint32_t len = htonl(length);
								char buffer[5 + notice_str.size()];
								buffer[0] = *temp_message_self;
								bcopy(&len,&buffer[1],4);
								for(unsigned int i = 0;i<notice_str.size();i++)
								{
									buffer[i+5] = notice_str[i];
								}
								if(send(watcher->fd,buffer,(5+notice_str.size()),0)<0)
									cerr<<"error at send to "<<watcher->fd<<" in 8!"<<endl;
							}
							break;
						case 'a'://发送版本信息
							{
								uint32_t len = htonl(version.size());
								char buffer[5+version.size()];
								buffer[0] = *temp_message_self;
								bcopy(&len,&buffer[1],4);
								for(unsigned int i = 0;i<version.size();i++)
								{
									buffer[i+5] = version[i];
								}
								if(send(watcher->fd,buffer,(5+version.size()),0)<0)
									cerr<<"error at send to "<<watcher->fd<<" in a!"<<endl;
							}
							break;
						default:
							cerr<<"error happened on write_cb!unknow head!"<<endl;
							break;
					}
				}
				else if(rt == 0)
				{
					//没有消息待发送,停止监听写
					ev_io_stop(loop,watcher);
					break;
				}
				else
					//消息获取出错
					cerr<<"error happened on write_cb!get message from messagequeue failed!"<<endl;
			}
		}
		else
			cerr<<"error happened on write_cb!user does not exist!"<<endl;
	}
	else
		cerr<<"error happened on write_cb: wrong event!"<<endl;
}
static void accept_cb(struct ev_loop* loop,struct ev_io* watcher,int events)
{
	//主线程中accept回调
	if(events & EV_READ)
	{
		int client;
		struct sockaddr_in c_addr;
		socklen_t c_addr_len = sizeof(c_addr);
		client = accept(watcher->fd,(struct sockaddr *)&c_addr,&c_addr_len);
		if(client <0)
		{
			//accept出错
			if(errno == EWOULDBLOCK | errno == ECONNABORTED | errno == EPROTO | errno == EINTR)
				cerr<<"client abort to connect to server!"<<endl;
			else
			{
				cerr<<"unknow error happened on accept!"<<endl;
				exit(1);
			}
		}
		else if(client>=0)
		{
			//正常accept
			//创建一个用户
			std::shared_ptr<userinfo> temp_userinfo_self = std::make_shared<userinfo>();
			//初始化用户信息
			temp_userinfo_self->login_flag = false;
			temp_userinfo_self->status = true;
			temp_userinfo_self->last_update_time = time(0);
			temp_userinfo_self->messages = std::make_shared<nolockqueue<char>>();
			//初始化reader 监听可读
			temp_userinfo_self->reader = (ev_io*)malloc(sizeof(ev_io));
			ev_io_init(temp_userinfo_self->reader,read_cb,client,EV_READ);
			ev_io_start(loop,temp_userinfo_self->reader);
			//初始化writer
			temp_userinfo_self->writer = (ev_io*)malloc(sizeof(ev_io));
			ev_io_init(temp_userinfo_self->writer,write_cb,client,EV_WRITE);
			//将user插入users列表中
			if(!users.insert(std::pair<int,std::shared_ptr<userinfo>>(client,temp_userinfo_self)).second)
			{
				cerr<<"error happened on accept_cb!insert user failed!"<<endl;
				close(client);
			}
			else
			{
				cout<<"new client accept!"<<endl;
				std::shared_ptr<char> temp_message_self = std::make_shared<char>('a');
				temp_userinfo_self->messages->push(temp_message_self);
				//监听写
				if(!ev_is_active(temp_userinfo_self->writer) && !ev_is_pending(temp_userinfo_self->writer))
					ev_io_start(loop,temp_userinfo_self->writer);

			}
		}
	}
	else
	{
		cerr<<"error happened on accept_cb!"<<endl;
	}
}
static void signal_cb(struct ev_loop * loop,struct ev_signal* siger,int events)
{
	//信号回调
}
bool set_no_block(int &fd)
{
	//设置套接字为非阻塞模式
	int flags = fcntl(fd,F_GETFL);
	flags |= O_NONBLOCK;
	if(fcntl(fd,F_SETFL,flags)<0)
		return false;
	return true;
}
void set_socket()
{
	//忽略sigpipe信号
	struct sigaction act;
	act.sa_flags = SA_RESTART;
	act.sa_handler = SIG_IGN;
	sigaction(SIGPIPE,&act,NULL);
	//初始化 监听套接字
	int listener = 0;
	while(1)
	{
		listener = socket(AF_INET,SOCK_STREAM,0);
		if(listener<=0)
			cerr<<"error happened on create socket!"<<endl;
		else
		{
			if(!set_no_block(listener))
				cerr<<"error happened on set non-block!"<<endl;
			else
				break;
		}
		cerr<<"try to create new socket!"<<endl;
		sleep(1);
	}
	//设置地址重用
	const int reuse = 1;
	setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
	cout<<"create socket success!"<<endl;

	struct sockaddr_in s_addr;
	s_addr.sin_family = AF_INET;

	if(ip.size())
		inet_pton(AF_INET,ip.c_str(),(void*)&s_addr.sin_addr.s_addr);
	else
		s_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(!port)
		s_addr.sin_port = htons(port);
	else
		s_addr.sin_port = htons(38146);
	if(bind(listener,(sockaddr*)&s_addr,sizeof(s_addr))<0)
		cerr<<"error happened on bind listener!"<<endl;
	else
	{
		cout<<"bind success!"<<endl;
	}

	//监听
	if(listen(listener,SOMAXCONN)<0)
	{
		cerr<<"error happened on listen!"<<endl;
		exit(1);
	}
	else
		cout<<"listen success!"<<endl;

	//初始化与redis的连接
	set_redis_connection();
	//初始化 一个默认的loop
	struct ev_loop * main_loop;
	while(1)
	{
		main_loop = ev_default_loop();
		if(!main_loop)
			cerr<<"error happened on create main_loop!"<<endl;
		else
			break;
	}
	//初始化一个iowatcher
	ev_io *accept_io = (ev_io*)malloc(sizeof(ev_io));
	ev_io_init(accept_io,accept_cb,listener,EV_READ);
	ev_io_start(main_loop,accept_io);
	//监听标准输入
	ev_io* stdin_io = (ev_io*)malloc(sizeof(ev_io));
	ev_io_init(stdin_io,stdin_cb,stdin->_fileno, EV_READ);
	ev_io_start(main_loop,stdin_io);
	//初始化一个timer
	ev_timer * timer = (ev_timer*)malloc(sizeof(ev_timer));
	ev_init(timer,time_cb);
	timer->repeat = 1.0f;
	ev_timer_again(main_loop,timer);
	//初始化一个siger监听信号
	ev_signal * siger = (ev_signal * )malloc(sizeof(ev_signal));
	ev_signal_init(siger,signal_cb,SIGPIPE);
	ev_signal_start(main_loop,siger);
	//开始主循环
	ev_run(main_loop,0);
	free(accept_io);
	free(timer);
}
void set_redis_connection()
{
	//初始化与redis的通信
	redis_c = redisConnectUnix("/var/run/redis/redis_unix.socket");
	if(!redis_c || redis_c->err)
	{
		if(redis_c->err)
		{
			cerr<<"connect to redis failed!"<<endl;
			exit(1);
		}
		else
		{
			cerr<<"allocate redis_c failed!"<<endl;
			exit(1);
		}
	}
	else
		cout<<"connect redis success!"<<endl;
}
void split_username_passwd(char* str,int len,std::string & username,std::string & passwd)
{
	//分割用户名和密码
	bool split_flag = false;
	for(int i = 0;i<len;i++)
	{
		if(str[i] == '$')
		{
			split_flag = true;
			continue;
		}
		if(split_flag)
			passwd+=str[i];
		else
			username+=str[i];
	}
}
void create_room_number(std::string & room_number)
{
	//生成随机的room号码
	srand(time(0));
	room_number.clear();
	for(int i = 0;i<16;i++)
	{
		int type = rand()%3;
		switch(type)
		{
			case 0:
				{
					char a = rand()%10 + 48;
					room_number += a;
				}
				break;
			case 1:
				{
					char a = rand()%26 + 97;
					room_number += a;
				}
				break;
			case 2:
				{
					char a = rand()%26 + 65;
					room_number += a;
				}
				break;
		}
	}
}
void logout(struct ev_loop* loop,int fd)
{
	//找到这个用户
	if(users.find(fd)!=users.end())
	{
		std::shared_ptr<userinfo> temp_userinfo_self = users[fd];
		//清除用户数据
		ev_clear_pending(loop,temp_userinfo_self->writer);
		ev_clear_pending(loop,temp_userinfo_self->reader);
		ev_io_stop(loop,temp_userinfo_self->reader);
		ev_io_stop(loop,temp_userinfo_self->writer);
		free(temp_userinfo_self->reader);
		free(temp_userinfo_self->writer);
		//如果玩家登录了，那么删除
		if(temp_userinfo_self->user_str.size())
		{
			auto it = login_users.find(temp_userinfo_self->user_str);
			if(it != login_users.end())
			{
				cout<<"login user logout! "<<temp_userinfo_self->user_str.c_str()<<endl;
				login_users.erase(it);
			}
			else
				cerr<<"error happened logout!unlogin user have user_str!"<<endl;
		}
		else
			cout<<"temp user logout : "<<fd<<"!"<<endl;
		//关闭socket
		close(fd);
		//如果用户有房间
		if(temp_userinfo_self->room_str.size())
		{
			//找到房间
			if(rooms.find(temp_userinfo_self->room_str)!=rooms.end())
			{
				std::shared_ptr<roominfo> temp_roominfo = rooms[temp_userinfo_self->room_str];
				temp_roominfo->room_users.erase(fd);
				//如果房间人数为0，那么关闭房间
				if(!temp_roominfo->room_users.size())
				{
					rooms.erase(temp_userinfo_self->room_str);
					cout<<"last user leave!close room : "<<temp_userinfo_self->room_str.c_str()<<"!"<<endl;
				}
				//房间内还有人
				else
				{
					if(temp_userinfo_self->status)
						temp_roominfo->ready_num--;
					else
						temp_roominfo->unready_num--;
				}
			}
			else
				//没有找到房间
				cerr<<"error happened on logout!room does not exist!"<<endl;
		}
		//将用户从用户列表中删除
		users.erase(fd);
	}
	else
		cerr<<"error happened on logout!user does not exist!"<<endl;
}
