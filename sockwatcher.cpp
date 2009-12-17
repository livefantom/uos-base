#include "sockwatcher.h"

SockWatcher::SockWatcher()
{
    size_limit();
    init();
}

SockWatcher::~SockWatcher()
{
}


int SockWatcher::add_fd( int fd, int rw )
{
    // if socket number already reached the max limit.
    if ( _sock_num >= _size )
    {
        printf("too many sockets in add_sock! count = %d\n", _sock_num);
        return -1;
    }
    // add socket to the set.
    _select_fds[_sock_num] = fd;
    switch ( rw )
    {
    case FDW_READ:
        FD_SET( fd, &_master_rset );
        break;
    case FDW_WRITE:
        FD_SET( fd, &_master_wset );
        break;
    case FDW_RDWR:
        FD_SET( fd, &_master_rset );
        FD_SET( fd, &_master_wset );
    	break;
    default:
        break;
    }
    if (fd > _maxfd)
        _maxfd = fd;
    // recode this socket index in the set.
    if ( _fd_idx[fd] < 0 || _fd_idx[fd] > _size )
    {
	    _fd_idx[fd] = _sock_num;
    }
    ++_sock_num;

//	printf("SockWatcher::add_fd [%04d]=%d, rw=%d, num=%d\n", fd, _fd_idx[fd], rw, _sock_num);
    return 1;
}

int SockWatcher::check_fd( int fd, int rw )
{
    switch ( rw )
    {
    case FDW_READ:
        return FD_ISSET( fd, &_work_rset );
    case FDW_WRITE:
        return FD_ISSET( fd, &_work_wset );
    default:
        return 0;
    }
}

int SockWatcher::del_fd( int fd )
{
    int idx = _fd_idx[fd];

    if ( idx < 0 || idx > _size )
    {
        printf("bad index `%d' of file `%d' in del_fd!\n", idx, fd);
        return -1;
    }

    --_sock_num;
    // move the last socket to this pos.
    _select_fds[idx] = _select_fds[_sock_num];
    // change index record of the last socket to current value.
    _fd_idx[ _select_fds[idx] ] = idx;
    // reset the last record of the set.
    _select_fds[_sock_num] = -1;
    // clear index record.
    _fd_idx[fd] = -1;

    FD_CLR( fd, &_master_rset );
    FD_CLR( fd, &_master_wset );

    if ( fd >= _maxfd )
        _maxfd_changed = true;

//	printf("SockWatcher::del_fd [%04d]=%d, [%d]=%d, num=%d\n", fd, _fd_idx[fd], _select_fds[idx], idx, _sock_num);
    return 1;
}

int SockWatcher::get_fd( int ridx )
{
    if ( ridx < 0 || ridx >= _size )
    {
        printf("bad ridx (%d) in get_fd", ridx);
        return -1;
    }
    return _fd_idx[ridx];
}

int SockWatcher::init()
{
    FD_ZERO( &_master_rset );
    FD_ZERO( &_master_wset );
    _select_fds = new int[_size];
    _fd_idx = new int[_size];
    _active_idx = new int[_size];
    _sock_num = 0;
    _maxfd = -1;
    _maxfd_changed = false;
    for ( int i=0; i<_size; ++i )
    {
        _select_fds[i] = _fd_idx[i] = -1;
    }
    return 1;
}

int SockWatcher::watch( long timeout_msecs )
{
    int nready;

    _work_rset = _master_rset;
    _work_wset = _master_wset;
    maxfd();

    if ( -1 == timeout_msecs )
    {
        nready = select( _maxfd+1, &_work_rset, &_work_wset, (fd_set*)0, (struct timeval*)0 );
    }
    else
    {
        struct timeval timeout;
        timeout.tv_sec = timeout_msecs / 1000L;
        timeout.tv_usec = ( timeout_msecs % 1000L ) * 1000L;
        nready = select( _maxfd +1, &_work_rset, &_work_wset, (fd_set*)0, &timeout );
    }
    return nready;
    /*
    	if (nready <= 0 )
    		return nready;

    	int j = 0;
    	for (int i = 0; i < sock_num; ++i)
    	{
    		if ( check_fd( _select_fds[i] ) )
    		{
    			_active_idx[j++] = _select_fds[i];
    			if (j == nready)
    			{
    				break;
    			}
    		}
    	}
    	return j;	// j should be equal to nready.
    */
}

int SockWatcher::maxfd()
{
    if ( true == _maxfd_changed )
    {
        _maxfd = -1;
        for ( int i = 0; i < _sock_num; ++i )
        {
            if ( _select_fds[i] > _maxfd )
                _maxfd = _select_fds[i];
        }
        _maxfd_changed = false;
    }
    return _maxfd;
}

int SockWatcher::size_limit()
{
	return _size = FD_SETSIZE;
}



