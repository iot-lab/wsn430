/*
 * Copyright (c) 2001, Adam Dunkels.
 * Copyright (c) 2009, Joakim Eriksson, Niclas Finne.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * $Id: sentapslip6.c,v 1.3 2009/11/03 14:00:28 nvt-se Exp $
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <err.h>

#include <netdb.h>

in_addr_t giaddr;
in_addr_t netaddr;
in_addr_t circuit_addr;

int ssystem(const char *fmt, ...)
     __attribute__((__format__ (__printf__, 1, 2)));
void write_to_serial(int outfd, void *inbuf, int len);

//#define PROGRESS(s) fprintf(stderr, s)
#define PROGRESS(s) do { } while (0)

#define USAGE_STRING "usage: sentapslip6 [-n node] [-t tundev]"

char tundev[32] = { "tap0" };

int
ssystem(const char *fmt, ...) __attribute__((__format__ (__printf__, 1, 2)));

int
ssystem(const char *fmt, ...)
{
  char cmd[128];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(cmd, sizeof(cmd), fmt, ap);
  va_end(ap);
  printf("%s\n", cmd);
  fflush(stdout);
  return system(cmd);
}

#define SLIP_END     0300
#define SLIP_ESC     0333
#define SLIP_ESC_END 0334
#define SLIP_ESC_ESC 0335

static void
print_packet(u_int8_t *p, int len) {
  int i;
  for(i = 0; i < len; i++) {
    printf("%02x", p[i]);
    if ((i & 3) == 3)
      printf(" ");
    if ((i & 15) == 15)
      printf("\n");
  }
  printf("\n");
}

int
is_sensible_string(const unsigned char *s, int len)
{
  int i;
  for(i = 1; i < len; i++) {
    if(s[i] == 0 || s[i] == '\r' || s[i] == '\n' || s[i] == '\t') {
      continue;
    } else if(s[i] < ' ' || '~' < s[i]) {
      return 0;
    }
  }
  return 1;
}


/*
 * Read from serial, when we have a packet write it to tun. No output
 * buffering, input buffered by stdio.
 */
void
serial_to_tun(int slipfd, int outfd)
{
  static union {
    unsigned char inbuf[2000];
  } uip;
  static int inbufptr = 0;

  int ret;
  unsigned char c, old_c = 0;

#ifdef linux
  if (old_c) {
	  ret = 1;
	  c = old_c;
	  old_c = 0;
  } else {
	  ret = read(slipfd, &c, 1);
  }
  if(ret == -1 || ret == 0) err(1, "serial_to_tun: read 1");
  goto after_fread;
#endif

 read_more:
  if(inbufptr >= sizeof(uip.inbuf)) {
     inbufptr = 0;
  }
  //ret = fread(&c, 1, 1, slipfd);
  if (old_c) {
    ret = 1;
    c = old_c;
    old_c = 0;
} else {
    ret = read(slipfd, &c, 1);
}
#ifdef linux
 after_fread:
#endif
  if(ret == -1) {
    return;
    err(1, "serial_to_tun: read 2");
  }
  if(ret == 0) {
    //clearerr(slipfd);
    return;
    fprintf(stderr, "serial_to_tun: EOF\n");
    exit(1);
  }
  /*  fprintf(stderr, ".");*/
  switch(c) {
  case SLIP_END:
    if(inbufptr > 0) {
      if(uip.inbuf[0] == '!') {
	if (uip.inbuf[1] == 'M') {
	  /* Read gateway MAC address and autoconfigure tap0 interface */
	  char macs[24];
	  int i, pos;
	  for(i = 0, pos = 0; i < 16; i++) {
	    macs[pos++] = uip.inbuf[2 + i];
	    if ((i & 1) == 1 && i < 14) {
	      macs[pos++] = ':';
	    }
	  }
	  macs[pos] = '\0';
	  printf("*** Gateway's MAC address: %s\n", macs);

	  ssystem("ifconfig %s down", tundev);
	  ssystem("ifconfig %s hw ether %s", tundev, &macs[6]);
	  ssystem("ifconfig %s up", tundev);
	}
#define DEBUG_LINE_MARKER '\r'
      } else if(uip.inbuf[0] == DEBUG_LINE_MARKER) {
	fwrite(uip.inbuf + 1, inbufptr - 1, 1, stdout);
      } else if(is_sensible_string(uip.inbuf, inbufptr)) {
	fwrite(uip.inbuf, inbufptr, 1, stdout);
      } else {
	printf("Writing to tun  len: %d\n", inbufptr);
	/*	print_packet(uip.inbuf, inbufptr);*/
	if(write(outfd, uip.inbuf, inbufptr) != inbufptr) {
	  err(1, "serial_to_tun: write");
	}
      }
      inbufptr = 0;
    }
    break;

  case SLIP_ESC:
    if(read(slipfd, &c, 1) != 1) {
      //clearerr(slipfd);
      /* Put ESC back and give up! */
      old_c = SLIP_ESC;
      return;
    }

    switch(c) {
    case SLIP_ESC_END:
      c = SLIP_END;
      break;
    case SLIP_ESC_ESC:
      c = SLIP_ESC;
      break;
    }
    /* FALLTHROUGH */
  default:
    uip.inbuf[inbufptr++] = c;
    break;
  }

  goto read_more;
}

