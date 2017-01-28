#include <linux/interrupt.h>
#include <linux/init.h> 
#include <linux/module.h> 
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/seq_file.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/random.h>
#include <linux/proc_fs.h>
#include "rtc_drv.h" 


MODULE_LICENSE( "GPL" ); 
MODULE_AUTHOR( "Ilja Stepanov <ilja.stepanov@gmail.com>" ); 

struct rtc_drv {
	struct rtc_device *rtc_dev;
	struct rtc_time rtc_alarm;
	struct rtc_time time_now;
};

static struct timer_list my_timer;
static struct platform_device *rtc_test0 = NULL;
static struct rtc_drv *my_rtc;
static time_t total_sec, cur_sec;
static enum e_rtc_mode {
	RTC_ACCEL, RTC_SLOW, RTC_RAND, RTC_UNKNOWN
} rtc_mode = RTC_ACCEL;

static void set_rtc(time_t sec, int offset, struct rtc_drv *rtc)
{
	struct tm tm;

	time_to_tm(sec, offset, &tm);
	rtc->time_now.tm_sec = tm.tm_sec;
	rtc->time_now.tm_min = tm.tm_min;
	rtc->time_now.tm_hour = tm.tm_hour;
	rtc->time_now.tm_mday = tm.tm_mday;
	rtc->time_now.tm_mon = tm.tm_mon;
	rtc->time_now.tm_year = tm.tm_year;
	rtc->time_now.tm_wday = tm.tm_wday;
	rtc->time_now.tm_yday = tm.tm_yday;
}

static void my_timer_callback(unsigned long data)
{
	int ret;
	int offset;

	ret = mod_timer(&my_timer, jiffies + msecs_to_jiffies(MY_TIMER_TIMEOUT));
	if (ret) {
		printk("** Error in mod_timer\n");
		return;
	}

	total_sec++;
	if (my_rtc == NULL) {
		return;
	}

	offset = 0;
	cur_sec = total_sec;

	switch (rtc_mode) {
	case RTC_ACCEL:
		cur_sec *= ACCEL_FACTOR;
		break;

	case RTC_SLOW:
		cur_sec /= SLOW_FACTOR;
		break;

	case RTC_RAND:
		get_random_bytes(&offset, 2);
		total_sec += offset;
		break;

	default:
		return;
	}

	set_rtc(cur_sec, offset, my_rtc);
}


static int rtc_drv_ioctl(struct device *dev, unsigned int cmd, unsigned long arg)
{
	//struct rtc_drv *rtc = dev_get_drvdata(dev);
	int ret = 0;

	printk("%s.%d cmd=0x%x\n", __FUNCTION__, __LINE__, cmd);
	switch (cmd) {
	case RTC_UIE_ON:
		break;

	case RTC_UIE_OFF:
		break;

	case RTC_AIE_ON:
		break;

	case RTC_AIE_OFF:
		break;

	default:
		ret = -ENOIOCTLCMD;
	}

	return ret;
}

static int rtc_drv_read_time(struct device *dev, struct rtc_time *tm)
{
	struct rtc_drv *rtc = dev_get_drvdata(dev);
	printk("%s.%d\n", __FUNCTION__, __LINE__);
	if (tm == NULL) {
		return -EINVAL;
	}
	tm->tm_sec = rtc->time_now.tm_sec;
	tm->tm_min = rtc->time_now.tm_min;
	tm->tm_hour = rtc->time_now.tm_hour;
	tm->tm_mday = rtc->time_now.tm_mday;
	tm->tm_mon = rtc->time_now.tm_mon;
	tm->tm_year = rtc->time_now.tm_year;
	return 0;
}

