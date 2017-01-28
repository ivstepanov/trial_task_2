#ifndef __RTC__DRV__H
#define __RTC__DRV__H

#define __devinit
#define __devexit

#define DRIVER_NAME 			"rtc_drv"
#define RTC_PROC_ENTRY_NAME		"driver/rtc_drv"
#define MY_TIMER_TIMEOUT		1000
#define ACCEL_FACTOR			1000
#define SLOW_FACTOR				20
#define RTC_ACCEL_MODE_NAME 	"RTC_ACCEL"
#define RTC_SLOW_MODE_NAME 		"RTC_SLOW"
#define RTC_RAND_MODE_NAME 		"RTC_RAND"
#define RTC_UNKNOWN_MODE_NAME 	"RTC_UNKNOWN"
#define MSG_BUF_MAX_LEN			256
#define RTC_SET_MODE_CMD 		"mode="


#endif // #ifndef __RTC__DRV__H
