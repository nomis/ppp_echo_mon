/*
 * Copyright ©2010  Simon Arlott
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (Version 2) as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#include <unistd.h>
#include <getopt.h>
#include "ppp_echo_mon.h"

static void help(FILE *out, const char *name) {
	fprintf(out, "Usage: %s [-n count] [-t timeout] <source interface> <source ip> <ppp device>\n", name);
}

int main(int argc, char *argv[]) {
	int ret, opt;
	char *source = NULL;
	char *ip = NULL;
	char *device = NULL;
	int count = 1;
	int timeout = 1;
	int s, len, ifidx, one = 1;
	struct sockaddr_in6 src;
	struct sockaddr_in6 dst;
	struct sockaddr_in6 rcv;
	socklen_t rcvlen = sizeof(rcv);
	struct group_source_req gsreq;
	u_int64_t c, hash = 0;
	char *tmp;
	char buf[1024];

	while ((opt = getopt(argc, argv, "hn:t:m:")) != -1) {
		switch (opt) {
		case 'n':
			count = atoi(optarg);
			break;

		case 't':
			timeout = atoi(optarg);
			break;

		case 'h':
			help(stdout, argv[0]);
			exit(EXIT_SUCCESS);

		case '?':
			break;
		}
	}

	if (optind < argc) {
		source = argv[optind++];
	} else {
		help(stderr, argv[0]);
		exit(EXIT_FAILURE);
	}

	if (optind < argc) {
		ip = argv[optind++];
	} else {
		help(stderr, argv[0]);
		exit(EXIT_FAILURE);
	}

	if (optind < argc) {
		device = argv[optind++];
	} else {
		help(stderr, argv[0]);
		exit(EXIT_FAILURE);
	}

	if (optind < argc) {
		help(stderr, argv[0]);
		exit(EXIT_FAILURE);
	}

	s = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	cerror("Socket error", !s);

	ifidx = if_nametoindex(source);
	cerror("Interface not found", !ifidx);

	src.sin6_family = AF_INET6;
	cerror("Invalid source", !inet_pton(AF_INET6, ip, &src.sin6_addr));
	src.sin6_port = htons(SPORT);
	src.sin6_scope_id = ifidx;

	dst.sin6_family = AF_INET6;
	cerror("Invalid group", !inet_pton(AF_INET6, GROUP, &dst.sin6_addr));
	dst.sin6_port = htons(DPORT);
	dst.sin6_scope_id = ifidx;
	tmp = device;
	while ((c = *tmp++))
		hash = c + (hash << 6) + (hash << 16) - hash;
	dst.sin6_addr.__in6_u.__u6_addr8[4] = (hash >> 56) & 0xFF;
	dst.sin6_addr.__in6_u.__u6_addr8[5] = (hash >> 48) & 0xFF;
	dst.sin6_addr.__in6_u.__u6_addr8[6] = (hash >> 40) & 0xFF;
	dst.sin6_addr.__in6_u.__u6_addr8[7] = (hash >> 32) & 0xFF;
	dst.sin6_addr.__in6_u.__u6_addr8[8] = (hash >> 24) & 0xFF;
	dst.sin6_addr.__in6_u.__u6_addr8[9] = (hash >> 16) & 0xFF;
	dst.sin6_addr.__in6_u.__u6_addr8[10] = (hash >> 8) & 0xFF;
	dst.sin6_addr.__in6_u.__u6_addr8[11] = hash & 0xFF;

	cerror("Failed to set SO_REUSEADDR", setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)));
	cerror("Failed to bind destination port", bind(s, (struct sockaddr*)&dst, sizeof(dst)));
	gsreq.gsr_interface = ifidx;
	memcpy(&gsreq.gsr_source, &src, sizeof(src));
	memcpy(&gsreq.gsr_group, &dst, sizeof(dst));
	cerror("Failed to join multicast group", setsockopt(s, IPPROTO_IPV6, MCAST_JOIN_SOURCE_GROUP, &gsreq, sizeof(gsreq)));

	alarm(count + timeout);
	while ((len = recvfrom(s, buf, 1023, MSG_NOSIGNAL, (struct sockaddr*)&rcv, &rcvlen)) >= 0) {
		char type[16];
		char name[32];
		unsigned long secs;
		unsigned int usecs;

		if (rcvlen != sizeof(src)
				|| rcv.sin6_family != src.sin6_family
				|| rcv.sin6_port != src.sin6_port
				|| rcv.sin6_scope_id != src.sin6_scope_id)
			continue;
		buf[len] = '\0';

		ret = sscanf(buf, "%15s %*u %31s %*d %*u.%*u %lu.%u", type, name, &secs, &usecs);
		if (ret == 4 && !strcmp(type, "EchoRep") && !strcmp(name, device)) {
			printf("Elapsed time: %lu.%06u seconds\n", secs, usecs);
			count--;
			if (count == 0)
				break;
		}
	}

	exit(EXIT_SUCCESS);
}
