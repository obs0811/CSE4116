#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/version.h>

#include <linux/ioctl.h>
#include <linux/timer.h>

#include <linux/interrupt.h>
#include <asm/irq.h>
#include <mach/gpio.h>
#include <asm/gpio.h>
#include <linux/wait.h>
#include <linux/cdev.h>

/* macros */
#define DEV_MAJOR 243
#define DEV_NAME  "/dev/stopwatch"
#define LED_ADDRESS 0x08000016
#define TEXT_LCD_ADDRESS 0x08000090

// global variable
static unsigned char *fnd_addr;
static unsigned char *led_addr;
static int led_port_usage = 0;
static unsigned char *text_lcd_addr;
static int text_lcd_port_usage = 0;

// timer structure
struct timer_list timer;
// a variable to check timer termination condition
int timer_count = 0;

struct timer_list timer_end;
int end_pressed = 0;

wait_queue_head_t wq_write;
DECLARE_WAIT_QUEUE_HEAD(wq_write);
int interruptCount = 0;
int time_cnt = 0;
int stopwatch_on = 0;
int beat_rate;
unsigned short led_value;	

// function prototype
int device_open(struct inode *minode, struct file *mfile);
int device_release(struct inode *minode, struct file *mfile);
int device_write(struct file *inode, const char *gdata, int length, loff_t *off_what);
irqreturn_t inter_handler1(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler2(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler3(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_handler4(int irq, void* dev_id, struct pt_regs* reg);
void timer_handler(unsigned long arg);
void timer_end_handler(unsigned long arg);

// file_operations structure
struct file_operations dev_fops =
{
    .open           =	device_open,
    .release        =	device_release,
    .write          =	device_write,
};

// start
irqreturn_t inter_handler1(int irq, void* dev_id, struct pt_regs* reg) {
	//del_timer_sync(&timer);
	printk(KERN_ALERT "interrupt1!!! = %x\n", gpio_get_value(IMX_GPIO_NR(1, 11)));
	/*if(!stopwatch_on) {
		printk("start is pressed\n");
		stopwatch_on = 1;
		timer.expires = get_jiffies_64() + HZ;
		timer.data = 0;
		timer.function = timer_handler;
		add_timer(&timer);
	}*/
	return IRQ_HANDLED;
}

// pause
irqreturn_t inter_handler2(int irq, void* dev_id, struct pt_regs* reg) {
	printk(KERN_ALERT "interrupt2!!! = %x\n", gpio_get_value(IMX_GPIO_NR(1, 12)));
	/*
	if(stopwatch_on) {
		stopwatch_on = 0;
		del_timer(&timer);
	}*/
	beat_rate = 0;
	del_timer(&timer);
	__wake_up(&wq_write, 1, 1, NULL);
	return IRQ_HANDLED;
}

// reset
irqreturn_t inter_handler3(int irq, void* dev_id,struct pt_regs* reg) {
	printk(KERN_ALERT "interrupt3!!! = %x\n", gpio_get_value(IMX_GPIO_NR(2, 15)));
	/*if(stopwatch_on) {
		stopwatch_on = 0;
	}
	time_cnt = 0;
	outw(0, (unsigned int)fnd_addr);
	del_timer(&timer);
	*/
	beat_rate--;
	if(beat_rate <= 1)	beat_rate = 1;
	del_timer(&timer);
	__wake_up(&wq_write, 1, 1, NULL);
    return IRQ_HANDLED;
}

irqreturn_t inter_handler4(int irq, void* dev_id, struct pt_regs* reg) {
	printk(KERN_ALERT "interrupt4!!! = %x\n", gpio_get_value(IMX_GPIO_NR(5, 14)));
	/*if(gpio_get_value(IMX_GPIO_NR(5, 14)) == 0) {	// if trigger falling
		printk("added timer_end_handler\n");
		timer_end.expires = get_jiffies_64() + 3 * HZ;
		timer_end.data = 0;
		timer_end.function = timer_end_handler;
		add_timer(&timer_end);
	}
	else {	// if trigger rising
		del_timer(&timer_end);
	}*/
	/*
	outw(0, (unsigned int)fnd_addr);
	del_timer_sync(&timer);
	__wake_up(&wq_write, 1, 1, NULL);
	*/
	beat_rate++;
	if(beat_rate >= 10)	beat_rate = 10;
	del_timer(&timer);
	__wake_up(&wq_write, 1, 1, NULL);
    return IRQ_HANDLED;
}

// define functions
int device_open(struct inode *minode, struct file *mfile)
{
	int irq, ret;
    
	if(led_port_usage != 0)	return -EBUSY;
	led_port_usage = 1;

	if(text_lcd_port_usage != 0)	return -EBUSY;
	text_lcd_port_usage = 1;

	// int1
	gpio_direction_input(IMX_GPIO_NR(1,11));
	irq = gpio_to_irq(IMX_GPIO_NR(1,11));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler1, IRQF_TRIGGER_FALLING, "home", 0);

	// int2
	gpio_direction_input(IMX_GPIO_NR(1,12));
	irq = gpio_to_irq(IMX_GPIO_NR(1,12));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler2, IRQF_TRIGGER_FALLING, "back", 0);

	// int3
	gpio_direction_input(IMX_GPIO_NR(2,15));
	irq = gpio_to_irq(IMX_GPIO_NR(2,15));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler3, IRQF_TRIGGER_FALLING, "volup", 0);

	// int4
	gpio_direction_input(IMX_GPIO_NR(5,14));
	irq = gpio_to_irq(IMX_GPIO_NR(5,14));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_handler4, IRQF_TRIGGER_FALLING, "voldown", 0);
	
	// initialize timer
	init_timer(&timer);
	/*timer.expires = get_jiffies_64() + 2 * HZ;
	timer.data = 0;
	timer.function = timer_handler;
	add_timer(&timer);*/
    return 0;
}

