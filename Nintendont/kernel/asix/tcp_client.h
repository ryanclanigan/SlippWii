/* SPDX-License-Identifier: GPL-2.0-or-later
 * One outbound TCP stream for SlippWii's TLS/WebSocket worker. All callbacks,
 * polling and queue operations run under the backend's existing lwIP mutex.
 * Receive window credit is returned only when the application reads bytes.
 */
static struct {
    struct tcp_pcb *pcb;
    struct pbuf *received;
    s32 fd;
    int connected,eof,error;
} outbound;
static u32 outbound_serial=0x20000000;
static void outbound_error(void *arg,err_t error) {
    (void)arg;outbound.pcb=NULL;outbound.error=error;
}
static err_t outbound_connected(void *arg,struct tcp_pcb *pcb,err_t error) {
    (void)arg;(void)pcb;
    if(error!=ERR_OK)outbound.error=error;
    else outbound.connected=1;
    return ERR_OK;
}
static err_t outbound_received(void *arg,struct tcp_pcb *pcb,struct pbuf *p,err_t error) {
    (void)arg;(void)pcb;
    if(!p){outbound.eof=1;return ERR_OK;}
    if(error!=ERR_OK)return error;
    if(outbound.received && p->tot_len>TCP_WND-outbound.received->tot_len)return ERR_MEM;
    if(outbound.received)pbuf_cat(outbound.received,p);
    else outbound.received=p;
    return ERR_OK;
}
s32 ASIXTcpOpen(u32 address,u16 port) {
    if(!address || !port)return -1;
    lock();s32 result=-1;
    if(!outbound.fd && !asix_hw_failed()) {
        memset(&outbound,0,sizeof(outbound));outbound.pcb=tcp_new();
        if(outbound.pcb) {
            tcp_recv(outbound.pcb,outbound_received);tcp_err(outbound.pcb,outbound_error);
            tcp_nagle_disable(outbound.pcb);
            ip_addr_t to;ip_addr_set_ip4_u32(&to,address);
            if(tcp_connect(outbound.pcb,&to,port,outbound_connected)==ERR_OK) {
                if(outbound_serial==0x3fffffff)outbound_serial=0x20000000;
                outbound.fd=result=++outbound_serial;
            } else {
                tcp_err(outbound.pcb,NULL);tcp_abort(outbound.pcb);outbound.pcb=NULL;
            }
        }
    }
    pump();unlock();return result;
}
s32 ASIXTcpConnect(s32 fd) {
    lock();pump();s32 result=-1;
    if(fd==outbound.fd && outbound.pcb && !outbound.error && !asix_hw_failed())
        result=outbound.connected?0:-26;
    unlock();return result;
}
s32 ASIXTcpSend(s32 fd,const void *data,u32 length) {
    if(!data && length)return -1;
    lock();pump();s32 result=-1;
    if(fd==outbound.fd && outbound.pcb && outbound.connected && !outbound.error && !asix_hw_failed()) {
        u16_t n=tcp_sndbuf(outbound.pcb);if(n>length)n=length;
        if(!length)result=0;
        else if(!n)result=-6;
        else {
            err_t error=tcp_write(outbound.pcb,data,n,TCP_WRITE_FLAG_COPY);
            if(error==ERR_OK){tcp_output(outbound.pcb);result=n;}
            else if(error==ERR_MEM)result=-6;
        }
    }
    pump();unlock();return result;
}
s32 ASIXTcpReceive(s32 fd,void *data,u32 capacity) {
    if(!data && capacity)return -1;
    lock();pump();s32 result=-1;
    if(fd==outbound.fd && outbound.fd) {
        if(!capacity)result=0;
        else if(outbound.received) {
            u16_t n=outbound.received->tot_len;if(n>capacity)n=capacity;
            pbuf_copy_partial(outbound.received,data,n,0);
            outbound.received=pbuf_free_header(outbound.received,n);
            if(outbound.pcb)tcp_recved(outbound.pcb,n);
            result=n;
        } else if(!outbound.error && !asix_hw_failed())result=outbound.eof?0:-6;
    }
    unlock();return result;
}
void ASIXTcpClose(s32 fd) {
    lock();
    if(outbound.fd && fd==outbound.fd) {
        if(outbound.pcb) {
            tcp_recv(outbound.pcb,NULL);tcp_err(outbound.pcb,NULL);
            tcp_abort(outbound.pcb);
        }
        if(outbound.received)pbuf_free(outbound.received);
        memset(&outbound,0,sizeof(outbound));
    }
    pump();unlock();
}
