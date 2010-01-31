#ifndef _SOCKET_WATCHER_H
#define _SOCKET_WATCHER_H

#include "uosdef.h"
#include <sys/select.h>


#define FDW_READ 1
#define FDW_WRITE 2
#define FDW_RDWR 3


class SockWatcher
{
public:
    SockWatcher();
    ~SockWatcher();

    int add_fd(int fd, int rw);
    int check_fd(int fd, int rw);
    int del_fd(int fd);

    // 根据索引获取“有事件”的文件描述符
    int get_fd( int ridx );
    int init();
    int watch( long timeout_msecs );
    int maxfd();
    int size_limit();

private:
    fd_set	_master_rset;
    fd_set	_master_wset;
    fd_set	_work_rset;
    fd_set	_work_wset;

    int*	_select_fds;
    int*	_fd_idx;
    int*	_active_idx;

    int		_sock_num;

    int		_maxfd;
    bool	_maxfd_changed;

    int		_size;
};


#endif//(_SOCKET_WATCHER_H)



