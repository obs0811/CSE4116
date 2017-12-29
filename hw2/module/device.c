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
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/version.h>

#include <linux/ioctl.h>
#include <linux/timer.h>

/* macros */
#define DEV_MAJOR 242
#define DEV_NAME  "/dev/dev_driver"

#define FND_ADDRESS 0x08000004 // pysical address
#define LED_ADDRESS 0x08000016 // pysical address
#define DOT_ADDRESS 0x08000210 // pysical address
#define TEXT_LCD_ADDRESS 0x08000090 // pysical address - 32 Byte (16 * 2)

#define IOCTL_TIMER _IOR(DEV_MAJOR, 0, char *)

// global variable
static int fnd_port_usage = 0;
static unsigned char *fnd_addr;
static int led_port_usage = 0;
static unsigned char *led_addr;
static int dot_port_usage = 0;
static unsigned char *dot_addr;
static int text_lcd_port_usage = 0;
static unsigned char *text_lcd_addr;

// timer structure
struct timer_list timer;
// a variable to check timer termination condition
static int timer_count = 0;

// data transferred from user program
int interval, cnt, start_pos, start_val;
struct file *device_file;
int val, pos;
int cur_pos_sn, cur_pos_name;
int plus_sn, plus_name;
char student_number[10] = "20121608";
char student_name[15] = "ByungsooOh";

// function prototype
int device_open(struct inode *minode, struct file *mfile);
int device_release(struct inode *minode, struct file *mfile);
int device_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what);
long device_ioctl(struct file *f, unsigned int cmd, unsigned long arg);
void timer_handler(unsigned long arg);

// file_operations structure
struct file_operations dev_fops =
{
    .open           =	device_open,
    .release        =	device_release,
    .write          =	device_write,
    .unlocked_ioctl =   device_ioctl,
};


// define functions
int device_open(struct inode *minode, struct file *mfile)
{
    // open fnd
    if(fnd_port_usage != 0) return -EBUSY;
    fnd_port_usage = 1;
    
    // open led
    if(led_port_usage != 0) return -EBUSY;
    led_port_usage = 1;

    // open dot matrix
    if(dot_port_usage != 0) return -EBUSY;
    dot_port_usage = 1;
    
    // open text lcd
    if(text_lcd_port_usage != 0) return -EBUSY;
    text_lcd_port_usage = 1;
    
    return 0;
}

int device_release(struct inode *minode, struct file *mfile)
{
    // close fnd
    fnd_port_usage = 0;
    
    // close led
    led_port_usage = 0;
    
    // close dot matrix
    dot_port_usage = 0;
    
    // close text lcd
    text_lcd_port_usage = 0;
    
    return 0;
}

