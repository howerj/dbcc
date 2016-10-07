/* SocketCAN read */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h> 
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

static int open_can_device(const char *port)
{
	struct ifreq ifr;
	struct sockaddr_can addr;
	int r;
	int fd;
	if((fd = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
		return -1;

	addr.can_family = AF_CAN;
	strcpy(ifr.ifr_name, port);

	if((r = ioctl(fd, SIOCGIFINDEX, &ifr)) < 0)
		return r;

	addr.can_ifindex = ifr.ifr_ifindex;

	if((r = fcntl(fd, F_SETFL, O_NONBLOCK)) < 0)
		return r;
	if((r = bind(fd, (struct sockaddr *)&addr, sizeof(addr))) < 0)
		return r;
	return fd;
}

static int read_can_loop(int port)
{
	struct can_frame frame_rd;
	int recvbytes = 0;

	while (true) {
		struct timeval timeout = { 1, 0 };
		fd_set read_set;
		FD_ZERO(&read_set);
		FD_SET(port, &read_set);

		if (select((port + 1), &read_set, NULL, NULL, &timeout) >= 0) {
			if (FD_ISSET(port, &read_set)) {
				errno = 0;
				recvbytes = read(port, &frame_rd, sizeof(struct can_frame));
				if(recvbytes < 0)
					switch(errno) {
					/*case EWOULDBLOCK:*/
					case EAGAIN: usleep(1);
					case EINTR:
						continue;
					default:
						return -errno;
					}
				if (recvbytes) {
					printf("id 0x%03x, dlc = %d\n\t", frame_rd.can_id, frame_rd.can_dlc);
					for (unsigned i = 0; i < frame_rd.can_dlc; i++)
						printf("%02x ", frame_rd.data[i]);
					printf("\n");
				}
			}
		}
	}
	return 0;
}

int send_can_msg(struct can_frame *frame, int port)
{
	int r;
	errno = 0;
again:
	r = write(port, frame, sizeof(struct can_frame));
	if(r <= 0 || r != sizeof(struct can_frame))
		switch(errno)  {
			case EAGAIN: usleep(1);
			case EINTR: goto again;
			default:
				return -errno;
		}
	return r;
}

static void usage(const char *arg0)
{
	fprintf(stderr, "%s: [-] [-h] [-d device]\n", arg0);
}

static void help(void)
{
	static const char *msg = "";
	fputs(msg, stderr);
}


int main(int argc, char **argv)
{
	char *port = "can0";
	int i;
	int fd = 0;
	for(i = 1; i < argc && argv[i][0] == '-'; i++)
		switch(argv[i][1]) {
		case '\0': /* stop argument processing */
			goto done; 
		case 'd':
			if(i >= argc - 1)
				goto fail;
			port = argv[++i];
			break;
		case 'h':
			usage(argv[0]);
			help();
			break;
		default:
		fail:
			usage(argv[0]);
			fprintf(stderr, "unknown/invalid command line option '%c'", argv[i][1]);
			return EXIT_FAILURE;
			break;
		}
done:
	fd = open_can_device(port);
	if(fd < 0) {
		perror(port);
		return EXIT_FAILURE;
	}
	errno = 0;
	if(read_can_loop(fd) < 0) {
		perror(port);
		return EXIT_FAILURE;
	}
	close(fd);
	return EXIT_SUCCESS;
}
