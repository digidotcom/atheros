/*
 * leds.h
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
#ifndef ATH6KL_LEDS_H
#define ATH6KL_LEDS_H

int leds_init(struct ath6kl *ar, unsigned int activity_led_gpio,
	      unsigned int scanning_led_gpio, unsigned int leds_active_high);
void leds_deinit(struct ath6kl *ar);
void leds_toggle_activity_led(struct ath6kl *ar);

/* active = 1: VIF is connected. Turn on activity LED, if it was off.
 * active = 0: VIF is disconnected. Turn off activity LED, if all VIFs are off.
 */
void leds_set_connection_state(struct ath6kl_vif *vif, int active);

/* active = 1: if no VIF is connected, then turn on scanning LED
 * active = 0: if all VIFs are disconnected, and all scannings are off,
 *             then turn off scanning LED
 */
void leds_set_scanning_state(struct ath6kl_vif *vif, int active);

#endif /* ATH6KL_LEDS_H */