static int rtc_drv_set_time(struct device *dev, struct rtc_time *tm)
{
	//struct rtc_drv *rtc = dev_get_drvdata(dev);
	int ret = 0;

	printk("%s.%d\n", __FUNCTION__, __LINE__);
	if (tm == NULL) {
		return -EINVAL;
	}
	printk("year: %d\n", tm->tm_year);
	printk("mon: %d\n", tm->tm_mon);
	printk("day: %d\n", tm->tm_mday);
	printk("hour: %d\n", tm->tm_hour);
	printk("min: %d\n", tm->tm_min);
	printk("sec: %d\n", tm->tm_sec);
	printk("total_sec=%ld\n", total_sec);
	total_sec = (time_t)(unsigned int)mktime(tm->tm_year + 1900, tm->tm_mon + 1, 
			tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	switch (rtc_mode) {
	case RTC_SLOW:
		total_sec *= SLOW_FACTOR;
		break;

	case RTC_ACCEL:
		total_sec /= ACCEL_FACTOR;
		break;

	default:
		break;
	}
	printk("total_sec=%ld\n", total_sec);
	return ret;
}

static int rtc_drv_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct rtc_drv *rtc = dev_get_drvdata(dev);

	if (alrm != NULL) {
		alrm->time.tm_sec = rtc->rtc_alarm.tm_sec;
		alrm->enabled = 0;
	}
	printk("%s.%d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int rtc_drv_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	//struct rtc_drv *rtc = dev_get_drvdata(dev);
	printk("%s.%d\n", __FUNCTION__, __LINE__);
	return 0;
}

static int rtc_drv_proc(struct device *dev, struct seq_file *seq)
{
	seq_printf(seq,
		"alarm_IRQ\t: %s\n"
		"wkalarm_IRQ\t: %s\n"
		"seconds_IRQ\t: %s\n",
		"x",
		"x",
		"x");
	return 0;
}

static struct rtc_class_ops rtc_drv_ops = {
	.ioctl         = rtc_drv_ioctl,
	.read_time     = rtc_drv_read_time,
	.set_time      = rtc_drv_set_time,
	.read_alarm    = rtc_drv_read_alarm,
	.set_alarm     = rtc_drv_set_alarm,
	.proc          = rtc_drv_proc,
};

static ssize_t rtc_drv_mode_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	ssize_t ret_val;

	switch (rtc_mode) {
	case RTC_ACCEL:
		ret_val = sprintf(buf, "%s\n", RTC_ACCEL_MODE_NAME);
		break;

	case RTC_SLOW:
		ret_val = sprintf(buf, "%s\n", RTC_SLOW_MODE_NAME);
		break;

	case RTC_RAND:
		ret_val = sprintf(buf, "%s\n", RTC_RAND_MODE_NAME);
		break;

	default:
		ret_val = sprintf(buf, "%s\n", RTC_UNKNOWN_MODE_NAME);
		break;
	}
	return ret_val;
}

static ssize_t rtc_drv_mode_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	size_t len;

	printk("%s.%d: %s total_sec=%ld\n", __FUNCTION__, __LINE__, buf, total_sec);
	len = strlen(RTC_ACCEL_MODE_NAME);
	if (strncmp(buf, RTC_ACCEL_MODE_NAME, len) == 0) {
		total_sec = cur_sec;
		rtc_mode = RTC_ACCEL;
		printk("total_sec=%ld\n", total_sec);
		return count;
	}

	len = strlen(RTC_SLOW_MODE_NAME);
	if (strncmp(buf, RTC_SLOW_MODE_NAME, len) == 0) {
		total_sec = cur_sec * SLOW_FACTOR;
		rtc_mode = RTC_SLOW;
		printk("total_sec=%ld\n", total_sec);
		return count;
	}

	len = strlen(RTC_RAND_MODE_NAME);
	if (strncmp(buf, RTC_RAND_MODE_NAME, len) == 0) {
		total_sec = cur_sec;
		rtc_mode = RTC_RAND;
		printk("total_sec=%ld\n", total_sec);
		return count;
	}

	return -EINVAL;
}

static char *msg_buf;

