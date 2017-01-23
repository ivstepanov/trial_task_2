#include <linux/init.h> 
#include <linux/module.h> 
#include "rtc_drv.h" 

MODULE_LICENSE( "GPL" ); 
MODULE_AUTHOR( "ilja stepanov <ilja.stepanov@gmail.com>" ); 

static int __init rtc_drv_init( void ) { 
   printk("** module rtc_drv start!\n");
   return 0; 
} 
static void __exit rtc_drv_exit( void ) { 
   printk( "** module rtc_drv unloaded!\n" );
} 
module_init( rtc_drv_init ); 
module_exit( rtc_drv_exit );
