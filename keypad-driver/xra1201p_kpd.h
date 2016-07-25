/*
 * Driver for keys on xra1201p I2C IO expander
 *
 * Copyright (C) 2016 SIM
 *
 * Author : <xipeng.gu@sim.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/timer.h>
#include <linux/err.h>
#include <linux/irq.h>
#include <linux/of_platform.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/xra1201p_kpd.h>
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>

#define XRA1201P_REG_0	0x00
#define XRA1201P_REG_1	0x01
#define XRA1201P_REG_2	0x02
#define XRA1201P_REG_3	0x03
#define XRA1201P_REG_4	0x04
#define XRA1201P_REG_5	0x05
#define XRA1201P_REG_6	0x06
#define XRA1201P_REG_7	0x07

#define XRA1201P_REG_2_VALUE		0x1F //port 0 (0-4) default be high, other be low
#define XRA1201P_REG_3_VALUE		0x00 //prot 1 row (0-7) default be low
#define XRA1201P_REG_6_VALUE		0x00 //port 0 col (0-2)4as output  (5-7) as output(GPIO)
#define XRA1201P_REG_7_VALUE		0x1F //prot 1 row (0-4) as input
#define XRA_GPIO_MASK			0x00FF

#define SETBIT(x,y,z) (x)|=((z)<<(y))
#define CLEARBIT(x,y) (x)&=(~(1<<(y)))
#define GETBIT(x,y) (x)=((x)>>(y))&1

#define  __XRA1201P_DEBUG__
#ifdef __XRA1201P_DEBUG__
#define xra1201p_debug(fmt,...) 	\
	do { \
		printk("xra1201p : %s %d " fmt,__func__,__LINE__, ##__VA_ARGS__); \
	} while(0)
#define __XRA1201P_WRITE_REG_DEBUG__
#else
#define xra1201p_debug(format...) do {} while(0)
#endif

#if 0
#define KEYBOARD_BL_CTRL

#ifdef KEYBOARD_BL_CTRL
#define KEY_BL_BIT	0x20
#define KEY_NUM_BIT	0x40
#define KEY_FN_BIT	0x80
int brightness_value = 0;
#endif
#endif

//static char rember_reg2_value = 0x1F; //tmp
//static char rember_reg3_value = 0xE0;

static struct xra1201p_chip_data* control_chip_external = NULL;

static int gpio_xra1201p_write_reg(struct xra1201p_chip_data *chip, int reg, u16 val)
{
	int retval = 0;

	xra1201p_debug("reg = %d, val = %x\n ", reg, val);

	retval = i2c_smbus_write_byte_data(chip->client, reg, val);
	if (retval < 0) {
		dev_err(&chip->client->dev,
			"%s failed, reg: %d, val: %d, error: %d\n",
			__func__, reg, val, retval);
		return retval;
	}

	return 0;
}
#if 0
static int gpio_xra1201p_read_reg(struct xra1201p_chip_data *chip, int reg, u16 *val)
{
	int retval;

	retval = i2c_smbus_read_byte_data(chip->client, reg);
	if (retval < 0) {
		dev_err(&chip->client->dev, "%s failed, reg: %d, error: %d\n",
			__func__, reg, retval);
		return retval;
	}

	*val = (u16)retval;
	return 0;
}
#endif

static int xra1201p_gpio_direction_output(struct gpio_chip *gc, unsigned off, int val);
static int xra1201p_gpio_direction_input(struct gpio_chip *gc, unsigned off);

static int xra1201p_kpd_set_registers(struct xra1201p_chip_data *chip)
{
	int ret = 1;
	unsigned int i = 0;
#if 0
	mutex_lock(&chip->i2c_lock);
	ret = gpio_xra1201p_write_reg(chip, XRA1201P_REG_6, XRA1201P_REG_6_VALUE);//col output(0~4)
	if (ret)
		goto exit;
	ret = gpio_xra1201p_write_reg(chip, XRA1201P_REG_7, XRA1201P_REG_7_VALUE);//row input(0~4)
	if (ret)
		goto exit;

	ret = gpio_xra1201p_write_reg(chip, XRA1201P_REG_2, XRA1201P_REG_2_VALUE);
	if (ret)
		goto exit;

	ret = gpio_xra1201p_write_reg(chip, XRA1201P_REG_3, XRA1201P_REG_3_VALUE);
	if (ret)
		goto exit;
#endif
	for (; i < 5; i++)
	{
		xra1201p_debug("i =%u\n", i);
		xra1201p_gpio_direction_output(&chip->gpio_chip, i, 1);
	}
	for (i = 8; i < 13; i++)
	{
		xra1201p_gpio_direction_input(&chip->gpio_chip,i);
	}
	ret = gpio_xra1201p_write_reg(chip, 0x09, 0xE0);//disable internal pull-up resistors
	if (ret)
		goto exit;
	ret = gpio_xra1201p_write_reg(chip, 0x0B, 0xFF);// enable interrupt
	if (ret)
		goto exit;
	ret = 0;
exit:
//	mutex_unlock(&chip->i2c_lock);
	return ret;
}

#ifdef __XRA1201P_WRITE_REG_DEBUG__
static ssize_t call_xra1201p_write_reg(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t size)
{
     int reg = 0;
	 int value = 0;

     sscanf(buf, "%d,%x", &reg, &value);
	 xra1201p_debug("xra1201p reg = %d, value = %d \n", reg, value);
	 if((reg <= 7)&&(control_chip_external != NULL))
	    gpio_xra1201p_write_reg(control_chip_external, reg, value);
	 return size;
}
static DEVICE_ATTR(write_xra1201p_reg, 0222, NULL, call_xra1201p_write_reg);
#endif

#if 0
int gpio_xra1201p_set_value(char gpio, char value)
{
  	int ret = 0;
	int port_line;
	char bit;
	char set_value;
	return 0;
	if(gpio < 0 || ((gpio - 10) > 8))
		return -1;

	if(gpio < 8){
		port_line = XRA1201P_REG_2;
		bit = gpio;
		set_value = rember_reg2_value;
	}else{
		port_line = XRA1201P_REG_3;
		bit = gpio - 10;
		set_value = rember_reg3_value;
	}
	xra1201p_debug("port_line = %d, bit = %d\n", port_line, bit);

	CLEARBIT(set_value, bit);
	SETBIT(set_value, bit, value);
  	xra1201p_debug("set_value = %d\n", set_value);
  	if(control_chip_external != NULL){
		ret = gpio_xra1201p_write_reg(control_chip_external, port_line, set_value);
		if(ret < 0)
		 	xra1201p_debug("ret = %d\n", ret);
		else{
			if(gpio < 8)
				rember_reg2_value = set_value;
			else
				rember_reg3_value = set_value;
		}
  	}

	return ret;
}
EXPORT_SYMBOL(gpio_xra1201p_set_value);

int gpio_xra1201p_get_value(char gpio)
{
  	int ret = 0;
	int port_line;
	char bit;
	u16 value;
return 0;

	if(gpio < 0 || ((gpio - 10) > 8))
		return -1;

	port_line = (gpio < 8) ? XRA1201P_REG_2 : XRA1201P_REG_3;
	bit = (gpio < 8) ? gpio : (gpio - 10);

  	if(control_chip_external != NULL){
		ret = gpio_xra1201p_read_reg(control_chip_external, port_line, &value);
		if(ret < 0)
		 	xra1201p_debug("%s ret = %d\n", __func__, ret);
		else
			return GETBIT(value, bit);
  	}

  	return -1;
}
EXPORT_SYMBOL(gpio_xra1201p_get_value);
#endif

static int xra1201p_write_reg(struct xra1201p_chip_data *chip, int reg, u16 val)
{
	int error;

	error = chip->io_size > 8 ?
		i2c_smbus_write_word_data(chip->client, reg << 1, val) :
		i2c_smbus_write_byte_data(chip->client, reg, val);
	if (error < 0) {
		dev_err(&chip->client->dev,
			"%s failed, reg: %d, val: %d, error: %d\n",
			__func__, reg, val, error);
		return error;
	}

	return 0;
}


static int xra1201p_read_reg(struct xra1201p_chip_data *chip, int reg, u16 *val)
{
	int retval;

	retval = chip->io_size > 8 ?
		 i2c_smbus_read_word_data(chip->client, reg << 1) :
		 i2c_smbus_read_byte_data(chip->client, reg);
	if (retval < 0) {
		dev_err(&chip->client->dev, "%s failed, reg: %d, error: %d\n",
			__func__, reg, retval);
		return retval;
	}

	*val = (u16)retval;
	return 0;
}

static int xra1201p_read_single(struct xra1201p_chip_data *chip, int reg, u32 *val,
				int off)
{
	int ret;
	int bank_shift = fls((chip->gpio_chip.ngpio - 1) / BANK_SZ);
	int offset = off / BANK_SZ;

	ret = i2c_smbus_read_byte_data(chip->client,
				(reg << bank_shift) + offset);
	*val = ret;
	if (ret < 0) {
		dev_err(&chip->client->dev, "failed reading register\n");
		return ret;
	}

	return 0;
}

static int xra1201p_write_single(struct xra1201p_chip_data *chip, int reg, u32 val,
				int off)
{
	int ret = 0;
	int bank_shift = fls((chip->gpio_chip.ngpio - 1) / BANK_SZ);
	int offset = off / BANK_SZ;
	ret = i2c_smbus_write_byte_data(chip->client,
					(reg << bank_shift) + offset, val);

	if (ret < 0) {
		dev_err(&chip->client->dev, "failed writing register\n");
		return ret;
	}
	xra1201p_debug("reg:%x, val:%x\n", (reg << bank_shift)+offset, val);

	return 0;
}

static int xra1201p_gpio_direction_input(struct gpio_chip *gc, unsigned off)
{
	struct xra1201p_chip_data *chip;
	u8 reg_val;
	int ret, offset = 0;

	chip = container_of(gc, struct xra1201p_chip_data, gpio_chip);
	mutex_lock(&chip->i2c_lock);
	reg_val = chip->reg_direction[off / BANK_SZ] | (1u << (off % BANK_SZ));

	offset = 3;
	ret = xra1201p_write_single(chip, offset, reg_val, off);
	if (ret)
		goto exit;

	chip->reg_direction[off / BANK_SZ] = reg_val;
	ret = 0;
exit:
	mutex_unlock(&chip->i2c_lock);
	return ret;
}

static int xra1201p_gpio_direction_output(struct gpio_chip *gc,
		unsigned off, int val)
{
	struct xra1201p_chip_data *chip;
	u8 reg_val;
	int ret, offset = 0;
	chip = container_of(gc, struct xra1201p_chip_data, gpio_chip);

	mutex_lock(&chip->i2c_lock);
	/* set output level */
	if (val)
		reg_val = chip->reg_output[off / BANK_SZ]
			| (1u << (off % BANK_SZ));
	else
		reg_val = chip->reg_output[off / BANK_SZ]
			& ~(1u << (off % BANK_SZ));

	offset = 1;
	ret = xra1201p_write_single(chip, offset, reg_val, off);
	if (ret)
		goto exit;

	chip->reg_output[off / BANK_SZ] = reg_val;

	/* then direction */
	reg_val = chip->reg_direction[off / BANK_SZ] & ~(1u << (off % BANK_SZ));
	offset = 3;
	ret = xra1201p_write_single(chip, offset, reg_val, off);
	if (ret)
		goto exit;

	chip->reg_direction[off / BANK_SZ] = reg_val;
	ret = 0;
