#include "com.h"

// int fd;//文件描述符
// int epid; //epoll标识符
// struct epoll_event event;
// struct epoll_event events[6];//事件集合

int Serial::openSerial(char *cSerialName)
{
    int iFd;

    struct termios opt; 

    iFd = open(cSerialName, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if(iFd < 0) {
        perror(cSerialName);
        return -1;
    }

    tcgetattr(iFd, &opt);      

    cfsetispeed(&opt, B115200);
    cfsetospeed(&opt, B115200);

    
    /*
     * raw mode
     */
    opt.c_lflag   &=   ~(ECHO   |   ICANON   |   IEXTEN   |   ISIG);
    opt.c_iflag   &=   ~(BRKINT   |   ICRNL   |   INPCK   |   ISTRIP   |   IXON);
    opt.c_oflag   &=   ~(OPOST);
    opt.c_cflag   &=   ~(CSIZE   |   PARENB);
    opt.c_cflag   |=   CS8;

    /*
     * 'DATA_LEN' bytes can be read by serial
     */
    opt.c_cc[VMIN]   =   DATA_LEN;                                      
    opt.c_cc[VTIME]  =   150;

    if (tcsetattr(iFd,   TCSANOW,   &opt)<0) {
        return   -1;
    }


    return iFd;
}

int Serial::EpollInit(int *cfd)
{
	epid = epoll_create(6);//可处理的最大句柄数6个
	
	event.events = EPOLLET | EPOLLIN;
	event.data.fd = cfd[0];
	if (epoll_ctl(epid, EPOLL_CTL_ADD, cfd[0], &event) != 0) {//将串口1读事件添加到epoll的事件队列中
		printf("set uart1 epoll error!\n");
		return 0;
	}
	printf("set uart1 epoll ok!\n");

	event.events = EPOLLET | EPOLLIN;
	event.data.fd = cfd[1];
	if (epoll_ctl(epid, EPOLL_CTL_ADD, cfd[1], &event) != 0) {//将串口2读事件添加到epoll的事件队列中
		printf("set uart2 epoll error!\n");
		return 0;
	}
	printf("set uart2 epoll ok!\n");
	
	return 1;
}

int Serial::ComRead(char * ReadBuff,const int ReadLen)
{
	int len = 0;

	//下面开始epoll等待
	int i = 0,witeNum = 0;
	run = 1;

	while (run) 
	{
		witeNum = epoll_wait(epid, events, 2, 100);
		//printf("witeNum0 = %d\n   ", witeNum);
		switch(witeNum)
		{		
			case 0:					//超时
				//printf("timeout\n");  
			break;  

			case -1:                 //出错  
				perror("epoll_wait");  
			break;  

			default: //需要处理事件数目
				for (i = 0; i < witeNum; i++) 
				{
					int rfd = events[i].data.fd; //准确获取哪个事件的描述符
					if ((events[i].events & EPOLLERR)
							|| (events[i].events & EPOLLHUP)
							|| (!(events[i].events & EPOLLIN))) 
					{
						printf("uart no data!\n");
						break;
					} 
					else if (events[i].events & EPOLLIN) 
					{//有数据进入  接受数据
						len = read(events[i].data.fd, ReadBuff, ReadLen);
						if(rfd == fd[0])
						{
							tcdrain(fd[0]);//等待所有输出都被传输
							tcflush(fd[0],TCIOFLUSH);//刷清未决输入和/或输出
							ReadBuff[len] = 0;
							printf("uart1: %s\n",ReadBuff);
							if(len != ReadLen) //如何保证每次都读到这些字节又不阻塞！ 
							{
								bzero(ReadBuff,128);
							}
							
							if( len == ReadLen)
							return len;		
						}
						else if(rfd == fd[1])
						{
							tcdrain(fd[1]);//等待所有输出都被传输
							tcflush(fd[1],TCIOFLUSH);//刷清未决输入和/或输出
							ReadBuff[len] = 0;
							printf("uart2: %s\n",ReadBuff);

							write(pipe_fd, ReadBuff, len);//写入管道
							
							if(len != ReadLen) //如何保证每次都读到这些字节又不阻塞！ 
							{
								bzero(ReadBuff,128);
							}
							
							if( len == ReadLen)
							return len;		
						}			
					}
				}
			break;
		}
	}

	return len ;
}