unsigned char slip_buf[2000];
int slip_end, slip_begin;


void
slip_send(int fd, unsigned char c)
{
  if (slip_end >= sizeof(slip_buf))
    err(1, "slip_send overflow");
  slip_buf[slip_end] = c;
  slip_end++;
}

int
slip_empty()
{
  return slip_end == 0;
}

void
slip_flushbuf(int fd)
{
  int n;

  if (slip_empty())
    return;

  //n = write(fd, slip_buf + slip_begin, (slip_end - slip_begin));
  n = send(fd, slip_buf + slip_begin, (slip_end - slip_begin), 0);

  if(n == -1 && errno != EAGAIN) {
    err(1, "slip_flushbuf write failed");
  } else if(n == -1) {
    PROGRESS("Q");		/* Outqueueis full! */
  } else {
    slip_begin += n;
    if(slip_begin == slip_end) {
      slip_begin = slip_end = 0;
    }
  }
}

void
write_to_serial(int outfd, void *inbuf, int len)
{
  u_int8_t *p = inbuf;
  int i, ecode;

  /*  printf("Got packet of length %d - write SLIP\n", len);*/
  /*  print_packet(p, len);*/

  /* It would be ``nice'' to send a SLIP_END here but it's not
   * really necessary.
   */
  /* slip_send(outfd, SLIP_END); */
   /* printf("writing packet to serial!!! %d\n", len); */
  for(i = 0; i < len; i++) {
    switch(p[i]) {
    case SLIP_END:
      slip_send(outfd, SLIP_ESC);
      slip_send(outfd, SLIP_ESC_END);
      break;
    case SLIP_ESC:
      slip_send(outfd, SLIP_ESC);
      slip_send(outfd, SLIP_ESC_ESC);
      break;
    default:
      slip_send(outfd, p[i]);
      break;
    }

  }
  slip_send(outfd, SLIP_END);
  PROGRESS("t");
}


/*
 * Read from tun, write to slip.
 */
void
tun_to_serial(int infd, int outfd)
{
  struct {
    unsigned char inbuf[2000];
  } uip;
  int size;

  if((size = read(infd, uip.inbuf, 2000)) == -1) err(1, "tun_to_serial: read");

  write_to_serial(outfd, uip.inbuf, size);
}

int
devopen(const char *dev, int flags)
{
  char t[32];
  strcpy(t, "/dev/");
  strcat(t, dev);
  return open(t, flags);
}

unsigned long name_resolve(char *host_name)
{
  struct in_addr addr;
  struct hostent *host_ent;

  if((addr.s_addr=inet_addr(host_name))==(unsigned)-1) {
    host_ent=gethostbyname(host_name);
    if(host_ent==NULL) return(-1);
    memcpy(host_ent->h_addr, (char *)&addr.s_addr, host_ent->h_length);
    }
  return (addr.s_addr);
}

int
tcpopen(char* server, int port)
{
  int fd;
  int flags;
  struct sockaddr_in sin;

  // Create the socket
  fd = socket(AF_INET, SOCK_STREAM, 0);

  // Set it non blocking
  flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);

  // prepare the remote address
  memset(&sin, 0, sizeof(struct sockaddr_in));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);

  struct hostent *he;
  he = gethostbyname(server);
  if (he == NULL) {
    herror("tcpopen, error resolving hostname");
    exit(1);
  }

  memcpy(&sin.sin_addr, he->h_addr_list[0], he->h_length);

  // Connect to the server
  connect(fd, (struct sockaddr *)&sin, sizeof(sin));

  return fd;
}

#ifdef linux
#include <linux/if.h>
#include <linux/if_tun.h>

