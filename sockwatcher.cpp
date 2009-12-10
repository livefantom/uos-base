
int SockWatcher::watch()
{
	
}

int SockWatcher::add_sock(int fd, int rw)
{
	// if socket number already reached the max limit.
	if ( _sock_num >= _size )
	{
		printf("too many sockets in add_sock!\n");
		return -1;
	}
	// add socket to the set.
	select_fds[_sock_num] = fd;
	switch( rw )
	{
		case READ:
			break;
		case WRITE:
			break;
		default:
			break;
	}
	if (fd > _maxfd)
		_maxfd = fd;
	// recode this socket index in the set.
	_fd_idx[fd] = _sock_num;
	++_sock_num;
}

int SockWatch::del_sock( int fd )
{
	int idx = _fd_idx[fd];
	if ( idx < 0 || idx > _size )
	{
		printf("bad index `%d' in del_sock!\n", idx);
		return -1;
	}
	--_sock_num;
	// move the last socket to this pos.
	select_fds[idx] = select_fds[_sock_num];
	// change index record of the last socket to current value.
	_fd_idx[select_fds[idx]] = idx;
	// reset the last record of the set.
	select_fds[_sock_num] = -1;
	// clear index record.
	_fd_idx[fd] = -1;
	
	FD_CLR();
	FD_CLR();
	
	if ( fd >= _maxfd )
		_maxfd_changed = true;

}

int SockWatch::get_maxfd()
{
	if ( true == _maxfd_changed )
	{
		_maxfd = -1;
		for ( int i = 0; i < _sock_num; ++i )
		{
			if ( select_fds[i] > _maxfd )
				_maxfd = select_fds[i];
		}
		_maxfd_changed = false;
	}
	return _max_fd;
}