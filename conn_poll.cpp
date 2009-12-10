fdwatch_add_fd();
num_ready = fdwatch();
fdwatch_check_fd();

select_check_fd()
{
	isset();
}

select_maxfd()
{
-- get max fd for select .
}

select_watch()
{
	select();
}

-- get new active fd.
select_get_next_client_data()
{
}


int start()
{
	// main loop
	while ( (! _terminate) || (_conn_cnt > 0) )
	{
		nready = watch();
		if ( nready < 0 )
		{
			if (errno == EINTR || error == EAGAIN )
				continue;
			//log::error("pollConn failed!\n");
			break;
		}
		if ( nready = 0 )
		{
			continue;
		}
		while()
		{
			if ( ! check() )
			{
			}
			else
			{
				switch ()
				{
					case 1:
						break;
					case 2:
						break;
					case 3:
						break;
					default:
						break;
				}
			}
		}// end while;
		shut();
	}
}