static int rtc_drv_seq_read(struct seq_file *seq, void *offset)
{
	struct rtc_drv *rtc = seq->private;
	struct rtc_time *tm;

	tm = &rtc->time_now;
	//seq_printf(seq, "%s.%d: ok\n", __FUNCTION__, __LINE__);
	seq_printf(seq, "%04d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, 
			tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	if (msg_buf) {
		rtc_drv_mode_show(NULL, NULL, msg_buf);
		seq_printf(seq, " mode=%s\n", msg_buf);
	} else {
		seq_printf(seq, "\n");
	}
	return 0;
}

ssize_t rtc_drv_proc_write(struct file *filp, const char *buf, size_t count, loff_t *offp)
{
	size_t copy_count;
	size_t rest_count;
	size_t tmp_len;
	char *tmp_str, *end_str;
	long tmp_val;
	struct rtc_time rtc;

	printk("%s.%d\n", __FUNCTION__, __LINE__);
	memset(&rtc, 0, sizeof(rtc));

	printk("count=%ld\n", count);
	if (msg_buf == NULL || count >= MSG_BUF_MAX_LEN) {
		printk("%s.%d error!\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}
	copy_count = copy_from_user(msg_buf, buf, count);
	msg_buf[count] = '\0';

	if (count > strlen(RTC_SET_MODE_CMD)) {
		tmp_str = strstr(msg_buf, RTC_SET_MODE_CMD);
		
		if (tmp_str) {
			tmp_len = strlen(RTC_SET_MODE_CMD);
			tmp_str = &msg_buf[tmp_len];
			rest_count = count - tmp_len;
			rtc_drv_mode_store(NULL, NULL, tmp_str, rest_count);
			return count;
		}
	}

	printk("msg_buf=%s\n", msg_buf);
	tmp_str = msg_buf;
	// year
	end_str = strstr(tmp_str, "-");
	if (end_str == NULL) {
		printk("%s.%d error!\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}
	tmp_val = simple_strtol(tmp_str, &end_str, 10);
	printk("year: %ld\n", tmp_val);
	if (tmp_val < 1970) {
		printk("%s.%d error!\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}
	rtc.tm_year = tmp_val - 1900;

	// month
	tmp_str = end_str + 1;
	end_str = strstr(tmp_str, "-");
	if (end_str == NULL) {
		printk("%s.%d error!\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}
	tmp_val = simple_strtol(tmp_str, &end_str, 10);
	printk("month: %ld\n", tmp_val);
	if (tmp_val < 1 || tmp_val > 12) {
		printk("%s.%d error!\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}
	rtc.tm_mon = tmp_val - 1;

	// day
	tmp_str = end_str + 1;
	end_str = strstr(tmp_str, " ");
	if (end_str == NULL) {
		printk("%s.%d error!\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}
	tmp_val = simple_strtol(tmp_str, &end_str, 10);
	printk("day: %ld\n", tmp_val);
	if (tmp_val < 1 || tmp_val > 31) {
		printk("%s.%d error!\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}
	rtc.tm_mday = tmp_val;

	// hour
	tmp_str = end_str + 1;
	end_str = strstr(tmp_str, ":");
	if (end_str == NULL) {
		printk("%s.%d error!\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}
	tmp_val = simple_strtol(tmp_str, &end_str, 10);
	printk("hour: %ld\n", tmp_val);
	if (tmp_val < 0 || tmp_val > 23) {
		printk("%s.%d error!\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}
	rtc.tm_hour = tmp_val;

	// minute
	tmp_str = end_str + 1;
	end_str = strstr(tmp_str, ":");
	if (end_str == NULL) {
		printk("%s.%d error!\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}
	tmp_val = simple_strtol(tmp_str, &end_str, 10);
	printk("minute: %ld\n", tmp_val);
	if (tmp_val < 0 || tmp_val > 59) {
		printk("%s.%d error!\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}
	rtc.tm_min = tmp_val;

	// second
	tmp_str = end_str + 1;
	if (tmp_str == NULL) {
		printk("%s.%d error!\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}
	tmp_val = simple_strtol(tmp_str, &end_str, 10);
	printk("second: %ld\n", tmp_val);
	if (tmp_val < 0 || tmp_val > 59) {
		printk("%s.%d error!\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}
	rtc.tm_sec = tmp_val;
	rtc_drv_set_time(NULL, &rtc);
	return (ssize_t)count;
}

static int rtc_drv_proc_open(struct inode *inode, struct file *file)
{
	struct rtc_drv *rtc = PDE_DATA(inode);

	printk("%s.%d\n", __FUNCTION__, __LINE__);
	if (!try_module_get(THIS_MODULE)) {
		printk("%s.%d error!\n", __FUNCTION__, __LINE__);
		return -ENODEV;
	}
	msg_buf = kzalloc(sizeof(*msg_buf) * MSG_BUF_MAX_LEN, GFP_KERNEL);
	return single_open(file, rtc_drv_seq_read, rtc);
}

static int rtc_drv_proc_release(struct inode *inode, struct file *file)
{
	int res = single_release(inode, file);
	printk("%s.%d\n", __FUNCTION__, __LINE__);
	if (msg_buf) {
		kfree(msg_buf);
		msg_buf = NULL;
	}
	module_put(THIS_MODULE);
	return res;
}


static const struct file_operations rtc_drv_proc_fops = {
	.open		= rtc_drv_proc_open,
	.read		= seq_read,
	.write		= rtc_drv_proc_write,
	.llseek		= seq_lseek,
	.release	= rtc_drv_proc_release,
};


void rtc_drv_proc_add_device(struct rtc_drv *rtc)
{
	struct proc_dir_entry *ent;

	ent = proc_create_data(RTC_PROC_ENTRY_NAME, S_IWUSR | S_IWUGO | S_IRUGO, NULL,
				       &rtc_drv_proc_fops, rtc);
	if (ent == NULL) {
		printk("%s.%d error!\n", __FUNCTION__, __LINE__);
	}
}

void rtc_drv_proc_del_device(void)
{
	remove_proc_entry(RTC_PROC_ENTRY_NAME, NULL);
}

static DEVICE_ATTR(mode, S_IRUGO | S_IWUSR | S_IWUGO, 
				   rtc_drv_mode_show, rtc_drv_mode_store);

static int __devinit rtc_drv_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret = 0;

	printk("** rtc_drv_probe start\n");

	setup_timer(&my_timer, my_timer_callback, 0);

	printk( "** starting timer...\n");
	ret = mod_timer(&my_timer, jiffies + msecs_to_jiffies(MY_TIMER_TIMEOUT));
	if (ret) {
		printk("** error in mod_timer\n");
	}

	/* Allocate memory for our RTC struct */
	my_rtc = kzalloc(sizeof(*my_rtc), GFP_KERNEL);
	if (unlikely(!my_rtc)) {
		printk("%s.%d: error\n", __FUNCTION__, __LINE__);
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, my_rtc);

	/* Register our RTC with the RTC framework */
	my_rtc->rtc_dev = rtc_device_register(pdev->name, dev, &rtc_drv_ops, THIS_MODULE);
	if (unlikely(IS_ERR(my_rtc->rtc_dev))) {
		ret = PTR_ERR(my_rtc->rtc_dev);
		printk("%s.%d: error\n", __FUNCTION__, __LINE__);
		goto _err_exit;
	}

	ret = device_create_file(&pdev->dev, &dev_attr_mode);
	if (ret) {
		printk("%s.%d: can't create mode attribute\n", __FUNCTION__, __LINE__);;
	} else {
		printk("%s.%d: done\n", __FUNCTION__, __LINE__);
	}

	rtc_drv_proc_add_device(my_rtc);

	return 0;

_err_exit:
	kfree(my_rtc);
	my_rtc = NULL;
	return ret;
}

static int __devexit rtc_drv_remove(struct platform_device *pdev)
{
	struct rtc_drv *rtc = platform_get_drvdata(pdev);

	rtc_drv_proc_del_device();
	rtc_device_unregister(rtc->rtc_dev);
	platform_set_drvdata(pdev, NULL);
	kfree(rtc);

	return 0;
}

static struct platform_driver rtc_platf_drv = {
	.driver		= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
	},
	.probe		= rtc_drv_probe,
	.remove		= rtc_drv_remove,
	.suspend	= NULL,
	.resume		= NULL,
};

static int __init rtc_drv_init(void)
{
	int ret_val = 0;

	printk("** module rtc_drv start!\n");
	ret_val = platform_driver_register(&rtc_platf_drv);
	printk("** platform_driver_register return: %d\n", ret_val);
	if (ret_val) {
		printk("%s.%d: error\n", __FUNCTION__, __LINE__);
		goto _exit;
	}
	printk("%s.%d\n", __FUNCTION__, __LINE__);

	rtc_test0 = platform_device_alloc(DRIVER_NAME, 0);
	if (rtc_test0 == NULL) {
		ret_val = -ENOMEM;
		printk("%s.%d: error\n", __FUNCTION__, __LINE__);
		goto _exit;
	}

	ret_val = platform_device_add(rtc_test0);
	if (ret_val) {
		printk("%s.%d: error\n", __FUNCTION__, __LINE__);
		goto _exit;
	}
	printk("%s.%d: done\n", __FUNCTION__, __LINE__);

_exit:
	return ret_val;
}
 
static void __exit rtc_drv_exit(void)
{
	int ret;

	ret = del_timer( &my_timer );
	if (ret) {
		printk("** The timer is still in use...\n");
	}
	if (rtc_test0 != NULL) {
		platform_device_unregister(rtc_test0);
	}
	platform_driver_unregister(&rtc_platf_drv);
	printk( "** module rtc_drv unloaded!\n" );
} 

module_init(rtc_drv_init); 
module_exit(rtc_drv_exit);

