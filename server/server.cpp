#include	"../util.h"

#include	<sys/epoll.h>

#include	<map>
using namespace std;

struct Sock_buff
{
	char buff[BUFF_SIZE];
	int  n_byte;
	Sock_buff() {
		n_byte = 0;
	}
};

/*
 *Accept a connection socket and add events to epoll
 *@return 
 	accepted socket id if success
	-1 if fail
 * */
int do_accept(int listen_fd, int epoll_fd)
{
	int conn_fd;

	/* accept a connection socket */
	if ( (conn_fd = accept(listen_fd, NULL, NULL)) < 0)
		return -1;

	if ( make_socket_unblocking(conn_fd) < 0) 
	{
		close(conn_fd);
		return -1;
	}

	/* add events to epoll */
	epoll_event ev;
	create_event(&ev, conn_fd, EPOLLIN);

	if ( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &ev) < 0)
	{
		close(conn_fd);
		return -1;
	}

	return conn_fd;
}

/*
 *@return
 	1 if read all bytes from socket
	0 if encounter EOF
	-1 if error occur
 *@assume pbuff->buff is infinate
 * */
int read_all(int sock_fd, Sock_buff* pbuff)
{
	int n_read;

	while (1)
	{
		if ( (n_read = read(sock_fd, pbuff->buff+pbuff->n_byte, BUFF_SIZE-pbuff->n_byte)) < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			if (errno == EINTR)
				continue;
			return -1;
		}
		else if (n_read == 0)
		{
			return 0;
		}
		else
		{
			pbuff->n_byte += n_read;
		}
	}

	return 1;
}

/*
 *@return
 	0 if succeed to write
	-1 if error occur
 * */
int write_all(int sock_fd, Sock_buff* pbuff)
{
	int n_write;

	while (pbuff->n_byte > 0)
	{
		if ( (n_write = write(sock_fd, pbuff->buff, pbuff->n_byte)) < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			if (errno == EINTR)
				continue;
			return -1;
		}
		pbuff->n_byte -= n_write;
	}

	return 0;
}

/*
 *@return 
 	0 if success
	-1 if fail
 * */
int update_event(int epoll_fd, int sock_fd, uint32_t events)
{
	epoll_event ev;
	create_event(&ev, sock_fd, events);

	if ( epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sock_fd, &ev) < 0)
		return -1;
	return 0;
}

void close_sock(int fd, map<int, Sock_buff*> & m)
{
	close(fd);
	delete m[fd];
	m.erase(fd);
}

void show(Sock_buff* p)
{
	int i;
	for (i = 0; i < p->n_byte; ++ i)
		printf("%c", p->buff[i]);
}

int main()
{
	int listen_fd;
	
	/* create listening socket */
	if ( (listen_fd = create_socket(NULL, PORT_NO)) < 0)
		err_quit("create listening socket fail.");

	if ( make_socket_unblocking(listen_fd) < 0)
		err_quit("make listening socket unblocking fail.");

	listen(listen_fd, 100);
	
	/* create epoll with lt */
	int epoll_fd;
	if ( (epoll_fd = epoll_create(1)) < 0)
		err_quit("create epoll fail.");

	/* add listening socket to epoll */
	epoll_event ev;
	create_event(&ev, listen_fd, EPOLLIN);

	if ( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) < 0)
		err_quit("add listening socket to epoll fail.");

	/* event polling */
	int n, i;
	epoll_event events[MAX_EVENTS];
	map<int, Sock_buff*> m_buff;

	while (1)
	{
		if ( (n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1)) < 0)
			err_quit("epoll_wait call fail.");

		for (i = 0; i < n; ++ i)
		{
			int sock_fd = events[i].data.fd;

			if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP)
			{ /* some error occur on sock_fd */
				close_sock(sock_fd, m_buff);
				continue;
			}

			if (sock_fd == listen_fd)
			{ /* listening socket is ready */
				int conn_fd;
				conn_fd = do_accept(listen_fd, epoll_fd);
				printf("new socket id = %d\n", conn_fd);

				Sock_buff* buff = new Sock_buff();
				m_buff[conn_fd] = buff;
			}
			else 
			{ /* connection socket is ready */

				if (events[i].events & EPOLLIN)
				{ /* connection socket is ready to read */
					int ret;
					if ( (ret = read_all(sock_fd, m_buff[sock_fd])) == 0)
					{
						printf("client shutdown!\n");
						close_sock(sock_fd, m_buff);
					}
					else if (ret < 0)
					{
						printf("read socket error, this socket will be closed!\n");
						close_sock(sock_fd, m_buff);
					}
					else
						update_event(epoll_fd, sock_fd, EPOLLOUT);
				}
				if (events[i].events & EPOLLOUT)
				{ /* connection socket is ready to write */
					if ( write_all(sock_fd, m_buff[sock_fd]) < 0)
					{
						printf("write socket error, this socket will be closed!\n");
						close_sock(sock_fd, m_buff);
					}
					else
						update_event(epoll_fd, sock_fd, EPOLLIN);
				}
			}
		} /* for */
	} /* while */

	return 0;
}
