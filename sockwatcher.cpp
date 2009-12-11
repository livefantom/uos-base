
int SockWatcher::watch()
{
	
}

int SockWatcher::add_fd(int fd, int rw)
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
		case FDW_READ:
			break;
		case FDW_WRITE:
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

int SockWatch::check_fd( int fd )
{
	switch( _fd_rw[fd] )
	{
		case FDW_READ:
			return FD_ISSET( fd, &working_rfdset );
		case FDW_WRITE:
			return FD_ISSET( fd, &working_wfdset );
		default:
			return 0;
	}
}

int SockWatch::del_fd( int fd )
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
	
	FD_CLR( fd, &master_rfdset );
	FD_CLR( fd, &master_wfdset );
	
	if ( fd >= _maxfd )
		_maxfd_changed = true;
	
	return 1;

}

int SockWatch::get_fd( int ridx )
{
	if ( ridx < 0 || ridx >= _size )
	{
		printf("bad ridx (%d) in get_fd", ridx);
		return -1;
	}
	return _fd_idx[ridx];
}

int SockWatch::init()
{
	FD_ZERO( &master_rfdset );
	FD_ZERO( &master_wfdset );
	select_fds = new int[_size];
	_fd_idx = new int[_szie];
	_active_idx = new int[_size];
	_sock_num = 0;
	_maxfd = -1;
	_maxfd_changed = false;
	for( int i=0; i<_size; ++i )
	{
		select_fds[i] = _fd_idx[i] = -1;
	}
	return 1;
}

int SockWatch::watch( long timeout_msecs )
{
	int max_fd = get_maxfd();
	if ( -1 == timeout_msecs )
	{
		nready = select( max_fd+1, &working_rfdset, &working_wfdset, (fd_set*)0, (struct timeval*)0 );
	}
	else
	{
		struct timeval timeout;
		timeout.tv_sec = timeout_msecs / 1000L;
		timeout.tv_usec = ( timeout_msecs % 1000L ) * 1000L;
		nready = select( max_fd +1, &working_rfdset, &working_wfdset, (fd_set*)0, &timeout );
	}
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