/* this function is called periodically from timer_handler().
   it writes to fnd, led, text editor, and dot matrix using outw() 
*/
int device_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what)
{
    int i;
    unsigned char value[4];
    unsigned short int value_short = 0;
    const char *tmp = gdata;
	unsigned short led_num[9] = {0, 128, 64, 32, 16, 8, 4, 2, 1};
	unsigned char dot_number[10][10] = {
		{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 0
		{0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e}, // 1
		{0x7e,0x7f,0x03,0x03,0x3f,0x7e,0x60,0x60,0x7f,0x7f}, // 2
		{0xfe,0x7f,0x03,0x03,0x7f,0x7f,0x03,0x03,0x7f,0x7e}, // 3
		{0x66,0x66,0x66,0x66,0x66,0x66,0x7f,0x7f,0x06,0x06}, // 4
		{0x7f,0x7f,0x60,0x60,0x7e,0x7f,0x03,0x03,0x7f,0x7e}, // 5
		{0x60,0x60,0x60,0x60,0x7e,0x7f,0x63,0x63,0x7f,0x3e}, // 6
		{0x7f,0x7f,0x63,0x63,0x03,0x03,0x03,0x03,0x03,0x03}, // 7
		{0x3e,0x7f,0x63,0x63,0x7f,0x7f,0x63,0x63,0x7f,0x3e}, // 8
		{0x3e,0x7f,0x63,0x63,0x7f,0x3f,0x03,0x03,0x03,0x03} // 9
	};
	char text_lcd[32];
	int len_sn = 8, len_name = 10;
	unsigned short led_value;
	unsigned short dot_value;
	unsigned short int lcd_value;
	int end = cur_pos_sn + len_sn;

	// test
	//for(i = 0; i < 4; i++)
	//	printk("[device_write] gdata[%d]: %d\n", i, gdata[i]);

    // copy GDATA to an array VALUE
    if (copy_from_user(&value, tmp, 4))
        return -EFAULT;
   
	// fnd
    value_short = value[0] << 12 | value[1] << 8 |value[2] << 4 |value[3];
    outw(value_short,(unsigned int)fnd_addr);
    
    // led
	led_value = led_num[val];
	outw(led_value, (unsigned int)led_addr);
    
    // dot matrix
	for(i = 0; i < 10; i++) {
		dot_value = dot_number[val][i] & 0x7F;
		outw(dot_value, (unsigned int)dot_addr+i*2);
	}

	// text lcd
	// initialize text lcd
	for(i = 0; i < 32; i++) {
		text_lcd[i] = 0;
	}
	// fill in text lcd
	for(i = cur_pos_sn; i < end; i++) {
		text_lcd[i] = student_number[i-cur_pos_sn];
	}
	end = cur_pos_name + len_name;
	for(i = cur_pos_name; i < end; i++) {
		text_lcd[i] = student_name[i-cur_pos_name];
	}
	// write to text lcd
	for(i = 0; i < 32; i++) {
		lcd_value = (text_lcd[i] & 0xFF) << 8 | (text_lcd[i+1] & 0xFF);
		outw(lcd_value, (unsigned int)text_lcd_addr+i);
		i++;
	}
	// update position of student number and name
	// student number
	if(plus_sn) {	// direction: ->
		if(cur_pos_sn < len_sn)
			cur_pos_sn++;
		else {
			plus_sn = 0;
			cur_pos_sn--;
		}
	}
    else  {	// direction: <-
		if(cur_pos_sn > 0)
			cur_pos_sn--;
		else {
			plus_sn = 1;
			cur_pos_sn++;
		}
	}
	// student name
	if(plus_name) {	// direction: ->
		if(cur_pos_name < 32 - len_name)
			cur_pos_name++;
		else {
			plus_name = 0;
			cur_pos_name--;
		}
	}
	else {	// direction: <-
		if(cur_pos_name > 16)
			cur_pos_name--;
		else {
			plus_name = 1;
			cur_pos_name++;
		}
	}
    return length;
}

void timer_handler(unsigned long arg) {
	//unsigned long data = timer.data;
	char gdata[4] = {0, 0, 0, 0};
	int i;	

	// periodically call device_write() to output to FND, LED, Dot matrix, and Text LCD
	gdata[pos] = val;
	device_write(device_file, gdata, 4, 0);
	// update VAL and POS (VAL and POS each represents the value and position of number
	// to write on FND device)
	timer_count++;
	val = val % 8 + 1;
	if(timer_count && timer_count % 8 == 0)	
		pos = (pos+1) % 4;

	printk("[timer] %d %d %d %d\n", start_pos, start_val, interval, cnt);
	printk("[timer] count: %d\n", timer_count);
	
	// terminate if the number of timer handler calls exceeds the specified count number
	if(timer_count > cnt) {
		gdata[0] = gdata[1] = gdata[2] = gdata[3] = 0;
		val = 0;
		for(i = 0; i < 8; i++)
			student_number[i] = 0;
		for(i = 0; i < 10; i++)
			student_name[i] = 0;
		device_write(device_file, gdata, 4, 0);
		return;
	}
	// add timer structure so that after the current timer expires, it can be called again
	timer.expires = get_jiffies_64() + (interval*HZ/10);
	timer.data = arg;
	timer.function = timer_handler;
	add_timer(&timer);
}

long device_ioctl(struct file *f, unsigned int cmd, unsigned long arg) {
	char num[10] = "20121608";
	char name[15] = "ByungsooOh";
	int i;

	//printk("[device driver] ioctl is called\n");
	if(cmd == IOCTL_TIMER) {
		//printk("cmd = IOCTL_TIMER\n");
		// decode the given argument
		start_pos = arg >> 24;
		start_val = (arg >> 16) & (0x000000ff);
		interval = (arg >> 8) & (0x000000ff);
		cnt = arg & (0x000000ff);
		//printk("%d %d %d %d\n", start_pos, start_val, interval, cnt);

		// initialize some global variables
		device_file = f;	// initialize device_file: opened file
		pos = start_pos;	// initialize pos: the position of the number to write on FND
		val = start_val;	// initialize val: the value of the number to write on FND
		// initialize the first indices of strings to output to Text LCD
		cur_pos_sn = 0;
		cur_pos_name = 16;
		// set direction of strings written to Text LCD to right
		plus_sn = plus_name = 1;
		for(i = 0; i < 8; i++)
			student_number[i] = num[i];
		for(i = 0; i < 10; i++)
			student_name[i] = name[i];

		// set the fields of timer structure TIMER and add it to timer_list
		init_timer(&timer);
		timer.expires = get_jiffies_64() + (interval*HZ/10);
		timer.data = arg;
		timer_count = 0;
		//printk("[ioctl] timer.data = %ld\n", timer.data);
		timer.function = timer_handler;
		add_timer(&timer);
	}
	return 0;
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
    
    // led - iomap
    led_addr = ioremap(LED_ADDRESS, 0x1);
    
    // dot matrix - iomap
    dot_addr = ioremap(DOT_ADDRESS, 0x10);
    
    // text lcd - iomap
    text_lcd_addr = ioremap(TEXT_LCD_ADDRESS, 0x32);

    printk("init module %s, major number : %d\n", DEV_NAME, DEV_MAJOR);
    
    return 0;
}

void device_exit(void) {
    // fnd
    iounmap(fnd_addr);
    // led
    iounmap(led_addr);
    // dot matrix
    iounmap(dot_addr);
    // text lcd
    iounmap(text_lcd_addr);

    unregister_chrdev(DEV_MAJOR, DEV_NAME);
	del_timer(&timer);    // delete the current timer from linked list
}


module_init(device_init);
module_exit(device_exit);

MODULE_LICENSE("GPL");