exit:
	mutex_unlock(&chip->i2c_lock);
	return ret;
}

static int xra1201p_gpio_get_value(struct gpio_chip *gc, unsigned off)
{
	struct xra1201p_chip_data *chip;
	u32 reg_val;
	int ret, offset = 0;
	chip = container_of(gc, struct xra1201p_chip_data, gpio_chip);

	mutex_lock(&chip->i2c_lock);
	offset = 0;
	ret = xra1201p_read_single(chip, offset, &reg_val, off);
	mutex_unlock(&chip->i2c_lock);
	if (ret < 0) {
		/* NOTE:  diagnostic already emitted; that's all we should
		 * do unless gpio_*_value_cansleep() calls become different
		 * from their nonsleeping siblings (and report faults).
		 */
		return 0;
	}

	return (reg_val & (1u << (off % BANK_SZ ))) ? 1 : 0;
}

static void xra1201p_gpio_set_value(struct gpio_chip *gc, unsigned off, int val)
{
	struct xra1201p_chip_data *chip;
	u8 reg_val;
	int ret, offset = 0;
	chip = container_of(gc, struct xra1201p_chip_data, gpio_chip);

	mutex_lock(&chip->i2c_lock);
	if (val)
		reg_val = chip->reg_output[off / BANK_SZ]
			| (1u << (off % BANK_SZ));
	else
		reg_val = chip->reg_output[off / BANK_SZ]
			& ~(1u << (off % BANK_SZ));

	offset = 1;

	ret = xra1201p_write_single(chip, offset, reg_val, off);
	if (ret)
		goto exit;

	chip->reg_output[off / BANK_SZ] = reg_val;
exit:
	mutex_unlock(&chip->i2c_lock);
}


