#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/err.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

//�����豸������Ϊ pwm
#define DEVICE_NAME				"pwm"

#define PWM_IOCTL_SET_FREQ		1
#define PWM_IOCTL_STOP			0

#define NS_IN_1HZ				(1000000000UL)

//������PWM_ID  0 
#define BUZZER_PWM_ID			0
//������GPIO����
#define BUZZER_PMW_GPIO			EXYNOS4_GPD0(0)

//����һ���ṹ��ָ��
static struct pwm_device *pwm4buzzer;
//����һ���ṹ���ź���ָ��,��Ϊ�ź��������Ļ��Ʋ��
//Mutex��һ��Կ�ף�һ�������˾Ϳɽ���һ�����䣬������ʱ���Կ�׽������еĵ�һ����һ����÷������ڴ��л���critical section����ķ��ʣ���֤��δ��벻�ᱻ���е����С�
//Semaphore��һ����������N�˵ķ��䣬����˲����Ϳ��Խ�ȥ����������ˣ���Ҫ�ȴ����˳���������N=1���������Ϊbinary semaphore��һ����÷��ǣ�������//�ƶ���ĳһ��Դ��ͬʱ���ʡ�
static struct semaphore lock;

//����Ƶ��
static void pwm_set_freq(unsigned long freq) {
//PWM��ռ�ձȵ�����
	//printk("pwm_set_freq start");
	int period_ns = NS_IN_1HZ / freq;

	pwm_config(pwm4buzzer, period_ns / 2, period_ns); 
	pwm_enable(pwm4buzzer);
	//������Ӧ��GPIO,��������IO���ó�PWM���ģʽ
	s3c_gpio_cfgpin(BUZZER_PMW_GPIO, S3C_GPIO_SFN(2));
}
//stop������������Դ��operations�ṹ��
static  void pwm_stop(void) {
	//printk("pwm_stop start");
	s3c_gpio_cfgpin(BUZZER_PMW_GPIO, S3C_GPIO_OUTPUT);

	pwm_config(pwm4buzzer, 0, NS_IN_1HZ / 100);
	pwm_disable(pwm4buzzer);
}

//open������������Դ��operations�ṹ�壬��Ҫ��pwm�Ĳ���
static int iTop4412_pwm_open(struct inode *inode, struct file *file) {
	//printk("iTop4412_pwm_open start");
	if (!down_trylock(&lock)) //���Լ��������ʧ�ܷ���0
		return 0;
	else
		return -EBUSY;
}

//close������������Դ��operations�ṹ�壬��Ҫ�ǹر�pwm����
static int iTop4412_pwm_close(struct inode *inode, struct file *file) {
	//printk("iTop4412_pwm_close start");
	up(&lock);
	return 0;
}
//����io�ڷ�����������Դ��operations�ṹ��,��ʵ�����ϲ�ϵͳ���ô���һ�����//����ʶ�����Ȼ��ִ����Ӧ���̡�
static long iTop4412_pwm_ioctl(struct file *filep, unsigned int cmd,
		unsigned long arg)
{
	//printk("iTop4412_pwm_ioctl start");
	switch (cmd) {
		case PWM_IOCTL_SET_FREQ:
			if (arg == 0)
				return -EINVAL;
			pwm_set_freq(arg);
			break;

		case PWM_IOCTL_STOP:
		default:
			pwm_stop();
			break;
	}

	return 0;
}

//���������Ҫ���Ľṹ���ˣ���ʵ����ṹ��Ķ�������һ��.h���У��������ĳ�ʼ//����ʽ�������������Ǹ�����ķ�����������һ���ġ���Ӧ�ĺ�������Ҳ���Ǻ�����//�׵�ַ����ֵ����Ӧ�Ľṹ���Ա��ʵ���������ṹ��ĳ�ʼ���������ķ���������//C++��JAVA�ȸ߼����ԵĲ�����
static  struct file_operations iTop4412_pwm_ops = {
	.owner			= THIS_MODULE,  			//��ʾ��ģ��ӵ��
	.open			= iTop4412_pwm_open,		//��ʾ����open����
	.release		= iTop4412_pwm_close,         //��
	.unlocked_ioctl	= iTop4412_pwm_ioctl,
};

//�����豸��ע��
static struct miscdevice iTop4412_misc_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &iTop4412_pwm_ops,
};
//pwm�豸��ʼ��,�豸�ڱ�insmod����ģ�鵽�ں˵Ĺ����л�����������
static int __init iTop4412_pwm_dev_init(void) {
	//printk("iTop4412_pwm_dev_init start");
	int ret;
	gpio_free(BUZZER_PMW_GPIO);
	
	ret = gpio_request(BUZZER_PMW_GPIO, DEVICE_NAME);
	if (ret) {
		printk("request GPIO %d for pwm failed\n", BUZZER_PMW_GPIO);
		return ret;
	}

	gpio_set_value(BUZZER_PMW_GPIO, 0);
	s3c_gpio_cfgpin(BUZZER_PMW_GPIO, S3C_GPIO_OUTPUT);

	pwm4buzzer = pwm_request(BUZZER_PWM_ID, DEVICE_NAME);
	if (IS_ERR(pwm4buzzer)) {
		printk("request pwm %d for %s failed\n", BUZZER_PWM_ID, DEVICE_NAME);
		return -ENODEV;
	}

	pwm_stop();

	sema_init(&lock, 1);
	ret = misc_register(&iTop4412_misc_dev);

	printk(DEVICE_NAME "\tinitialized\n");

	return ret;
}
//�豸�ڱ�ж��rmmod�Ĺ����л�����������
static void __exit iTop4412_pwm_dev_exit(void) {
	//printk("iTop4412_pwm_exit start");
	pwm_stop();

	misc_deregister(&iTop4412_misc_dev);
	gpio_free(BUZZER_PMW_GPIO);
}

//ģ���ʼ��
module_init(iTop4412_pwm_dev_init);
//����ģ��
module_exit(iTop4412_pwm_dev_exit);
//����GPLЭ��
MODULE_LICENSE("GPL");
//���ߣ�yangyuanxin
MODULE_AUTHOR("Yangyuanxin");
//����:����PWM�豸
MODULE_DESCRIPTION("Exynos4 PWM Driver");