int
tun_alloc(char *dev)
{
  struct ifreq ifr;
  int fd, err;

  if( (fd = open("/dev/net/tun", O_RDWR)) < 0 )
    return -1;

  memset(&ifr, 0, sizeof(ifr));

  /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
   *        IFF_TAP   - TAP device
   *
   *        IFF_NO_PI - Do not provide packet information
   */
  ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
  if(*dev != 0)
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

  if((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ){
    close(fd);
    return err;
  }
  strcpy(dev, ifr.ifr_name);
  return fd;
}
#else
int
tun_alloc(char *dev)
{
  return devopen(dev, O_RDWR);
}
#endif

const char *ipaddr = "127.0.0.1";
const char *netmask = "255.0.0.0";

void
cleanup(void)
{
  ssystem("ifconfig %s down", tundev);
#ifndef linux
  ssystem("sysctl -w net.ipv6.conf.all.forwarding=1");
#endif
  /* ssystem("arp -d %s", ipaddr); */
  ssystem("netstat -nr"
	  " | awk '{ if ($2 == \"%s\") print \"route delete -net \"$1; }'"
	  " | sh",
	  tundev);
}

void
sigcleanup(int signo)
{
  fprintf(stderr, "signal %d\n", signo);
  exit(0);			/* exit(0) will call cleanup() */
}

static int got_sigalarm;
static int request_mac;

void
sigalarm(int signo)
{
  got_sigalarm = 1;
  return;
}

void
sigalarm_reset()
{
#ifdef linux
#define TIMEOUT (997*1000)
#else
#define TIMEOUT (2451*1000)
#endif
  ualarm(TIMEOUT, TIMEOUT);
  got_sigalarm = 0;
}

void
ifconf(const char *tundev, const char *ipaddr, const char *netmask)
{
  struct in_addr netname;
  netname.s_addr = inet_addr(ipaddr) & inet_addr(netmask);

#ifdef linux
  ssystem("ifconfig %s inet `hostname` up", tundev);
  if(strcmp(ipaddr, "0.0.0.0") != 0) {
    ssystem("route add -net %s netmask %s dev %s",
	    inet_ntoa(netname), netmask, tundev);
  }
#else
  ssystem("ifconfig %s inet `hostname` %s up", tundev, ipaddr);
  if(strcmp(ipaddr, "0.0.0.0") != 0) {
    ssystem("route add -net %s -netmask %s -interface %s",
	    inet_ntoa(netname), netmask, tundev);
  }
  ssystem("sysctl -w net.inet.ip.forwarding=1");
#endif /* !linux */

  ssystem("ifconfig %s\n", tundev);
}

int
main(int argc, char **argv)
{
  int c;
  int tunfd, slipfd, maxfd;
  int ret;
  fd_set rset, wset;

  int node = 1;
  request_mac = 1;
  setvbuf(stdout, NULL, _IOLBF, 0); /* Line buffered output. */

  while((c = getopt(argc, argv, "n:h:t:")) != -1) {
    switch (c) {
    case 't':
      if(strncmp("/dev/", optarg, 5) == 0) {
	strcpy(tundev, optarg + 5);
      } else {
	strcpy(tundev, optarg);
      }
      break;
    case 'n':
	node = atoi(optarg);
	fprintf(stderr, "Using node : %i\n", node);
	break;
    case '?':
    case 'h':
    default:
      errx(1, USAGE_STRING);
      break;
    }
  }
  argc -= (optind - 1);
  argv += (optind - 1);

  if(argc != 1) {
    errx(1, USAGE_STRING);
  }

  circuit_addr = inet_addr(ipaddr);
  netaddr = inet_addr(ipaddr) & inet_addr(netmask);

  if(node != 0) {
      slipfd = tcpopen("experiment", 30000 + node);
      if(slipfd == -1) {
	err(1, "can't open TCP to node %i", node);
      }
  }

  fprintf(stderr, "slip started on node %i\n", node);
  slip_send(slipfd, SLIP_END);

  tunfd = tun_alloc(tundev);
  printf("opening: %s", tundev);
  if(tunfd == -1) err(1, "main: open");
  fprintf(stderr, "opened device ``/dev/%s''\n", tundev);

  atexit(cleanup);
  signal(SIGHUP, sigcleanup);
  signal(SIGTERM, sigcleanup);
  signal(SIGINT, sigcleanup);
  signal(SIGALRM, sigalarm);
  ifconf(tundev, ipaddr, netmask);

  while(1) {
    maxfd = 0;
    FD_ZERO(&rset);
    FD_ZERO(&wset);

    /* request mac address from gateway node for autoconfiguration of
       ethernet interface tap0 */
    if (request_mac) {
      slip_send(slipfd, '?');
      slip_send(slipfd, 'M');
      slip_send(slipfd, SLIP_END);
      request_mac = 0;
    }

    if(got_sigalarm) {
      /* Send "?IPA". */
      slip_send(slipfd, '?');
      slip_send(slipfd, 'I');
      slip_send(slipfd, 'P');
      slip_send(slipfd, 'A');
      slip_send(slipfd, SLIP_END);
      got_sigalarm = 0;
    }

    if(!slip_empty()) {		/* Anything to flush? */
      FD_SET(slipfd, &wset);
    }

    FD_SET(slipfd, &rset);	/* Read from slip ASAP! */
    if(slipfd > maxfd) maxfd = slipfd;

    /* We only have one packet at a time queued for slip output. */
    if(slip_empty()) {
      FD_SET(tunfd, &rset);
      if(tunfd > maxfd) maxfd = tunfd;
    }

    ret = select(maxfd + 1, &rset, &wset, NULL, NULL);
    if(ret == -1 && errno != EINTR) {
      err(1, "select");
    } else if(ret > 0) {
      if(FD_ISSET(slipfd, &rset)) {
        serial_to_tun(slipfd, tunfd);
      }

      if(FD_ISSET(slipfd, &wset)) {
	slip_flushbuf(slipfd);
	sigalarm_reset();
      }

      if(slip_empty() && FD_ISSET(tunfd, &rset)) {
        tun_to_serial(tunfd, slipfd);
	slip_flushbuf(slipfd);
	sigalarm_reset();
      }
    }
  }
}
