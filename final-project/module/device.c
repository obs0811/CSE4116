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
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/version.h>

#include <linux/ioctl.h>

/* macros */
#define DEV_MAJOR 242
#define DEV_NAME  "/dev/dev_driver"

#define FND_ADDRESS 0x08000004 // pysical address
#define LED_ADDRESS 0x08000016 // pysical address
#define DOT_ADDRESS 0x08000210 // pysical address
#define TEXT_LCD_ADDRESS 0x08000090 // pysical address - 32 Byte (16 * 2)

#define IOCTL_PIANO _IOR(DEV_MAJOR, 0, char *)

// global variable
static int fnd_port_usage = 0;
static unsigned char *fnd_addr;
static int led_port_usage = 0;
static unsigned char *led_addr;
static int dot_port_usage = 0;
static unsigned char *dot_addr;
static int text_lcd_port_usage = 0;
static unsigned char *text_lcd_addr;
int scale, octave;	// pressed key's scale and octave are decoded
int flag = 0;

// data transferred from user program
struct file *device_file;

// function prototype
int device_open(struct inode *minode, struct file *mfile);
int device_release(struct inode *minode, struct file *mfile);
int device_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what);
long device_ioctl(struct file *f, unsigned int cmd, unsigned long arg);

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

int device_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what)
{
    int i;
    unsigned char value[4];
    unsigned short int value_short = 0;
    const char *tmp = gdata;
	unsigned short led_num[9] = {128, 64, 32, 16, 8, 4, 2, 1};
	unsigned char dot_number[10][10] = {
		{0x7f,0x40,0x40,0x40,0x40,0x7f,0x08,0x08,0x08,0x7f}, // do
		{0x79,0x09,0x0B,0x0B,0x7B,0x47,0x43,0x43,0x41,0x79}, // le
		{0x7d,0x45,0x45,0x45,0x45,0x45,0x45,0x45,0x45,0x7d}, // me
		{0x7e,0x2a,0x2a,0x2a,0x2a,0x2b,0x2a,0x2a,0x2a,0x7e}, // pa
		{0x08,0x14,0x49,0x7f,0x00,0x7f,0x01,0x7f,0x40,0x7f}, // sol
		{0x7a,0x0a,0x0a,0x0a,0x7b,0x43,0x42,0x42,0x42,0x7a}, // la
		{0x01,0x01,0x01,0x11,0x29,0x45,0x01,0x01,0x01,0x01}, // si
		{0x22,0x7f,0x22,0x7f,0x22,0x08,0x08,0x0E,0x0A,0x0C}, // flat
		{0x3e,0x7f,0x63,0x63,0x7f,0x7f,0x63,0x63,0x7f,0x3e}, //not used
		{0x3e,0x7f,0x63,0x63,0x7f,0x3f,0x03,0x03,0x03,0x03}  //not used
	};
	unsigned short led_value;
	unsigned short dot_value;
	unsigned short int lcd_value;
	char str_scale[8] = "CDEFGAB";
	int scale_dec;
	int temp[4] = {0, 0, 0, 0};
	// test
	//for(i = 0; i < 4; i++)
	//	printk("[device_write] gdata[%d]: %d\n", i, gdata[i]);
	printk("write called\n");
    // copy GDATA to an array VALUE
    //if (copy_from_user(&value, tmp, 2))
    //    return -EFAULT;
   
	// fnd
	temp[3] = octave;
    value_short = temp[0] << 12 | temp[1] << 8 | temp[2] << 4 | temp[3];
	//value_short = octave;
    printk("value_short: %d\n", value_short);

    outw(value_short, (unsigned int)fnd_addr);

    // led
	for(i = 0; i < 7; i++) {
		if((char)scale == str_scale[i]) {
			scale_dec = i;
			break;
		}
	}
	printk("scale_dec: %d\n", scale_dec);
	led_value = led_num[scale_dec];
	outw(led_value, (unsigned int)led_addr);
    
	
    // dot matrix
	if(flag == 1) {
		scale_dec = 7;
		flag=0;
	}
	for(i = 0; i < 10; i++) {
		dot_value = dot_number[scale_dec][i] & 0x7F;
		outw(dot_value, (unsigned int)dot_addr+i*2);
	}

	// fill in text lcd
	// write to text lcd
	/*for(i = 0; i < 32; i++) {
		lcd_value = (text_lcd[i] & 0xFF) << 8 | (text_lcd[i+1] & 0xFF);
		outw(lcd_value, (unsigned int)text_lcd_addr+i);
		i++;
	}*/
    return length;
}

long device_ioctl(struct file *f, unsigned int cmd, unsigned long arg) {
	int sound_sum = 0;
	char gdata[4], len = 2;
	printk("[device driver] ioctl is called\n");
	if(cmd == IOCTL_PIANO) {
		// decode the given argument
		sound_sum = arg;
		printk("sound_sum = %d!\n", sound_sum);
		if(sound_sum > 1000) {
			flag = 1;
			sound_sum -= 1000;
		}
		printk("sound_sum: %d\n", sound_sum);
		scale = sound_sum % 100;
		octave = sound_sum / 100;
		printk("scale: %d, octave: %d\n", scale, octave);
		// initialize some global variables
		device_file = f;	// initialize device_file: opened file
		gdata[0] = scale;
		gdata[1] = octave;
		device_write(device_file, gdata, 2, 0);	// write to LED, FND, Dot matrix, and text LCD
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
}

module_init(device_init);
module_exit(device_exit);

MODULE_LICENSE("GPL");
