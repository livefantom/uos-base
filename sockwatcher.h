#ifndef _SOCKET_WATCHER_H
#define _SOCKET_WATCHER_H

#include "uosdef.h"
#include <sys/select.h>


#define FDW_READ 1
#define FDW_WRITE 2


class SockWatcher
{
public:
    SockWatcher();
    ~SockWatcher();

    int add_fd(int fd, int rw);
    int check_fd(int fd, int rw);
    int del_fd(int fd);

    // ����������ȡ�����¼������ļ�������
    int get_fd( int ridx );
    int init();
    int watch( long timeout_msecs );
    int maxfd();

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



