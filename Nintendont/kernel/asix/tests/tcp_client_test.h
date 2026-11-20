/* SPDX-License-Identifier: GPL-2.0-or-later
 * Real so_tcp.c over the production raw TCP backend and simulated wire peer.
 */
static u16 outgoing_port;
static u32 outgoing_seq,outgoing_ack;
static int ignore_syn;
static volatile int *cancel_during_sleep;
static u32 cancel_at;
static void outgoing_segment(u8 flags,const void *payload,unsigned n) {
    u8 *ip=new_frame(54+n,0x800)+14,*tcp=ip+20;
    ip[0]=0x45;w16(ip+2,40+n);ip[8]=64;ip[9]=6;
    w32(ip+12,0xc0a88901);w32(ip+16,lwip_ntohl(current_ip_address));w16(ip+10,checksum(ip,20,0));
    w16(tcp,443);w16(tcp+2,outgoing_port);w32(tcp+4,outgoing_seq);w32(tcp+8,outgoing_ack);
    tcp[12]=0x50;tcp[13]=flags;w16(tcp+14,32768);
    if(n)memcpy(tcp+20,payload,n);
    w16(tcp+16,checksum(tcp,20+n,6+20+n+0xc0a8+0x8901+b16(ip+16)+b16(ip+18)));
    outgoing_seq+=n+!!(flags&2)+!!(flags&1);
}
static void outgoing_wire(const u8 *tcp,unsigned header,unsigned length) {
    if(tcp[13]&4)return;
    if(tcp[13]&2) {
        outgoing_port=b16(tcp);outgoing_ack=b32(tcp+4)+1;outgoing_seq=100000;
        if(!ignore_syn)outgoing_segment(0x12,NULL,0);
    } else if(length) {
        u32 seq=b32(tcp+4);
        if(seq==outgoing_ack) {
            outgoing_ack+=length;outgoing_segment(0x18,tcp+header,length);
        } else outgoing_segment(0x10,NULL,0);
    }
}
u64 so_time_us(void){return (u64)host_ms*1000;}
u32 so_time_ms(void){return host_ms;}
void so_sleep_us(u32 us){host_ms+=(us+999)/1000;if(cancel_during_sleep && host_ms>=cancel_at)*cancel_during_sleep=1;}
void *so_malloc(u32 n){return calloc(1,n);}
void so_log(const char *format,...){(void)format;}
s32 IOS_Ioctl(s32 fd,s32 req,void *in,s32 ilen,void *out,s32 olen) {
    (void)fd;(void)req;(void)in;(void)ilen;(void)out;(void)olen;
    assert(!"custom matchmaking must not call IOS networking");return -1;
}
s32 IOS_Ioctlv(s32 fd,s32 req,s32 in,s32 out,void *vec) {
    (void)fd;(void)req;(void)in;(void)out;(void)vec;
    assert(!"custom matchmaking must not call IOS networking");return -1;
}
#include "../../slippi_online/so_tcp.c"
static void tcp_client_tests(void) {
    test_diagnostic_log=0;assert(ASIXTryInit()==1);
    /* The outbound worker must coexist with an accepted mirroring stream. */
    assert(ASIXSocket(AF_INET,SOCK_STREAM,IPPROTO_IP)==SERVER_FD);
    struct sockaddr_in target={0};target.sin_family=AF_INET;target.sin_port=lwip_htons(51441);
    assert(ASIXBind(SERVER_FD,(struct sockaddr*)&target)==0 && ASIXListen(SERVER_FD,1)==0);
    segment(2,NULL,0);s32 mirror=ASIXAccept(SERVER_FD);assert(mirror>=3);
    volatile int cancel=1;
    assert(so_tcp_open(lwip_htonl(0xc0a88901),443,100,&cancel)==SO_TCP_CANCELLED && !outbound.fd);
    cancel=0;assert(so_tcp_open(lwip_htonl(0xc0a88901),443,100,&cancel)==0);
    s32 first=outbound.fd;assert(first>0 && outgoing_port!=51441);
    assert(ASIXTcpOpen(lwip_htonl(0xc0a88901),443)<0 && outbound.fd==first);
    u8 data[4096],copy[4096];for(unsigned i=0;i<sizeof(data);i++)data[i]=i*37+19;
    assert(so_tcp_send(data,sizeof(data),1000)==sizeof(data));
    unsigned got=0;while(got<sizeof(copy)) {
        int n=so_tcp_recv(copy+got,37,100);assert(n>0);got+=n;
    }
    assert(!memcmp(data,copy,sizeof(data)) && !outbound.received);
    /* The mirror and UDP/DNS still work while the worker owns its stream. */
    u8 prefix[4]={0,0,0,15};segment(0x18,prefix,4);
    assert(ASIXRecv(mirror,copy,4)==4 && !memcmp(copy,prefix,4));
    s32 udp=ASIXUdpOpen(45000);assert(ASIXUdpSend(udp,lwip_htonl(0xc0a88901),43113,prefix,4)==4);
    u32 from;u16 port;assert(ASIXUdpReceive(udp,&from,&port,copy,4)==4);ASIXUdpClose(udp);
    u32 address;assert(ASIXResolve("tls.test",&address)==0);
    assert(so_tcp_recv(copy,1,20)==0);
    outgoing_segment(0x18,"bye",3);outgoing_segment(0x11,NULL,0);
    assert(so_tcp_recv(copy,8,100)==3 && !memcmp(copy,"bye",3));
    assert(so_tcp_recv(copy,8,100)==SO_TCP_CLOSED);
    so_tcp_close();assert(!outbound.fd && !outbound.pcb && !outbound.received);
    assert(so_tcp_open(lwip_htonl(0xc0a88901),443,100,&cancel)==0 && outbound.fd!=first);
    ASIXTcpClose(first);assert(so_tcp_is_open() && ASIXTcpReceive(first,copy,1)<0);
    outgoing_segment(4,NULL,0);assert(so_tcp_recv(copy,1,100)<0);so_tcp_close();
    ignore_syn=1;u32 start=host_ms;
    assert(so_tcp_open(lwip_htonl(0xc0a88901),443,25,&cancel)==SO_TCP_TIMEOUT && host_ms-start<50 && !outbound.fd);
    cancel_during_sleep=&cancel;cancel_at=host_ms+20;
    assert(so_tcp_open(lwip_htonl(0xc0a88901),443,100,&cancel)==SO_TCP_CANCELLED && !outbound.fd);
    cancel_during_sleep=NULL;cancel=0;ignore_syn=0;
    assert(so_tcp_open(lwip_htonl(0xc0a88901),443,100,&cancel)==0);
    static u8 large[32768];block_output=1;unblock_at=host_ms+10000;start=host_ms;
    assert(so_tcp_send(large,sizeof(large),25)==SO_TCP_TIMEOUT && host_ms-start<50);
    block_output=0;so_tcp_close();assert(!outbound.received);
    assert(ASIXClose(mirror)==0 && ASIXClose(SERVER_FD)==0);
    puts("PASS matchmaking TCP: production so_tcp bridge, 4096-byte binary echo and partial reads, mirror/UDP/DNS coexistence, EOF/RST, stale descriptors, timeout/cancel/backpressure, no IOS network calls");
}