static void xra1201p_setup_gpio(struct xra1201p_chip_data *chip, int gpios)
{
	struct gpio_chip *gc;
	gc = &chip->gpio_chip;

	gc->direction_input  = xra1201p_gpio_direction_input;
	gc->direction_output = xra1201p_gpio_direction_output;
	gc->get = xra1201p_gpio_get_value;
	gc->set = xra1201p_gpio_set_value;
	gc->can_sleep = 1;

	//gc->base = chip->gpio_start;
	gc->ngpio = gpios;
	gc->label = chip->client->name;
	gc->dev = &chip->client->dev;
	gc->owner = THIS_MODULE;
	gc->names = chip->names;
}

static void xra1201p_kpd_select_button(struct xra1201p_chip_data *chip, u16 reg_val, int pin_index)
{
	switch(reg_val){//row
	case XRA1201P_KEY_RELEASE:// 0x00
		if(chip->pressed){
			input_event(chip->input, EV_KEY, chip->active_key, 0);
			input_sync(chip->input);
			xra1201p_debug(">>>>>>>>>>>>>>xra1201p now release button = %d\n",chip->active_key);
			chip->pressed = 0;
			chip->active_key = 0;
		}
		break;
	case XRA1201P_KEY_ROW_0:// 0x01
		switch(pin_index){
			case XRA1201P_KEY_COL_0:// 0x00
				xra1201p_debug("xra1201p  K1 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[0].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[0].code;
				break;
			case XRA1201P_KEY_COL_1: // 0x01
				xra1201p_debug("xra1201p K6 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[5].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[5].code;
				break;
			case XRA1201P_KEY_COL_2:// 0x02
				xra1201p_debug("xra1201p K11 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[10].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[10].code;
				break;
			case XRA1201P_KEY_COL_3:// 0x02
				xra1201p_debug("xra1201p K16 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[15].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[15].code;
				break;
			case XRA1201P_KEY_COL_4:// 0x02
				xra1201p_debug("xra1201p K21 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[20].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[20].code;
				break;

		}
		break;
	case XRA1201P_KEY_ROW_1:// 0x02
		switch(pin_index){
			case XRA1201P_KEY_COL_0:
				xra1201p_debug("xra1201p K2 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[1].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[1].code;
				break;
			case XRA1201P_KEY_COL_1:
				xra1201p_debug("xra1201p K7 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[6].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[6].code;
				break;
			case XRA1201P_KEY_COL_2:// 0x02
				xra1201p_debug("xra1201p K12 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[11].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[11].code;
				break;
			case XRA1201P_KEY_COL_3:// 0x02
				xra1201p_debug("xra1201p K17 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[16].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[16].code;
				break;
			case XRA1201P_KEY_COL_4:// 0x02
				xra1201p_debug("xra1201p K22 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[21].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[21].code;
				break;
		}
		break;
	case XRA1201P_KEY_ROW_2:// 0x04
		switch(pin_index){
			case XRA1201P_KEY_COL_0:
				xra1201p_debug("xra1201p K3 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[2].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[2].code;
				break;
			case XRA1201P_KEY_COL_1:
				xra1201p_debug("xra1201p K8 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[7].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[7].code;
				break;
			case XRA1201P_KEY_COL_2:
				xra1201p_debug("xra1201p K13 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[12].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[12].code;
				break;
			case XRA1201P_KEY_COL_3:// 0x02
				xra1201p_debug("xra1201p K18 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[17].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[17].code;
				break;
			case XRA1201P_KEY_COL_4:// 0x02
				xra1201p_debug("xra1201p K23 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[22].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[22].code;
				break;
		}
		break;
	case XRA1201P_KEY_ROW_3:// 0x08
		switch(pin_index){
			case XRA1201P_KEY_COL_0:
				xra1201p_debug("xra1201p K4 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[3].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[3].code;
				break;
			case XRA1201P_KEY_COL_1:
				xra1201p_debug("xra1201p K9 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[8].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[8].code;
				break;
			case XRA1201P_KEY_COL_2:// 0x02
				xra1201p_debug("xra1201p K14 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[13].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[13].code;
				break;
			case XRA1201P_KEY_COL_3:// 0x02
				xra1201p_debug("xra1201p K19 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[18].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[18].code;
				break;
			case XRA1201P_KEY_COL_4:// 0x02
				xra1201p_debug("xra1201p K24 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[23].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[23].code;
				break;
		}
		break;
	case XRA1201P_KEY_ROW_4://0x10
		switch(pin_index){
			case XRA1201P_KEY_COL_0:
				xra1201p_debug("xra1201p K5 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[4].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[4].code;
				break;
			case XRA1201P_KEY_COL_1:
				xra1201p_debug("xra1201p K10 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[9].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[9].code;
				break;
			case XRA1201P_KEY_COL_2:
				xra1201p_debug("xra1201p K15 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[14].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[14].code;
				break;
			case XRA1201P_KEY_COL_3:// 0x02
				xra1201p_debug("xra1201p K21 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[19].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[19].code;
				break;
			case XRA1201P_KEY_COL_4:// 0x02
				xra1201p_debug("xra1201p K25 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[24].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[24].code;
				break;

		}
		break;
#if 0
	case XRA1201P_KEY_ROW_5:
		switch(pin_index){
			case XRA1201P_KEY_COL_0:
				xra1201p_debug("xra1201p K6 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[5].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[5].code;
				break;
			case XRA1201P_KEY_COL_1:
				xra1201p_debug("xra1201p K14 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[13].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[13].code;
				break;
		}
		break;
	case XRA1201P_KEY_ROW_6:
		switch(pin_index){
			case XRA1201P_KEY_COL_0:
				xra1201p_debug("xra1201p K7 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[6].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[6].code;
				break;
			case XRA1201P_KEY_COL_1:
				xra1201p_debug("xra1201p K15 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[14].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[14].code;
				break;
			case XRA1201P_KEY_COL_2:
				xra1201p_debug("xra1201p K17 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[16].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[16].code;
				break;

		}
		break;
	case XRA1201P_KEY_ROW_7:
		switch(pin_index){
			case XRA1201P_KEY_COL_0:
				xra1201p_debug("xra1201p K8 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[7].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[7].code;
				break;
			case XRA1201P_KEY_COL_1:
				xra1201p_debug("xra1201p K16 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[15].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[15].code;
				break;
			case XRA1201P_KEY_COL_2:
				xra1201p_debug("xra1201p K18 pressed!!\n");
				input_event(chip->input, EV_KEY, chip->buttons[17].code, 1);
				input_sync(chip->input);
				chip->active_key = chip->buttons[17].code;
				break;

		}
		break;
#endif
	}
}


static void xra1201p_kpd_scan(struct xra1201p_chip_data *chip)
{
	u16 reg_val = 0;
	u16 reg_val2 = 0;
	u16 write_val = 0x01;
	int i,pin_index = 0;

	/*read which row is change to high*/
	xra1201p_read_reg(chip, XRA1201P_REG_1, &reg_val);
	xra1201p_debug("\n");

	if((reg_val & 0x1F) == 0) // For the value of rows that use int keypad.
		xra1201p_kpd_select_button(chip, 0, 0);

	if(chip->pinmask == reg_val || chip->pressed == 1)
		return;

	chip->pinmask = reg_val;
	xra1201p_read_reg(chip, XRA1201P_REG_2, &reg_val2);
	reg_val2 &= 0xE0; //For not change the state of the other gpio that not used in keypad.
	for(i = 0; i < 5; i++){
		xra1201p_write_reg(chip, XRA1201P_REG_2, (reg_val2 | (write_val << (i))));
		xra1201p_read_reg(chip, XRA1201P_REG_1, &reg_val);
	    	xra1201p_debug("xra1201p_keys_scan row(%d) = %x\n", i, reg_val);
		if((reg_val & 0x1F)) {
			chip->pressed = 1;
			pin_index = i;
			break;
		}
	}
	xra1201p_kpd_select_button(chip, (reg_val & 0x1F), pin_index);
#ifdef KEYBOARD_BL_CTRL
	if(brightness_value)
		xra1201p_write_reg(chip, XRA1201P_REG_2, XRA1201P_REG_2_VALUE|KEY_BL_BIT);
	else
#endif
	mutex_lock(&chip->i2c_lock);
	xra1201p_read_reg(chip, XRA1201P_REG_2, &reg_val2);
	xra1201p_write_reg(chip, XRA1201P_REG_2, (reg_val2 | XRA1201P_REG_2_VALUE));
	mutex_unlock(&chip->i2c_lock);
}

static int xra1201p_keys_open(struct input_dev *dev)
{
	struct xra1201p_chip_data *chip = input_get_drvdata(dev);

	xra1201p_debug("\n");
	/* Get initial device state in case it has switches */
	xra1201p_kpd_scan(chip);
	enable_irq(chip->irq);

	return 0;
}
static void xra1201p_keys_close(struct input_dev *dev)
{
	struct xra1201p_chip_data *chip = input_get_drvdata(dev);
	xra1201p_debug("\n");
	disable_irq(chip->irq);
}

static void xra1201p_irq_work(struct work_struct *work){
	struct xra1201p_chip_data *chip =
		container_of(work, struct xra1201p_chip_data, xra1201p_work);
	xra1201p_debug("\n");
	xra1201p_kpd_scan(chip);
}

static irqreturn_t xra1201p_interrupt_handler(int irq, void *dev_id)
{
	struct xra1201p_chip_data *chip = dev_id;
	xra1201p_debug("\n");
	queue_work(chip->xra1201p_wq, &chip->xra1201p_work);
	return IRQ_HANDLED;
}

static void xra1201p_kpd_parse_dt(struct device *dev, struct xra1201p_pdata *pdata)
{
	struct device_node *np = dev->of_node;
	if (np == NULL)
		return;

	pdata->irq_gpio = of_get_named_gpio_flags(np, "xra1201p,interrupt-gpio",0, NULL);
	xra1201p_debug("irq_gpio:%d\n",pdata->irq_gpio);
}

#ifdef KEYBOARD_BL_CTRL
static void xra1201p_button_backlight(struct led_classdev *led_cdev, enum led_brightness brightness)
{
	struct xra1201p_chip_data *chip = container_of(led_cdev, struct xra1201p_chip_data, cdev);
	brightness_value = brightness;
	if (brightness == 0) {
		xra1201p_write_reg(chip, XRA1201P_REG_2, XRA1201P_REG_2_VALUE);
		xra1201p_debug("xra1201p turn off button led\n");
	} else {
		xra1201p_write_reg(chip, XRA1201P_REG_2, XRA1201P_REG_2_VALUE|KEY_BL_BIT);
		xra1201p_debug("xra1201p turn on button led\n");
	}
}
#endif

static int  xra1201p_chip_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct xra1201p_pdata *pdata;
	struct xra1201p_chip_data *chip;
	struct input_dev *input;
	int ret,i;
	xra1201p_debug("====in====\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "%s: %s adapter not supported\n", __func__,dev_driver_string(&client->adapter->dev));
		return  -ENODEV;
	}

	if (client->dev.of_node) {
		pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
		 if (!pdata) {
			dev_err(&client->dev, "%s: Failed to allocate memory for pdata.\n", __func__);
			ret = -ENOMEM;
			return ret;
		}
#ifdef CONFIG_OF_GPIO
		xra1201p_kpd_parse_dt(&client->dev, pdata);
#endif
		if (!gpio_is_valid(pdata->irq_gpio)) {
			goto request_failed;
		}
	} else {
		pdata = client->dev.platform_data;
		if (!pdata) {
			dev_err(&client->dev, "%s no platform data.\n", __func__);
			ret = -EINVAL;
			return ret;
		}
	}

	pdata->buttons	= xra1201p_kpd_keys,
	pdata->nbuttons	= ARRAY_SIZE(xra1201p_kpd_keys),
	pdata->rep		= 0,
	pdata->pinmask	= 0x0000,

	chip = kzalloc(sizeof(struct xra1201p_chip_data)+
		   pdata->nbuttons * sizeof(struct xra1201p_button), GFP_KERNEL);
	if(!chip){
		dev_err(&client->dev, "%s: Failed to allocate memory for chip_data.\n", __func__);
		ret = -ENOMEM;
		goto request_failed;
	}



	input = input_allocate_device();
	if (!input) {
		ret = -ENOMEM;
		dev_err(&client->dev, "%s input_allocate_device failed.\n", __func__);
		goto free_chip_data;
	}

	chip->client = client;
	chip->platform_data = pdata;

	chip->vcc_i2c = regulator_get(&chip->client->dev, "vcc_i2c");

	if (IS_ERR(chip->vcc_i2c)) {
		ret = PTR_ERR(chip->vcc_i2c);
		dev_err(&chip->client->dev,	"Regulator get failed vcc_i2c ret=%d\n", ret);
		if (regulator_count_voltages(chip->vcc_i2c) > 0)
			regulator_set_voltage(chip->vcc_i2c, 0, 1800000);
	}

	if (regulator_count_voltages(chip->vcc_i2c) > 0) {
		ret = regulator_set_voltage(chip->vcc_i2c, 1800000,
					   1800000);
		if (ret) {
			dev_err(&chip->client->dev,	"Regulator set_vtg failed vcc_i2c ret=%d\n", ret);
			regulator_put(chip->vcc_i2c);
		}
	}

	ret = regulator_enable(chip->vcc_i2c);
	if (ret) {
		dev_err(&chip->client->dev,	"Regulator vcc_i2c enable failed ret=%d\n", ret);
	}

	xra1201p_setup_gpio(chip, 16);
	ret = gpiochip_add(&chip->gpio_chip);
	xra1201p_debug("gpio_base:%u\n", chip->gpio_chip.base);
	if (ret)
		goto free_chip_data;

	chip->input = input;
	//chip->io_size = id->driver_data; //can't boot here
	chip->pinmask = pdata->pinmask;
	chip->active_key = 0;
	chip->pressed = 0;

	//input->phys = "xra1201p-keys/input4";
	input->name = client->name;
	input->dev.parent = &client->dev;
	input->open = xra1201p_keys_open;
	input->close = xra1201p_keys_close;
	//input->id.bustype = BUS_HOST;
	//input->id.vendor = 0x0001;
	//input->id.product = 0x0001;
	//input->id.version = 0x0100;

	/* Enable auto repeat feature of Linux input subsystem */
	if (pdata->rep)
			__set_bit(EV_REP, input->evbit);

	for (i = 0; i < pdata->nbuttons; i++) {
		unsigned int type;
		chip->buttons[i] = pdata->buttons[i];
		type = (pdata->buttons[i].type) ?: EV_KEY;
		input_set_capability(input, type, pdata->buttons[i].code);
	}

	i2c_set_clientdata(client, chip);
	input_set_drvdata(input, chip);
	ret = input_register_device(input);
	if (ret) {
		dev_err(&client->dev,"Unable to register input device\n");
		goto free_input;
	}

	mutex_init(&chip->i2c_lock);

	INIT_WORK(&chip->xra1201p_work, xra1201p_irq_work);
	chip->xra1201p_wq = create_singlethread_workqueue("xra1201p");
	if (!chip->xra1201p_wq) {
		dev_err(&client->dev,"xra1201p error]%s: can't create workqueue\n", __func__);
		ret = -ENOMEM;
		goto freeall;
	}

	ret = xra1201p_kpd_set_registers(chip);
	xra1201p_debug("\n");

	if (ret){
		dev_err(&client->dev, "%s: setup registers failed ret = %d\n", __func__, ret);
		goto freeall;
	}

	chip->irq_pin = pdata->irq_gpio;
	chip->irq = gpio_to_irq(pdata->irq_gpio);
	ret = request_threaded_irq(chip->irq, NULL, xra1201p_interrupt_handler,
			     IRQF_TRIGGER_FALLING | IRQF_ONESHOT, "xra1201p-irq", chip);
  	if (ret) {
		dev_err(&client->dev,"tca1112 request irq error]%s: req_irq(%d) fail for gpio %d (%d)\n",
			__func__, chip->irq, chip->irq_pin, ret);
		goto freeall;

	}

	//device_init_wakeup(&client->dev, 1);
 	//irq_set_irq_wake(chip->irq, 1);

	control_chip_external = chip;

#ifdef KEYBOARD_BL_CTRL
	chip->cdev.name = "button-backlight";
	chip->cdev.brightness_set = xra1201p_button_backlight;
	chip->cdev.brightness = LED_OFF;
	chip->cdev.max_brightness = 128;
	chip->cdev.flags |= LED_CORE_SUSPENDRESUME;
	led_classdev_register(&client->dev, &chip->cdev);
#endif

#ifdef __XRA1201P_WRITE_REG_DEBUG__
	ret = device_create_file(&client->dev, &dev_attr_write_xra1201p_reg);
	if(ret) {
		xra1201p_debug("creat sys file failed.\n");
	}
#endif
	xra1201p_debug("===OK====\n");

	return 0;

freeall:
	destroy_workqueue(chip->xra1201p_wq);
free_input:
	input_free_device(input);
free_chip_data:
	kfree(chip);
request_failed:
	kfree(pdata);
	return ret;
}

static int xra1201p_chip_remove(struct i2c_client *client)
{
	struct xra1201p_chip_data *chip = i2c_get_clientdata(client);
	int ret = 0;

	ret = regulator_disable(chip->vcc_i2c);
	if (ret) {
		dev_err(&chip->client->dev,	"Regulator vcc_i2c disable failed ret=%d\n", ret);
		return ret;
	}

	ret = gpiochip_remove(&chip->gpio_chip);
	if (ret) {
		dev_err(&client->dev, "%s failed, %d\n",
				"gpiochip_remove()", ret);
		return ret;
	}

#ifdef __XRA1201P_WRITE_REG_DEBUG__
	device_remove_file(&client->dev, &dev_attr_write_xra1201p_reg);
#endif
	mutex_destroy(&chip->i2c_lock);
	kfree(chip);

	return 0;
}

static const struct i2c_device_id xra1201p_chip_id[] = {
	{ "xra1201p-kpd", 16 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, xra1201p_chip_id);

#ifdef CONFIG_OF
static const struct of_device_id xra1201p_match[] = {
	{.compatible = "exar,xra1201p-kpd"},
	{}
};
#else
#define xra1201p_match = NULL;
#endif

#ifdef CONFIG_PM_SLEEP
static int xra1201p_pm_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct xra1201p_chip_data *chip_data = i2c_get_clientdata(client);

	xra1201p_debug("==xra1201p_pm_suspend==\n");

	xra1201p_keys_close(chip_data->input);

	return 0;
}

static int xra1201p_pm_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct xra1201p_chip_data *chip_data = i2c_get_clientdata(client);

	xra1201p_debug("==xra1201p_pm_resume==\n");

	xra1201p_kpd_set_registers(chip_data);
	xra1201p_keys_open(chip_data->input);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(xra1201p_pm_ops, xra1201p_pm_suspend, xra1201p_pm_resume);

static struct i2c_driver xra1201p_kpd_driver = {
	.driver = {
		.name	= "xra1201p-kpd",
		.owner	= THIS_MODULE,
		.pm		= &xra1201p_pm_ops,
		.of_match_table = xra1201p_match,
	},
	.probe		= xra1201p_chip_probe,
	.remove		= xra1201p_chip_remove,
	.id_table		= xra1201p_chip_id,
};

static int __init xra1201p_chip_init(void)
{
	//xra1201p_debug("\n");

	return i2c_add_driver(&xra1201p_kpd_driver);
}

static void __exit xra1201p_chip_exit(void)
{
	xra1201p_debug("\n");
	i2c_del_driver(&xra1201p_kpd_driver);
}
module_init(xra1201p_chip_init);
module_exit(xra1201p_chip_exit);

MODULE_AUTHOR(" SIM xipeng.gu@sim.com>");
MODULE_DESCRIPTION("xra1201p IO expander as GPIO-EXT");
MODULE_LICENSE("GPL v2");
