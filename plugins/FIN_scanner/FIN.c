#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "scanner.h"
#include "packet.h"
#include "register.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <stdbool.h>
#include <netdb.h> 
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include "util.h"

#define  SRC_PORT   9000



static unsigned int _open_port_num = 0;
static unsigned int _close_port_num = 0;

/*
*   FIN扫描是建立在TCP基础上的，
*   其思想是关闭的端口会用适当的RST来回复FIN数据包
*   但是打开的端口会忽略对FIN数据包的回复（Unix系统）
*   NT无法扫描，可区分操作系统类别
*   port 为端口号
*/
static int FIN_scanner(int port)
{
    //scanner_log("%s\n", "I am fin");
    int  skfd;
    struct sockaddr_in target;//目标地址的相关信息
    struct sockaddr_in *_target;//处理多线程

    struct tcphdr *tcp;

    u_int32_t dst_ip;
    u_int32_t src_ip;

    int fin_ret;
    int sock_len ,i,j;
    char buffer[8192];

    if((skfd=socket(AF_INET,SOCK_RAW,IPPROTO_TCP)) < 0){
        perror("Create Error");
        return -errno;
    }
    src_ip = get_local_ip(skfd);
    //获取目标的地址和端口号
    _target = get_scan_addr();
    memcpy(&target,_target,sizeof(*_target));
    target.sin_port = htons(port);
    memcpy(&dst_ip,&target.sin_addr,4);
    //scanner_log("target ip %s\n", inet_ntoa(target.sin_addr));
    //scanner_log("target port %d\n", port);
    //scanner_log("src port%d\n",SRC_PORT);
    //scanner_log("src ip %u\n", src_ip);
    //
    fin_ret = tcp_fin_pack(buffer,8192,src_ip,dst_ip,SRC_PORT,port);
    if(fin_ret < 0){
        perror("tcp_fin_pack");
        return fin_ret;
    }
    fin_ret = sendto(skfd,buffer,fin_ret,0,(struct sockaddr*)&target,sizeof(target));
    
    if(fin_ret < 0){
        perror("sendto");
        return -errno;
    }

    sock_len = sizeof(target);
    for (i = 0; i < 20; i++)
    {
        fin_ret = recvfrom(skfd, buffer,8192, MSG_DONTWAIT , (struct sockaddr*)&target,&sock_len);
        if(fin_ret < 0 && (errno != EAGAIN) && (errno != EWOULDBLOCK)){
            perror("recvfrom");
            close(skfd);
        }else if(fin_ret >= sizeof(struct ip) + sizeof(struct tcphdr)){
            //检测Tcp报文的RST字段，为1，则表明该端口关闭，否则可能打开，或者为NT系统
            tcp = (struct tcphdr*)(buffer + ((struct ip*)buffer)->ip_hl * 4);
            if(tcp->rst){
                _close_port_num ++;
                close(skfd);
                return 1;
            }else{
                _open_port_num++;
                close(skfd);
                return 1;
            }
        }
        usleep( 100 * 1000);
    }

    _open_port_num++;
    close(skfd);
    return 0;
}

/*
    所有的端口扫描完成之后执行此程序
    输出端口打开、关闭的统计情况
    对于Windows操作系统扫描得出的结果应该是all-closed
    但是忽略其它因素，当关闭端口所占比例大于90%，则初步认定为windows系统
    否则为Unix操作系统
*/

static void over()
{
    float total = (float)( _open_port_num + _close_port_num);
    float open_port_rate = _open_port_num / total;
    float close_port_rate = 1 - open_port_rate;
    scanner_log("The number of open-port is %d, ",_open_port_num);
    scanner_log("account for a rate of %.2f\n", open_port_rate);
    scanner_log("The number of closed-port is %d, ",_close_port_num);
    scanner_log("account for a rate of %.2f\n", close_port_rate);

    //if(close_port_rate >= 0.8){
        //scanner_log("%s\n", "The server's operating system is likely to be Windows");
   // }else{
        //scanner_log("%s\n", "The server's operating system is likely to be Unix/Linux");
   // }

}


/*
    向scanner系统框架中注册 FIN扫描函数
    DEFAULT_SCANNER 为默认扫描方式，
    FIN_scanner为注册的函数名，
    SCAN_FIN为扫描方式参数，为FIN扫描
*/
void init_FIN()
{
    register_scanner(DEFAULT_SCANNER, FIN_scanner, SCAN_FIN);
    register_over(over, SCAN_FIN);
    //scanner_log("init\n");
}




int main(int argc, char **argv)
{
    init_FIN();
}