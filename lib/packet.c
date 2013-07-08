#include "packet.h"
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
/**
 *
 **/
static u_int16_t _net_cksum(unsigned char *data, int len)
{
	int sum = 0;
	int odd = len & 0x01;

	while(len & 0xfffe)
	{
		sum += *(unsigned short*)data;
		data += 2;
		len -= 2;
	}
	if(odd)
	{
		unsigned short tmp = ((*data));
		sum += tmp;
	}
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);

	return ~sum; 
}

/*计算ip段的校验和*/
u_int16_t ip_chksum(struct ip *ip)
{
	//return _net_cksum((void*)ip, sizeof(struct ip));
	long sum = 0;  
	int len = sizeof(struct ip);

   	while ( len >1 ) {
        sum += *((unsigned short *) ip);
        if (sum & 0x80000000)       
            sum = (sum & 0xFFFF) + (sum >> 16) ;
        len -= 2;
        ip = (struct ip*)((unsigned long)ip + 2);
    }

    if ( len )                                      

        sum += ( unsigned  short  ) * (unsigned char *) ip;

    while ( sum >> 16)

         sum =(sum & 0xFFFF) + (sum>> 16);

   return ~sum;
}

int ip_pack(struct ip *iph, int proto, int ttl, u_int32_t src_ip, u_int32_t dest_ip, int data_len)
{
	/*参数检查*/
	if(!iph)
	{
		errno = EINVAL;
		return -errno;
	}
	/*参数修正*/
	ttl = (ttl <= 0) ? 1 : ttl;
	ttl =  (ttl > 255) ? 255 : ttl;
	/*开始构造*/
	/*使用ipv4*/
	iph->ip_v = 4;
	/*ip头部不带选项*/
	iph->ip_hl = 5;
	iph->ip_tos = 0;
	iph->ip_len = htons(20 + data_len);
	/*设置标志,为进程号*/
	iph->ip_id = getpid();
	/*偏移为0*/
	iph->ip_off = 0;
	iph->ip_ttl = ttl;
	iph->ip_p = proto;
	/*待会计算校验和*/
	iph->ip_sum = 0;
	memcpy((void*)&iph->ip_src, (void*)&src_ip, sizeof(src_ip));
	memcpy((void*)&iph->ip_dst, (void*)&dest_ip, sizeof(dest_ip));
	/*正式开始计算校验和*/
	iph->ip_sum = ip_chksum(iph);
	return 0;
}

int tcp_pack(struct tcphdr *tcp, u_int32_t src_ip, u_int32_t dst_ip, int src_port, int dest_port, u_int32_t seq, u_int32_t ack_seq, int ack, int psh, int rst, int syn, int fin, int window, char *data, int data_len)
{
	char buffer[8192];
	unsigned short hdr_len = htons(sizeof(struct tcphdr));

	if(!tcp || !data || data_len < 0 || data_len > 8192 - sizeof(struct tcphdr) - 12)
	{
		errno = EINVAL;
		return -errno;
	}
	memset(tcp, 0, sizeof(*tcp));
	/*构造tcp头*/
	tcp->source = htons(src_port);
	tcp->dest = htons(dest_port);
	tcp->seq = htonl(seq);
	tcp->ack_seq = htonl(ack_seq);
	tcp->ack = ack ? 1 : 0;
	tcp->psh = psh ? 1 : 0;
	tcp->rst = rst ? 1 : 0;
	tcp->syn = syn ? 1 : 0;
	tcp->fin = fin ? 1 : 0;
	tcp->window = htons((unsigned short)window);
	tcp->doff = 5;
	/*开始计算校验和*/
	memcpy(buffer, &src_ip, 4);
	memcpy(buffer + 4, &dst_ip, 4);
	buffer[8] = 0;
	buffer[9] = IPPROTO_TCP;
	memcpy(buffer + 10, &hdr_len, 2);
	memcpy(buffer + 12, tcp, sizeof(struct tcphdr));
	if(data_len > 0)
		memcpy(buffer + 12 + sizeof(struct tcphdr), data, data_len); 
	tcp->check = _net_cksum(buffer, 12 + sizeof(struct tcphdr) + data_len);
	return 0;
}

int tcp_syn_pack(char *buffer, int len, u_int32_t src_ip, u_int32_t dst_ip, int src_port, int dest_port, u_int32_t seq)
{
	int ret;
	if(!buffer || len < sizeof(struct tcphdr))
	{
		errno = EINVAL;
		return -errno;
	}
	ret = tcp_pack((struct tcphdr*)buffer, src_ip, dst_ip, src_port, dest_port, seq, 0, 0, 0, 0, 1, 0, 4096, buffer + sizeof(struct tcphdr), 0);
	return sizeof(struct tcphdr);
}

int tcp_fin_pack(char *buffer,int len,u_int32_t src_ip,u_int32_t dest_ip,int src_port,int dest_port)
{
	int ret;
	if(!buffer || len < sizeof(struct tcphdr)){
		errno = EINVAL;
		return -errno;
	}
	ret = tcp_pack((struct tcphdr*)buffer,src_ip,dest_ip,src_port,dest_port,0x10000000,0,0,0,0,0,1,4096,buffer + sizeof (struct tcphdr),0);
	//ret = tcp_pack((struct tcphdr*)buffer,src_ip,dest_ip,src_port,dest_port,1000,0,0,0,0,0,0,4096,buffer + sizeof (struct tcphdr),0);
	return sizeof(struct tcphdr);
}

#define PACKET_DEBUG
#ifdef PACKET_DEBUG
int main(int argc, char **argv)
{
	struct ip ip;
	ip_pack(&ip, IPPROTO_TCP, 10, 0x1234, 0x2345, 0);
}
#endif