int device_release(struct inode *minode, struct file *mfile)
{
	del_timer(&timer);
	//led_value = 0;
	//outw(led_value, (unsigned int)led_addr);

	led_port_usage = 0;
	text_lcd_port_usage = 0;
    // close fnd
    free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 12)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);
    return 0;
}

int device_write(struct file *inode, const char *gdata, int length, loff_t *off_what)
{
    char data[100];
	
	// stopwatch timer
	beat_rate = length;
	if(beat_rate <= 1) {
		beat_rate = 1;
	}
	if(beat_rate >= 10) {
		beat_rate = 10;
	}
	timer.expires = get_jiffies_64() + beat_rate * HZ;
	timer.data = 0;
	timer.function = timer_handler;
	add_timer(&timer);

	/*timer.expires = get_jiffies_64() + HZ;
	timer.data = 0;
	timer.function = timer_handler;
	*/
	// timer to end stopwatch
	//init_timer(&timer_end);
	/*timer_end.expires = get_jiffies_64() + 3 * HZ;
	timer_end.data = 0;
	timer_end.function = timer_end_handler;
	*/
	printk("added timer handler\n");
	printk("process sleeps\n");
	interruptible_sleep_on(&wq_write);
	printk("process wakes up\n");
	//led_value = 0;
	//outw(led_value, (unsigned int)led_addr);
	return beat_rate;
}

void timer_handler(unsigned long arg) {
	//unsigned long data = timer.data;
	char str[10] = "metronome";
	char text_lcd[32];
	int i;
	static int cur_pos = 0, end = 9;
	unsigned short int lcd_value;
	static int plus = 0;

	//led_value = 1;
	//outw(led_value, (unsigned int)led_addr);
	for(i = 0; i < 32; i++) {
		text_lcd[i] = 0;
	}
	end = cur_pos + 9;
	for(i = cur_pos; i < end; i++) {
		text_lcd[i] = str[i-cur_pos];
	}
	for(i = 0; i < 32; i++) {
		lcd_value = (text_lcd[i] & 0xFF) << 8 | (text_lcd[i+1] & 0xFF);
		outw(lcd_value, (unsigned int)text_lcd_addr+i);
		i++;
	}
	if(plus) {
		if(cur_pos < 23)
			cur_pos++;
		else {
			plus = 0;
			cur_pos--;
		}
	}
	else {
		if(cur_pos > 0)
			cur_pos--;
		else {
			plus = 1;
			cur_pos++;
		}
	}

	__wake_up(&wq_write, 1, 1, NULL);
	// periodically call device_write() to output to FND, LED, Dot matrix, and Text LCD
	// add timer structure so that after the current timer expires, it can be called again
	//if(stopwatch_on)	time_cnt++;
	//printk("**** A\n");
	//printk("**** B\n");
	//printk("**** C\n");

	/*mydata.timer.expires = get_jiffies_64() + HZ;
	mydata.timer.data = (unsigned long)&mydata;
	mydata.timer.function = timer_handler;
	//printk("write");
	add_timer(&mydata.timer);*/
	//timer.expires = get_jiffies_64() + 2 * HZ;
	//timer.data = 0;
	//timer.function = timer_handler;
	//add_timer(&timer);
}

void timer_end_handler(unsigned long arg) {
	/*printk("End of stopwatch\n");
	del_timer(&timer);
	__wake_up(&wq_write, 1, 1, NULL);
	*/
}

int device_init(void) {
    int result;

	led_addr = ioremap(LED_ADDRESS, 0x1);
	text_lcd_addr = ioremap(TEXT_LCD_ADDRESS, 0x32);
    
	// register major number, device driver name, and file operation structure
    result = register_chrdev(DEV_MAJOR, DEV_NAME, &dev_fops);
    if(result < 0) {
        printk("major number cannot be registered\n");
        return result;
    }
    // fnd - iomap
    
    printk("init module %s, major number : %d\n", DEV_NAME, DEV_MAJOR);
    
    return 0;
}

void device_exit(void) {
	iounmap(led_addr);
	iounmap(text_lcd_addr);
    unregister_chrdev(DEV_MAJOR, DEV_NAME);
}

module_init(device_init);
module_exit(device_exit);

MODULE_LICENSE("GPL");
