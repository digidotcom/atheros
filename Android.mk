ifneq ($(TARGET_SIMULATOR),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

SHELL := /bin/bash

ATHEROS_SRC := $(realpath $(LOCAL_PATH))
ATHEROS_MOD := \
	compat-wireless/compat/compat.ko \
	compat-wireless/drivers/net/wireless/ath/ath6kl/ath6kl_sdio.ko \
	compat-wireless/net/wireless/cfg80211.ko
ATH_MOD_OBJ  := $(addprefix $(LOCAL_PATH)/,$(ATHEROS_MOD))

ifeq ($(TARGET_DEL_ATHEROS_WIFI),true)

$(ATH_MOD_OBJ): atheros_clean atheros

.PHONY: atheros atheros_clean
atheros: del_linux_image
	$(MAKE) -C kernel_imx KLIB_BUILD=$(ANDROID_BUILD_TOP)/kernel_imx $(KERNEL_ENV) M=$(ATHEROS_SRC)

endif

atheros_clean:
	@$(MAKE) -C $(ATHEROS_SRC) distclean

endif
