class SockWatcher
{
	friend class Socket;
public:
	int add_fd();
	int check_fd();
	int del_fd();
	int get_fd();
	int init();
	int watch();

	int handle_recv();
	int handle_send();
	int handle_linger();
	int max_fd();
	
}


