#include	"../util.h"

#include	<netinet/in.h>
#include	<sys/types.h>

void conmunicate(int fd) {
	char	read_buff[BUFF_SIZE];
	int		n_write;
	int 	n_read;

	while ( fgets(read_buff, BUFF_SIZE, stdin) != NULL) {
		n_write = write(fd, read_buff, strlen(read_buff)+1);
		if ( (n_read = read(fd, read_buff, strlen(read_buff)+1)) == 0) {
			printf("server process broken the connection already!\n");
			break;
		}
		fputs(read_buff, stdout);
	}
}

int main(int argc, char** argv) {
	
	int 				conn_fd;
	struct sockaddr_in 	serv_addr;

	if (argc < 2)
		err_quit("require two parameters for main()!");

	if ( (conn_fd = create_socket(argv[1], PORT_NO)) < 0)
		err_quit("create connection socket fail.");

	/* conmunicate with server */
	conmunicate(conn_fd);

	if ( close(conn_fd) == -1)
		err_quit("call close failed!");

	return 0;
}
