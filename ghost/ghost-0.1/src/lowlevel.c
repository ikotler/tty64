/*
 * lowlevel.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <fcntl.h>
#include <errno.h>

#include "log.h"

/*
 * async_duplex_bridge, duplex bridge between 2 fd's
 * * cfd, client fd
 * * sfd, proxy fd
 * * to_client, callback when data is ready to be sent to client
 * * to_proxy, callback when data is ready to be sent to proxy
 * * prebuf, previous client buffered chunk
 * * preblen, previous client bufferd chunk length
 */

int async_duplex_bridge(int cfd, int sfd, void *(*to_client)(char **, int), void *(*to_proxy)(char **, int), char *prebuf, int preblen) {
	char sbuf[2048], cbuf[2048];
    	char *chead, *ctail, *shead, *stail, *s_mod_hdr, *c_mod_hdr;
	int num, nfd, spos, cpos;
	fd_set rd, wr;

	chead = ctail = cbuf;
	cpos = spos = 0;
	shead = stail = sbuf;

	if (prebuf) {
		memcpy(chead, prebuf, preblen);
		ctail += preblen;
		cpos += preblen;
	}

	while (1) {

        	FD_ZERO(&rd);
        	FD_ZERO(&wr);

        	if (spos < sizeof(sbuf)-1) { 
			FD_SET(sfd, &rd);
		}

        	if (ctail > chead) { 
			FD_SET(sfd, &wr);
		}

        	if (cpos < sizeof(cbuf)-1) { 
			FD_SET(cfd, &rd);
		}

        	if (stail > shead) { 
			FD_SET(cfd, &wr);
		}

        	nfd = select(256, &rd, &wr, 0, NULL);

        	if (nfd <= 0) {
			return nfd;
		}

	        if (FD_ISSET(sfd, &rd)) {
		        num = read(sfd,stail,sizeof(sbuf)-spos);
	        	if ((num == -1) && (errno != EWOULDBLOCK)) { break; }
		        if (num == 0) break;
	        	if (num > 0) {
	                	spos += num;
		                stail += num;
		                if (!--nfd) continue;
		         }
	        }

	        if (FD_ISSET(cfd, &rd)) {
		        num = read(cfd,ctail,sizeof(cbuf)-cpos);
		        if ((num == -1) && (errno != EWOULDBLOCK)) { break; }
		        if (num == 0) break;
		        if (num > 0) {
	                	cpos += num;
 		                ctail += num;
			       	if (!--nfd) continue;
            		}
        	}

	        if (FD_ISSET(sfd, &wr)) {

			if (to_proxy) {
				c_mod_hdr = to_proxy(&chead, ctail-chead);
				if (c_mod_hdr) {
					num = write(sfd, c_mod_hdr, strlen(c_mod_hdr));
				}
			}

			num = write(sfd,chead,ctail-chead);
		        if ((num == -1) && (errno != EWOULDBLOCK)) { break; }
		        if (num > 0) {
			        chead += num;
		                if (chead == ctail) {
		     	        	chead = ctail = cbuf;
			                cpos = 0;
 		                }
                		if (!--nfd) continue;
            		}
		}

	        if (FD_ISSET(cfd, &wr)) {

			if (to_client) {
				s_mod_hdr = to_client(&shead, stail-shead);
				if (s_mod_hdr) {
					num = write(cfd, s_mod_hdr, strlen(s_mod_hdr));
				}
			}

			num = write(cfd,shead,stail-shead);
		        if ((num == -1) && (errno != EWOULDBLOCK)) { break; }
		        if (num > 0) {
		                shead += num;
		                if (shead == stail) {
			                shead = stail = sbuf;
			                spos = 0;
		                }
		                if (!--nfd) continue;
		         }
	        }
    }

    return num;
}

/*
 * net_setononblock, set O_NONBLOCK on given fd
 * * fd, given fd
 */

int net_setononblock(int fd) {
	int fl;

        fl = fcntl(fd, F_GETFL, 0);

        if (fcntl(fd, F_SETFL, fl | O_NONBLOCK) < 0) {
		return -1;
	}

	return 1;
}

/*
 * net_acceptsocket, accept new connection from ``master socket``
 * * srv_fd, given master socket
 */

int net_acceptsocket(int srv_fd) {
	int addrlen, new_fd;
	struct sockaddr_in client_addr;

	addrlen = sizeof(client_addr);

    	new_fd = accept(srv_fd, (struct sockaddr *)&client_addr, &addrlen);

	return new_fd;
}

/*
 * net_mastersocket, create server socket
 * * host, given host to bind socket
 * * port, given port to bind socket
 */

int net_mastersocket(char *host, char *port) {
	int srv_fd, yes = 1;
	struct sockaddr_in srv_addr;	

	srv_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (srv_fd < 0) {
		lerror("socket");
		return -1;
	}

	memset((void*)&srv_addr, 0x0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(atoi(port));
	srv_addr.sin_addr.s_addr = (host) ? inet_addr(host) : INADDR_ANY;
	
        if (setsockopt(srv_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
		lerror("setsockopt");
		close(srv_fd);
                return -1;
        }

        if (bind(srv_fd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) == -1) {
                lerror(host);
		close(srv_fd);
                return -1;
        }

        if (listen(srv_fd, 5) < 0) {
                lerror(host);
		close(srv_fd);
                return -1;
        }

	return srv_fd;
}

/*
 * net_getpeeraddr, return address of given peer
 * * peer, given peer
 */

char *net_getpeeraddr(int peer) {
        struct sockaddr_in peer_addr;
        int addrlen, retval;

        addrlen = sizeof(struct sockaddr_in);

        retval = getpeername (peer, (struct sockaddr *)&peer_addr, &addrlen);

        if (retval < 0) {
                return NULL;
        }

        return inet_ntoa(peer_addr.sin_addr);
}

/*
 * net_opensocket, open connection to given destnation / port
 * * addr, given address
 * * port, given port
 * * tout, given timeout in secs
 */

int net_opensocket(char *addr, int port, int tout) {
        int fd, retval;
        struct sockaddr_in sin;
        struct hostent *he;

        fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	he = gethostbyname(addr);

        if (fd < 0 || !he) {
             return -1;
        }

	memset((void*)&sin, 0x0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_port = htons(port);
        bcopy(he->h_addr, (char *)&sin.sin_addr, he->h_length);

	if (net_setononblock(fd) < 0) {
		close(fd);
		return -1;
	}

        retval = connect(fd, (struct sockaddr*)&sin, sizeof(sin));

	sleep(tout);

        retval = connect(fd, (struct sockaddr*)&sin, sizeof(sin));

        if (retval == 0) {
	        return fd;
        }

        close(fd);
        return -1;
}

/*
 * async_read, async read from fd
 * * fd, given fd
 * * buf, given buffer
 * * len, given buffer length
 * * tout, timeout in secs
 */

int async_read(int fd, char *buf, int len, int tout) {
	int rval, eta = 0;

        do {
		rval = read(fd, buf, len);
		sleep(1);
		eta++;

        } while ( (rval < 0 && errno == EWOULDBLOCK) && (eta < tout) );

	return rval;
}

/*
 * async_write, async write to fd
 * * fd, given fd
 * * buf, given buffer
 * * len, given buffer length
 * * tout, timeout in secs
 */

int async_write(int fd, char *buf, int len, int tout) {
	int rval, eta = 0;

        do {
		rval = write(fd, buf, len);
		sleep(1);
		eta++;

        } while ( (rval < 0 && errno == EWOULDBLOCK) && (eta < tout) );

	return rval;
}
