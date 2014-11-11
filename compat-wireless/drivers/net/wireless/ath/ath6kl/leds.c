/*
 * leds.c
 *
 * Copyright (C) 2013 by Digi International Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * Description: LED handling for ATH6KL driver
 *
 */

#include "core.h"
#include "leds.h"
#include <linux/gpio.h>

#define ACTIVITY_LED_TOGGLE_TIMEOUT	2	/* in jiffies */
#define ACTIVITY_LED_OFF_TIMEOUT	20	/* in jiffies */

/* active = 1: VIF is connected. Turn on activity LED, if it was off.
 * active = 0: VIF is disconnected. Turn off activity LED, if all VIFs are off.
 */
void leds_set_connection_state(struct ath6kl_vif *vif, int active)
{
	struct ath6kl *ar = vif->ar;
	unsigned int prev_state, curr_state;

	/* Activity LED support is disabled */
	if (ar->activity_led == -1)
		return;

	if (vif->fw_vif_idx >= ATH6KL_VIF_MAX)
		return;

	/* connection_state keeps track of the virtual interfaces' connection
	 * state. I need only a true / false state.
	 */
	prev_state = ar->connection_state ? 1 : 0;

	if (active)
		ar->connection_state |= (1 << vif->fw_vif_idx);
	else
		ar->connection_state &= ~(1 << vif->fw_vif_idx);

	/* Should change LED state only if all VIFs are disconnected, or
	 * at least on is connected
	 */
	curr_state = ar->connection_state ? 1 : 0;

	if (curr_state != prev_state)
		gpio_set_value(ar->activity_led, curr_state ? ar->leds_active :
						!ar->leds_active);
}

/* active = 1: if no VIF is connected, then turn on scanning LED
 * active = 0: if all VIFs are disconnected, and all scannings are off,
 *             then turn off scanning LED
 */
void leds_set_scanning_state(struct ath6kl_vif *vif, int active)
{
	struct ath6kl *ar = vif->ar;
	unsigned int prev_state;

	/* Scanning LED support is disabled */
	if (ar->scanning_led == -1)
		return;

	if (vif->fw_vif_idx >= ATH6KL_VIF_MAX)
		return;

	/* scanning_state keeps track of the virtual interfaces' scanning
	 * state. I need only a true / false state.
	 */
	prev_state = ar->scanning_state ? 1 : 0;

	if (active)
		ar->scanning_state |= (1 << vif->fw_vif_idx);
	else
		ar->scanning_state &= ~(1 << vif->fw_vif_idx);

	/* If connected, always disable scanning LED */
	if (ar->connection_state)
		gpio_set_value(ar->scanning_led, !ar->leds_active);
	else {
		unsigned int curr_state;

		curr_state = ar->scanning_state ? 1 : 0;
		/* If state was changed, set LED */
		if (curr_state != prev_state)
			gpio_set_value(ar->scanning_led, curr_state ? ar->leds_active :
							!ar->leds_active);
	}
}

void leds_toggle_activity_led(struct ath6kl *ar)
{
	/* Activity LED support is disabled */
	if (ar->activity_led == -1)
		return;

	/* If not connected, don't show activity */
	if (!ar->connection_state)
		return;

	ar->rxtx_activity = 1;
	/* run timer if not already running */
	if (!timer_pending(&ar->activity_led_timer))
		mod_timer(&ar->activity_led_timer,
			  jiffies + ACTIVITY_LED_TOGGLE_TIMEOUT);
}

static void leds_activity_led_timer_fn(unsigned long dev)
{
	struct ath6kl *ar = (struct ath6kl *)dev;

	if (ar->rxtx_activity) {
		/* toggle RX/TX Ethernet activity LED */
		gpio_set_value(ar->activity_led,
			       !gpio_get_value(ar->activity_led));
		mod_timer(&ar->activity_led_timer,
			  jiffies + ACTIVITY_LED_TOGGLE_TIMEOUT);
		ar->rxtx_cnt = 0;
		ar->rxtx_activity = 0;
	} else {
		if (ar->rxtx_cnt++ < ACTIVITY_LED_OFF_TIMEOUT / ACTIVITY_LED_TOGGLE_TIMEOUT)
			mod_timer(&ar->activity_led_timer,
				  jiffies + ACTIVITY_LED_TOGGLE_TIMEOUT);
		else {
			gpio_set_value(ar->activity_led,
				       ar->connection_state ?
				       ar->leds_active :
				       !ar->leds_active);
			ar->rxtx_cnt = 0;
		}
	}
}

int leds_init(struct ath6kl *ar, unsigned int activity_led_gpio,
	      unsigned int scanning_led_gpio, unsigned int leds_active_high)
{
	int ret;

	/* If any of the LED GPIOs is not defined, disable LED support */
	if ((activity_led_gpio == -1) || (scanning_led_gpio == -1)) {
		ar->activity_led = -1;
		ar->scanning_led = -1;

		return 0;
	}

	ret = gpio_request(activity_led_gpio, "wlan_activity_led");
	if (ret) {
		printk(KERN_ERR "Couldn't request GPIO for Activity LED\n");
		goto err;
	}

	ret = gpio_request(scanning_led_gpio, "wlan_scanning_led");
	if (ret) {
		printk(KERN_ERR "Couldn't request GPIO for Scanning LED\n");
		goto err_activity;
	}

	/* Turn OFF LEDs */
	gpio_direction_output(activity_led_gpio, !leds_active_high);
	gpio_direction_output(scanning_led_gpio, !leds_active_high);

	ar->scanning_state = 0;
	ar->connection_state = 0;
	ar->activity_led = activity_led_gpio;
	ar->scanning_led = scanning_led_gpio;
	ar->leds_active = leds_active_high;
	ar->rxtx_cnt = 0;
	ar->activity_led_timer.data = (unsigned long)ar;
	ar->activity_led_timer.function = leds_activity_led_timer_fn;
	ar->activity_led_timer.expires = jiffies + ACTIVITY_LED_TOGGLE_TIMEOUT;
	init_timer(&ar->activity_led_timer);
	add_timer(&ar->activity_led_timer);

	return 0;

err_activity:
	gpio_free(activity_led_gpio);
	ar->activity_led = -1;
	ar->scanning_led = -1;
err:
	return ret;
}

void leds_deinit(struct ath6kl *ar)
{
	if ((ar->activity_led == -1) || (ar->scanning_led == -1))
		return;

	del_timer(&ar->activity_led_timer);

	/* Disable LEDs, and free them */
	gpio_set_value(ar->activity_led, ar->leds_active);
	gpio_free(ar->activity_led);
	ar->activity_led = -1;

	gpio_set_value(ar->scanning_led, ar->leds_active);
	gpio_free(ar->scanning_led);
	ar->scanning_led = -1;
}
