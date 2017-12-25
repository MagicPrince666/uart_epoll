#include <stdio.h>
#include "com.h"
#include <fcntl.h>      /*file control*/
#include <signal.h>
#include <pthread.h> 
#include <iostream>
#include <unistd.h>

#define FIFO_NAME "/tmp/my_fifo"
#define BUF_SIZE 1024

using namespace std;

Serial serial; 
int run = 1;

static void sigint_handler(int sig)
{
	run = 0;
	serial.run = 0;
    cout << "--- quit the loop! ---" << endl;
}

void * uart_send (void *arg) 
{
    while(run)
    {
        //write(serial.fd[0], "hello!,I'm uart1\n",17);	
        //write(serial.fd[1], "hello!,I'm uart2\n",17);	
        sleep(1);
    }
    pthread_exit(NULL);
}

void * uart_rev (void *arg) 
{
	char tmp[128] = {0};
	
    printf("PID %d open pipe O_WRONLY\n", getpid());
	serial.pipe_fd = open(FIFO_NAME, O_WRONLY | O_NONBLOCK);//在这里会阻塞
	if(serial.pipe_fd <= 0)
		printf("open pipe error\n");
	
	serial.EpollInit(serial.fd);

    while(run)
    {
		bzero(tmp,sizeof(tmp));		
		serial.ComRead(tmp,128);//读取8个字节放到缓存	
	}

	close(serial.epid);
	close(serial.fd[0]);
	close(serial.fd[1]);
	if(serial.pipe_fd > 0)//退出管道
		close(serial.pipe_fd);
    pthread_exit(NULL);
}

int main(int argc,char **argv)
{
	
	pthread_t pthread_id[2];//线程ID

	signal(SIGINT, sigint_handler);//信号处理

	serial.fd[0] = serial.openSerial((char *)"/dev/ttyS1");
	serial.fd[1] = serial.openSerial((char *)"/dev/ttyS2");
	
	if(serial.fd[0] < 0)
	{
		printf("open com1 fail!\n");
		return 0;
	}
	if(serial.fd[1] < 0)
	{
		printf("open com2 fail!\n");
		return 0;
	}
	
	tcflush(serial.fd[0],TCIOFLUSH);//清空串口输入输出缓存
	tcflush(serial.fd[1],TCIOFLUSH);//清空串口输入输出缓存

	if (pthread_create(&pthread_id[0], NULL, uart_send, NULL))
		cout << "Create uart_send error!" << endl;
	if (pthread_create(&pthread_id[1], NULL, uart_rev, NULL))
		cout << "Create uart_rev error!" << endl;    

	if(pthread_id[0] !=0) {                   
		pthread_join(pthread_id[0],NULL);
		cout << "uart_send "<< pthread_id[0]<< " exit!"  << endl;
	}
	if(pthread_id[1] !=0) {                   
		pthread_join(pthread_id[1],NULL);
		cout << "uart_rev " << pthread_id[1] << " exit!"  << endl;
	}
	
	return 0;
}