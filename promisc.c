#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <string.h>
#include <sys/ioctl.h>

 #define PLOG(fmt, args...) printf(fmt, ## args)

char dev[20]={0};
char s_ip[20]={0};


//For command options
static const char *optstring = "d:r:p:h";
static const char *usage = "\
Usage: gpio_ir_app [option] [option parameter]\n\
-h          display help information\n\
-d  device name\n\
-s  the source ip\n\
-p  the source port\n\
";
/* -------------------------------------------------------------------
 * Parse settings from app's command line.
 * ------------------------------------------------------------------- */
int parse_opt(const char opt, const char *optarg)
{
    int i;
    switch(opt)
    {
    case 'h':
        PLOG("%s\n",usage);
        exit(0);
        break;
    case 'd':
        PLOG("option:d\n");
        strcpy(dev, optarg);
        break;
    case 's':
        PLOG("Spot Inspection Version 1.0, 20160815\n");
        strcpy(s_ip, optarg);
        break;
    
    default:
        PLOG("###Unknown option:%c###\n",opt);
        PLOG("%s\n",usage);
        exit(0);
        break;
    }
    return 0;
}

int main(int argc, char **argv) {
   int sock, n;
   char opt;
   char buffer[2048];
   struct ethhdr *eth;
   struct iphdr *iph;
	struct ifreq ethreq;
    //parse
    while((opt = getopt(argc, argv, optstring)) != -1)
    {

        parse_opt(opt, optarg);
    }

   if (0>(sock=socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP)))) {
     perror("socket");
     exit(1);
   }
	strncpy(ethreq.ifr_name,dev,IFNAMSIZ);
	if(-1 == ioctl(sock,SIOCGIFFLAGS,&ethreq)){
     perror("ioctl");
     close(sock);
     exit(1);
	}
	ethreq.ifr_flags |=IFF_PROMISC;
	if(-1 == ioctl(sock,SIOCGIFFLAGS,&ethreq)){
     perror("ioctl");
     close(sock);
     exit(1);
	}

   while (1) {
     printf("=====================================\n");
     //注意：在这之前我没有调用bind函数，原因是什么呢？
     n = recvfrom(sock,buffer,2048,0,NULL,NULL);
     printf("%d bytes read\n",n);

     //接收到的数据帧头6字节是目的MAC地址，紧接着6字节是源MAC地址。
     eth=(struct ethhdr*)buffer;
     printf("Dest MAC addr:%02x:%02x:%02x:%02x:%02x:%02x\n",eth->h_dest[0],eth->h_dest[1],eth->h_dest[2],eth->h_dest[3],eth->h_dest[4],eth->h_dest[5]);
     printf("Source MAC addr:%02x:%02x:%02x:%02x:%02x:%02x\n",eth->h_source[0],eth->h_source[1],eth->h_source[2],eth->h_source[3],eth->h_source[4],eth->h_source[5]);

     iph=(struct iphdr*)(buffer+sizeof(struct ethhdr));
     //我们只对IPV4且没有选项字段的IPv4报文感兴趣
     if(iph->version ==4 && iph->ihl == 5){
             printf("Source host:%s\n",(char *)inet_ntoa(*(struct in_addr *)&iph->saddr));
             printf("Dest host:%s\n",(char *)inet_ntoa(*(struct in_addr *)&iph->daddr));
     }
   }
}
