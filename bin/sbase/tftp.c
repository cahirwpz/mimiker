/* See LICENSE file for copyright and license details. */
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netdb.h>
#include <netinet/in.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

#define BLKSIZE 512
#define HDRSIZE 4
#define PKTSIZE (BLKSIZE + HDRSIZE)

#define TIMEOUT_SEC 5
/* transfer will time out after NRETRIES * TIMEOUT_SEC */
#define NRETRIES 5

#define RRQ  1
#define WWQ  2
#define DATA 3
#define ACK  4
#define ERR  5

static char *errtext[] = {
	"Undefined",
	"File not found",
	"Access violation",
	"Disk full or allocation exceeded",
	"Illegal TFTP operation",
	"Unknown transfer ID",
	"File already exists",
	"No such user"
};

static struct sockaddr_storage to;
static socklen_t tolen;
static int timeout;
static int state;
static int s;

static int
packreq(unsigned char *buf, int op, char *path, char *mode)
{
	unsigned char *p = buf;

	*p++ = op >> 8;
	*p++ = op & 0xff;
	if (strlen(path) + 1 > 256)
		eprintf("filename too long\n");
	memcpy(p, path, strlen(path) + 1);
	p += strlen(path) + 1;
	memcpy(p, mode, strlen(mode) + 1);
	p += strlen(mode) + 1;
	return p - buf;
}

static int
packack(unsigned char *buf, int blkno)
{
	buf[0] = ACK >> 8;
	buf[1] = ACK & 0xff;
	buf[2] = blkno >> 8;
	buf[3] = blkno & 0xff;
	return 4;
}

static int
packdata(unsigned char *buf, int blkno)
{
	buf[0] = DATA >> 8;
	buf[1] = DATA & 0xff;
	buf[2] = blkno >> 8;
	buf[3] = blkno & 0xff;
	return 4;
}

static int
unpackop(unsigned char *buf)
{
	return (buf[0] << 8) | (buf[1] & 0xff);
}

static int
unpackblkno(unsigned char *buf)
{
	return (buf[2] << 8) | (buf[3] & 0xff);
}

static int
unpackerrc(unsigned char *buf)
{
	int errc;

	errc = (buf[2] << 8) | (buf[3] & 0xff);
	if (errc < 0 || errc >= LEN(errtext))
		eprintf("bad error code: %d\n", errc);
	return errc;
}

static int
writepkt(unsigned char *buf, int len)
{
	int n;

	n = sendto(s, buf, len, 0, (struct sockaddr *)&to,
	           tolen);
	if (n < 0)
		if (errno != EINTR)
			eprintf("sendto:");
	return n;
}

static int
readpkt(unsigned char *buf, int len)
{
	int n;

	n = recvfrom(s, buf, len, 0, (struct sockaddr *)&to,
	             &tolen);
	if (n < 0) {
		if (errno != EINTR && errno != EWOULDBLOCK)
			eprintf("recvfrom:");
		timeout++;
		if (timeout == NRETRIES)
			eprintf("transfer timed out\n");
	} else {
		timeout = 0;
	}
	return n;
}

static void
getfile(char *file)
{
	unsigned char buf[PKTSIZE];
	int n, op, blkno, nextblkno = 1, done = 0;

	state = RRQ;
	for (;;) {
		switch (state) {
		case RRQ:
			n = packreq(buf, RRQ, file, "octet");
			writepkt(buf, n);
			n = readpkt(buf, sizeof(buf));
			if (n > 0) {
				op = unpackop(buf);
				if (op != DATA && op != ERR)
					eprintf("bad opcode: %d\n", op);
				state = op;
			}
			break;
		case DATA:
			n -= HDRSIZE;
			if (n < 0)
				eprintf("truncated packet\n");
			blkno = unpackblkno(buf);
			if (blkno == nextblkno) {
				nextblkno++;
				write(1, &buf[HDRSIZE], n);
			}
			if (n < BLKSIZE)
				done = 1;
			state = ACK;
			break;
		case ACK:
			n = packack(buf, blkno);
			writepkt(buf, n);
			if (done)
				return;
			n = readpkt(buf, sizeof(buf));
			if (n > 0) {
				op = unpackop(buf);
				if (op != DATA && op != ERR)
					eprintf("bad opcode: %d\n", op);
				state = op;
			}
			break;
		case ERR:
			eprintf("error: %s\n", errtext[unpackerrc(buf)]);
		}
	}
}

static void
putfile(char *file)
{
	unsigned char inbuf[PKTSIZE], outbuf[PKTSIZE];
	int inb, outb, op, blkno, nextblkno = 0, done = 0;

	state = WWQ;
	for (;;) {
		switch (state) {
		case WWQ:
			outb = packreq(outbuf, WWQ, file, "octet");
			writepkt(outbuf, outb);
			inb = readpkt(inbuf, sizeof(inbuf));
			if (inb > 0) {
				op = unpackop(inbuf);
				if (op != ACK && op != ERR)
					eprintf("bad opcode: %d\n", op);
				state = op;
			}
			break;
		case DATA:
			if (blkno == nextblkno) {
				nextblkno++;
				packdata(outbuf, nextblkno);
				outb = read(0, &outbuf[HDRSIZE], BLKSIZE);
				if (outb < BLKSIZE)
					done = 1;
			}
			writepkt(outbuf, outb + HDRSIZE);
			inb = readpkt(inbuf, sizeof(inbuf));
			if (inb > 0) {
				op = unpackop(inbuf);
				if (op != ACK && op != ERR)
					eprintf("bad opcode: %d\n", op);
				state = op;
			}
			break;
		case ACK:
			if (inb < HDRSIZE)
				eprintf("truncated packet\n");
			blkno = unpackblkno(inbuf);
			if (blkno == nextblkno)
				if (done)
					return;
			state = DATA;
			break;
		case ERR:
			eprintf("error: %s\n", errtext[unpackerrc(inbuf)]);
		}
	}
}

static void
usage(void)
{
	eprintf("usage: %s -h host [-p port] [-x | -c] file\n", argv0);
}

int
main(int argc, char *argv[])
{
	struct addrinfo hints, *res, *r;
	struct timeval tv;
	char *host = NULL, *port = "tftp";
	void (*fn)(char *) = getfile;
	int ret;

	ARGBEGIN {
	case 'h':
		host = EARGF(usage());
		break;
	case 'p':
		port = EARGF(usage());
		break;
	case 'x':
		fn = getfile;
		break;
	case 'c':
		fn = putfile;
		break;
	default:
		usage();
	} ARGEND

	if (!host || !argc)
		usage();

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	ret = getaddrinfo(host, port, &hints, &res);
	if (ret)
		eprintf("getaddrinfo: %s\n", gai_strerror(ret));

	for (r = res; r; r = r->ai_next) {
		if (r->ai_family != AF_INET &&
		    r->ai_family != AF_INET6)
			continue;
		s = socket(r->ai_family, r->ai_socktype,
		           r->ai_protocol);
		if (s < 0)
			continue;
		break;
	}
	if (!r)
		eprintf("cannot create socket\n");
	memcpy(&to, r->ai_addr, r->ai_addrlen);
	tolen = r->ai_addrlen;
	freeaddrinfo(res);

	tv.tv_sec = TIMEOUT_SEC;
	tv.tv_usec = 0;
	if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
		eprintf("setsockopt:");

	fn(argv[0]);
	return 0;
}
