#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//����������򿪵Ĳ���
#define PWM_OPEN		1
//����������رյĲ���
#define PWM_STOP			0
static void close_bell(void);
static void open_bell(void);
static void set_bell_freq(int freq);
static void stop_bell(void);
int main(int argc, char **argv)
{
	//���÷�������Ƶ�ʵĳ�ʼֵΪ1000
	int freq = 1000 ;
	//�򿪷�����
	open_bell();
	stop_bell();
	while( 1 )
	{
		while(1){
		//���÷�����
		set_bell_freq(freq);
		//���Ƶ��С��20000
		if(freq < 20000){
			//�Լ�
			freq+=100 ;
			printf( "\tFreq = %d\n", freq );
			//����������һ��ѭ��
			if(freq == 20000){
				break ;
			}
		}
		}
		
		while(1){
			//���÷�����	
			set_bell_freq(freq);
			//���Ƶ�ʴ���1000
			if(freq > 1000)
				//�Լ�
				freq-=100 ;
			printf( "\tFreq = %d\n", freq );
			if(freq == 1000){
				break ;
			}			
		}
		//�ܶ���ʼ��ִ�У����Ƿ������ͻ��񳪸�һ��
	}
}

static int fd = -1;
//�򿪷�����
static void open_bell(void)
{
	//���豸
	fd = open("/dev/pwm", 0);
	//�����С��0��ʾʧ��
	if (fd < 0) {
		perror("open pwm_buzzer device");
		exit(1);
	}

	//��ʼ����������ʱ���ȹرգ���������
	atexit(close_bell);
}

//�رշ�����
static void close_bell(void)
{
	if (fd >= 0) {
		//�رշ�����
		ioctl(fd, PWM_STOP);
		if (ioctl(fd, 2) < 0) {
			perror("ioctl 2:");
		}
		close(fd);
		fd = -1;
	}
}
//���÷�������Ƶ��
static void set_bell_freq(int freq)
{
	//����Ƶ��
	int ret = ioctl(fd, PWM_OPEN, freq);
	if(ret < 0) {
		perror("set the frequency of the buzzer");
		exit(1);
	}
}
//ֹͣ������
static void stop_bell(void)
{
	//�÷�����ֹͣ��
	int ret = ioctl(fd, PWM_STOP);
	if(ret < 0) {
		perror("stop the buzzer");
		exit(1);
	}
	if (ioctl(fd, 2) < 0) {
		perror("ioctl 2:");
	}
}