/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef SLIPPI_ASIX_H
#define SLIPPI_ASIX_H
#include "global.h"
#include "net.h"
s32 ASIXInit(void);
/* 1 = selected, 0 = use native IOS, negative = selected driver failed. */
s32 ASIXTryInit(void);
/* Startup-only screen checkpoints; no filesystem I/O or runtime polling. */
#ifdef ASIX_HOST_TEST
static inline void ASIXBootProgress(u32 step) {(void)step;}
#else
void ASIXBootProgress(u32 step);
#endif
int ASIXIsActive(void);
const char *ASIXAddressMode(void);
s32 ASIXDhcpResult(void);
s32 ASIXSocket(u32 domain, u32 type, u32 protocol);
s32 ASIXClose(s32 sock);
s32 ASIXBind(s32 sock, const struct sockaddr *addr);
s32 ASIXListen(s32 sock, u32 backlog);
s32 ASIXAccept(s32 sock);
s32 ASIXSend(s32 sock, const void *data, s32 len);
s32 ASIXRecv(s32 sock, void *data, s32 len);
s32 ASIXConnect(s32 sock, const struct sockaddr *addr);
s32 ASIXSetSockOpt(s32 sock, u32 level, u32 option, const void *value, u32 len);
s32 ASIXPoll(struct pollsd *sds, s32 count, s32 timeout);
/* IPv4 addresses in network byte order; ports in host order. Receive returns
 * -6 when no datagram is queued. Sends/receives are nonblocking. */
s32 ASIXUdpOpen(u16 port);
void ASIXUdpClose(s32 fd);
s32 ASIXUdpSend(s32 fd,u32 addr,u16 port,const void *data,u32 length);
s32 ASIXUdpReceive(s32 fd,u32 *addr,u16 *port,void *data,u32 capacity);
int ASIXResolve(const char *name,u32 *addr);
/* Single outbound stream, separate from the mirror listener/client. Address is
 * network order; port is host order. Nonblocking: connect returns -26 while
 * pending, send/receive -6 while waiting, receive 0 at EOF, other errors <0.
 * Close must release the descriptor even after a reset/connection error. */
s32 ASIXTcpOpen(u32 address,u16 port);
s32 ASIXTcpConnect(s32 fd);
s32 ASIXTcpSend(s32 fd,const void *data,u32 length);
s32 ASIXTcpReceive(s32 fd,void *data,u32 capacity);
void ASIXTcpClose(s32 fd);
void ASIXUpdate(void);
void ASIXShutdown(void);
#endif
