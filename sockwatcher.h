class SockWatcher
{
	friend class Socket;
public:
	int watch_all();
	int chk_sock();
	int del_sock();
	int add_sock();
	int handle_recv();
	int handle_send();
	int handle_linger();
	int max_fd();
	
}