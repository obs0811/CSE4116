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
#define DEV_MAJOR 242
#define DEV_NAME  "/dev/stopwatch"

#define FND_ADDRESS 0x08000004 // pysical address

// global variable
static int fnd_port_usage = 0;
static unsigned char *fnd_addr;

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

// function prototype
int device_open(struct inode *minode, struct file *mfile);
int device_release(struct inode *minode, struct file *mfile);
int device_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what);
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
	printk(KERN_ALERT "interrupt1!!! = %x\n", gpio_get_value(IMX_GPIO_NR(1, 11)));
	if(!stopwatch_on) {	 // if Home button is pressed for the first time, add timer
		printk("start is pressed\n");
		stopwatch_on = 1;
		timer.expires = get_jiffies_64() + HZ;
		timer.data = 0;
		timer.function = timer_handler;
		add_timer(&timer);
	}
	return IRQ_HANDLED;
}

// pause
irqreturn_t inter_handler2(int irq, void* dev_id, struct pt_regs* reg) {
	printk(KERN_ALERT "interrupt2!!! = %x\n", gpio_get_value(IMX_GPIO_NR(1, 12)));
	
	if(stopwatch_on) {	// if stopwatch is ticking on, delete timer from the timer list
		stopwatch_on = 0;
		del_timer(&timer);
	}
	return IRQ_HANDLED;
}

// reset
irqreturn_t inter_handler3(int irq, void* dev_id,struct pt_regs* reg) {
	printk(KERN_ALERT "interrupt3!!! = %x\n", gpio_get_value(IMX_GPIO_NR(2, 15)));
	if(stopwatch_on) {	// if stopwatch is ticking on, delete timer from the timer list
		stopwatch_on = 0;
		del_timer(&timer);
	}
	time_cnt = 0;
	outw(0, (unsigned int)fnd_addr);
	//del_timer(&timer);
    return IRQ_HANDLED;
}

irqreturn_t inter_handler4(int irq, void* dev_id, struct pt_regs* reg) {
	printk(KERN_ALERT "interrupt4!!! = %x\n", gpio_get_value(IMX_GPIO_NR(5, 14)));
	if(gpio_get_value(IMX_GPIO_NR(5, 14)) == 0) {	// if trigger falling
		printk("added timer_end_handler\n");
		timer_end.expires = get_jiffies_64() + 3 * HZ;
		timer_end.data = 0;
		timer_end.function = timer_end_handler;
		add_timer(&timer_end);	// timer_end_handler() is called in 3 seconds
	}
	else {	// if trigger rising
		del_timer(&timer_end);	// delete timer_end from the timer list
	}
	/*
	outw(0, (unsigned int)fnd_addr);
	del_timer_sync(&timer);
	__wake_up(&wq_write, 1, 1, NULL);
	*/
    return IRQ_HANDLED;
}

// define functions
int device_open(struct inode *minode, struct file *mfile)
{
	int irq, ret;

    // open fnd
    if(fnd_port_usage != 0) return -EBUSY;
    fnd_port_usage = 1;
    
	// register devices to irq table
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
	ret=request_irq(irq, inter_handler4, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "voldown", 0);
    return 0;
}

int device_release(struct inode *minode, struct file *mfile)
{
    // close fnd
    fnd_port_usage = 0;
	del_timer(&timer);
    free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 12)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);
    return 0;
}

int device_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what)
{
	// stopwatch timer
	init_timer(&timer);
	// timer to end stopwatch
	init_timer(&timer_end);
	//add_timer(&timer);
	printk("process sleeps\n");
	interruptible_sleep_on(&wq_write);	// put the current process to wait queue until the interrupt handler wakes it up
	printk("process wakes up\n");
	return length;
}

void timer_handler(unsigned long arg) {
	unsigned char gdata[5];
	unsigned short int val_fnd = 0;
		
	printk("* time_cnt: %d\n", time_cnt);
	if(stopwatch_on)	time_cnt++;		// if stopwatch_on is set, increment timer count
	// store timer count in "minute:second" form
	gdata[0] = (time_cnt / 60) / 10;
	gdata[1] = (time_cnt / 60) % 10;
	gdata[2] = (time_cnt % 60) / 10;
	gdata[3] = (time_cnt % 60) % 10;
	val_fnd = gdata[0] << 12 | gdata[1] << 8 | gdata[2] << 4 | gdata[3];
	outw(val_fnd, (unsigned int)fnd_addr);	// write timer count to FND device
	
	// add timer structure so that after the current timer expires, it can be called again
	timer.expires = get_jiffies_64() + HZ;
	timer.data = 0;
	timer.function = timer_handler;
	add_timer(&timer);
}

void timer_end_handler(unsigned long arg) {
	printk("End of stopwatch\n");
	outw(0, (unsigned int)fnd_addr);
	if(stopwatch_on)	del_timer(&timer);
	//del_timer(&timer);
	__wake_up(&wq_write, 1, 1, NULL);	// wake up the process currently in wait queue
}

int device_init(void) {
    int result;
    
	// register major number, device driver name, and file operation structure
    result = register_chrdev(DEV_MAJOR, DEV_NAME, &dev_fops);
    if(result < 0) {
        printk("major number cannot be registered\n");
        return result;
    }
    // fnd - iomap
    fnd_addr = ioremap(FND_ADDRESS, 0x4);
    
    printk("init module %s, major number : %d\n", DEV_NAME, DEV_MAJOR); 
    return 0;
}

void device_exit(void) {
    // fnd
    iounmap(fnd_addr);
    unregister_chrdev(DEV_MAJOR, DEV_NAME);
}

module_init(device_init);
module_exit(device_exit);

MODULE_LICENSE("GPL");
