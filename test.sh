#!/bin/bash

MODULE_NAME=rtc_drv
MODULE_DRV=$MODULE_NAME.ko
MODULE_PROC_ENTRY=/proc/driver/rtc_drv
CUR_DATE="2017-01-29 14:00:00"
TEST_DATE=""

function eror_exit()
{
    echo "Error! "$1
    sudo rmmod $MODULE_DRV
    echo "Test failed!"
    exit
}

if [ ! -f $MODULE_DRV ]; then
    echo "File $FILE does not exist. Please build module "$MODULE_DRV
else
    sudo insmod $MODULE_DRV
    if [ ! $? -eq 0 ]; then
	echo "Error! Can't insmod "$MODULE_DRV
	exit
    fi
    echo "insmod: OK"

    if [ -f $MODULE_PROC_ENTRY ]; then
	echo "Test start..."
	echo "Show current date/time and mode..."
	cat $MODULE_PROC_ENTRY
	echo "Check default mode is RTC_ACCEL..."
	cat $MODULE_PROC_ENTRY | grep mode=RTC_ACCEL > /dev/null
	if [ ! $? -eq 0 ]; then
	    eror_exit "Wrong default mode (must be RTC_ACCEL)!"
	fi
	echo "OK"

	echo "Set mode RTC_SLOW..."
	echo "mode=RTC_SLOW" > $MODULE_PROC_ENTRY
	if [ !  $? -eq 0 ]; then
	    eror_exit "Can't set mode RTC_SLOW!"
	fi
	cat $MODULE_PROC_ENTRY | grep mode=RTC_SLOW > /dev/null
	if [ ! $? -eq 0 ]; then
	    eror_exit "Wrong  mode (must be RTC_SLOW)!"
	fi
	echo "OK"

	echo "Setup new date time value..."
	CUR_DATE=`date +%Y-%m-%d_%T`
	CUR_DATE=`echo $CUR_DATE | tr '_' ' '`
#	echo $CUR_DATE
	echo $CUR_DATE > $MODULE_PROC_ENTRY
	if [ !  $? -eq 0 ]; then
	    eror_exit "Can't set date/time!!"
	fi
	TEST_DATE=`cat $MODULE_PROC_ENTRY | sed -r 's/mode=RTC_SLOW//g'`
#	echo $TEST_DATE
	echo $CUR_DATE > x1
	echo $TEST_DATE > x2
	diff x1 x2
	if [ ! $? -eq 0 ]; then
	    rm x1 x2
	    eror_exit "Error while set date/time!"
	fi
	rm x1 x2
	echo "OK"
	echo "Setup RTC_RAND mode..."
	echo "mode=RTC_RAND" > $MODULE_PROC_ENTRY
	if [ !  $? -eq 0 ]; then
	    eror_exit "Can't set mode RTC_RAND!"
	fi
	cat $MODULE_PROC_ENTRY | grep mode=RTC_RAND > /dev/null
	if [ ! $? -eq 0 ]; then
	    eror_exit "Wrong  mode (must be RTC_RAND)!"
	fi
	echo "OK"
    else
	echo "Error! Driver proc entry not exist ("$MODULE_PROC_ENTRY")"
    fi

    sudo rmmod $MODULE_NAME
    if [ ! $? -eq 0 ]; then
	echo "Error! Can't rmmod "$MODULE_DRV
	exit
    fi
    echo "Test done!"
fi

