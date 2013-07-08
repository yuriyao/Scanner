/**
 *  处理底层的包
 **/
#ifndef PACKET_H
#define PACKET_H

#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

/**
  *  构造ip头
  *  @iph  需要进行构造的ip头
  *  @proto  协议
  *  @ttl  
  *  @src_ip  转换到大端模式的源ip
  *  @des_ip  转换到大端模式的目的ip 
  *  @data_len ip协议的有效载荷,(包括tcp头,udp,icmp头)的长度
  *  构造成功,返回0,否则返回负的错误号,并置errno
  */
int ip_pack(struct ip *iph, int proto, int ttl, u_int32_t src_ip, u_int32_t dest_ip, int data_len);

/**
 *  构造tcp头
 *  参数都是可以自解释的,详见tcp头部的定义(所有参数不要转为大端模式)
 *  tcp头和data必须是连在一起的,并且data指向tcp的有效载荷数据
 **/
int tcp_pack(struct tcphdr *tcp, u_int32_t src_ip, u_int32_t dst_ip, int src_port, int dest_port, u_int32_t seq, u_int32_t ack_seq, int ack, int psh, int rst, int syn, int fin, int window, char *data, int data_len);

/**
 *  构造tcp的syn报文
 *  @buffer  报文存放的缓冲区
 *  @len 缓冲区的大小
 *  @src_port
 *  @dest_port
 *  @seq  包的序列号
 *  成功返回报文大小,否则返回负数
 **/
int tcp_syn_pack(char *buffer, int len, u_int32_t src_ip, u_int32_t dst_ip, int src_port, int dest_port, u_int32_t seq);


int tcp_fin_pack(char *buffer,int len,u_int32_t src_ip,u_int32_t dest_ip,int src_port,int dest_port);
#endif
