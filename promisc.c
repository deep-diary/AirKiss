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
//包含AirKiss头文件
#include "airkiss.h"
#include <signal.h>
#include <time.h>
#include <sys/time.h>

static void airkiss_finish(void);
static void wifi_promiscuous_rx(char *buf, unsigned short len);
void start_airkiss(void);
void airkissInitSigaction(void);
void airkissTimeCallback(int sig);
void airkissInitTime(long ms);
void airkissTimerRun(void);
void wifi_set_channel(int cur_channel);

//当前监听的无线信道
int cur_channel = 0;
//用于切换信道的定时器，平台相关
//AirKiss过程中需要的RAM资源，完成AirKiss后可以供其他代码使用
airkiss_context_t akcontex;
//定义AirKiss库需要用到的一些标准函数，由对应的硬件平台提供，前三个为必要函数
const airkiss_config_t akconf =
{
    (airkiss_memset_fn)&memset,
    (airkiss_memcpy_fn)&memcpy,
    (airkiss_memcmp_fn)&memcmp,
    0
};



/*
 * airkiss成功后读取配置信息，平台无关，修改打印函数即可
 */
static void airkiss_finish(void)
{
    char err;
    // unsigned char buffer[256];
    airkiss_result_t result;
    err = airkiss_get_result(&akcontex, &result);
    if (err == 0)
    {
        printf("airkiss_get_result() ok!");
        printf("ssid = \"%s\", pwd = \"%s\", ssid_length = %d, pwd_length = %d, random = 0x%02x\r\n",result.ssid, result.pwd, result.ssid_length, result.pwd_length, result.random);
    }
    else
    {
        printf("airkiss_get_result() failed !\r\n");
    }
}

/*
 * 混杂模式下抓到的802.11网络帧及长度，平台相关
 */
static void wifi_promiscuous_rx(char *buf, unsigned short len)
{
    char ret;
    //将网络帧传入airkiss库进行处理
    ret = airkiss_recv(&akcontex, buf, len);
    printf(">>>>>>>>>>>>>>>>>>%d\n", ret);
    //判断返回值，确定是否锁定信道或者读取结果
    if ( ret == AIRKISS_STATUS_CHANNEL_LOCKED)
    {
        printf("the channel have locked\n\n\n\n\n\n");
        airkissInitTime(0);
    }
    else if ( ret == AIRKISS_STATUS_COMPLETE )
    {
        printf("airkiss Finished \n\n\n\n\n\n");
        airkiss_finish();
        // wifi_promiscuous_enable(0);//关闭混杂模式，平台相关
    }
}

/*
 * 初始化并开始进入AirKiss流程，平台相关
 */
void start_airkiss(void)
{
    char ret;
    //如果有开启AES功能，定义AES密码，注意与手机端的密码一致
    // const char* key = "Wechatiothardwav";

    printf("Start airkiss!\r\n");
    //调用接口初始化AirKiss流程，每次调用该接口，流程重新开始，akconf需要预先设置好参数
    ret = airkiss_init(&akcontex, &akconf);
    //判断返回值是否正确
    if (ret < 0)
    {
        printf("Airkiss init failed!\r\n");
        return;
    }
    

#if AIRKISS_ENABLE_CRYPT
//如果使用AES加密功能需要设置好AES密钥，注意包含正确的库文件，头文件中的宏要打开
    // airkiss_set_key(&akcontex, key, strlen(key));
#endif
    printf("Finish init airkiss!\r\n");
    airkissTimerRun();
    //以下与硬件平台相关，设置模块为STATION模式并开启混杂模式，启动定时器用于定时切换信道
    // wifi_station_disconnect();
    // wifi_set_opmode(STATION_MODE);
    // cur_channel = 1;
    // wifi_set_channel(cur_channel);
    // os_timer_setfn(&time_serv, (os_timer_func_t *)time_callback, NULL);
    // os_timer_arm(&time_serv, 100, 1);
    // wifi_set_promiscuous_rx_cb(wifi_promiscuous_rx);
    // wifi_promiscuous_enable(1);
}


//------------------------------------------------
void airkissInitSigaction(void)
{
    struct sigaction tact;
    tact.sa_handler = airkissTimeCallback;
    tact.sa_flags = 0;
    sigemptyset(&tact.sa_mask);
    sigaction(SIGALRM, &tact, NULL);
}

//------------------------------------------------
void airkissTimeCallback(int sig)
{
    printf("It's the time to change channel .......\n");
        //切换信道
    if (cur_channel >= 11)
        cur_channel = 1;
    else
        cur_channel++;
    printf("the current channel is %d\n", cur_channel);
    wifi_set_channel(3);
    printf("-------------can you reach here\n");
    airkiss_change_channel(&akcontex);//清缓存
}

//------------------------------------------------
void airkissInitTime(long ms)
{
    struct itimerval value;
    value.it_value.tv_sec = ms / 1000;
    value.it_value.tv_usec = ms % 1000 * 1000;
    value.it_interval = value.it_value;
    setitimer(ITIMER_REAL, &value, NULL);
}
//------------------------------------------------
void airkissTimerRun(void)
{
    airkissInitSigaction();
    airkissInitTime(100);
}

void wifi_set_channel(int cur_channel)
{
    char cmd_buff[40]={0};
    sprintf(cmd_buff,"iw dev mon0 set channel %d",cur_channel);
    system(cmd_buff);
    // system("iw dev mon0 info");

}

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
    // int i;
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

    // start_airkiss();
    while (1) {
     n = recvfrom(sock,buffer,2048,0,NULL,NULL);  
     if (n<24)
     {
         printf("---------------------error-----------\n");
         continue;
     }
     printf("=====================================\n");
     //注意：在这之前我没有调用bind函数，原因是什么呢？
     
     printf("%d bytes read\n",n);
     wifi_promiscuous_rx(buffer, (unsigned short)n);

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
