ATHEROS_BASEDIR := compat-wireless

ifneq ($(KERNELRELEASE),)

ATH_DEFINES += \
	-DCOMPAT_BASE_TREE="\"$(shell cat $(src)/$(ATHEROS_BASEDIR)/compat_base_tree)\"" \
	-DCOMPAT_BASE_TREE_VERSION="\"$(shell cat $(src)/$(ATHEROS_BASEDIR)/compat_base_tree_version)\"" \
	-DCOMPAT_PROJECT="\"Compat-wireless\"" \
	-DCOMPAT_VERSION="\"$(shell cat $(src)/$(ATHEROS_BASEDIR)/compat_version)\""

NOSTDINC_FLAGS := -I$(M)/$(ATHEROS_BASEDIR)/include/ \
	-include $(M)/$(ATHEROS_BASEDIR)/include/linux/compat-2.6.h \
	$(ATH_DEFINES)

KLIB_BUILD ?= $(DEL_PROJ_DIR)/build/kernel

include $(src)/$(ATHEROS_BASEDIR)/config.mk

SHELL_EXPORT := PATH=$(src)/$(ATHEROS_BASEDIR)/scripts:$${PATH} \
		COMPAT_CONFIG=$(src)/$(ATHEROS_BASEDIR)/config.mk \
		CONFIG_CHECK=.$(COMPAT_CONFIG)_md5sum.txt \
		COMPAT_AUTOCONF=$(src)/$(ATHEROS_BASEDIR)/include/linux/compat_autoconf.h

dummy := $(shell $(SHELL_EXPORT) bash -c "cd $(src)/$(ATHEROS_BASEDIR) && ./scripts/check_config.sh || true")

obj-y := $(ATHEROS_BASEDIR)/compat/
obj-$(CONFIG_COMPAT_WIRELESS) += $(ATHEROS_BASEDIR)/net/wireless/
obj-$(CONFIG_COMPAT_WIRELESS_MODULES) += $(ATHEROS_BASEDIR)/drivers/net/wireless/ath/ath6kl/

else #ifneq ($(KERNELRELEASE),)

###############################################################################
##-DEL: install firmware
all: install

SHELL     = /bin/bash
FIRMWARE := $(addprefix fw_tablet_dongle/, athtcmd_ram.bin athwlan.bin fw-4.bin nullTestFlow.bin utf.bin)
FIRMWARE += Digi_6203-6233-US.bin Digi_6203-6233-World.bin
ifeq ("$(DEL_PLATFORM)","cpx2")
FIRMWARE += calData_AR6103_Digi_X2e_B.bin calData_AR6103_Digi_X2e_B_world.bin
endif

ATHEROS_SRC = $(shell pwd)

.PHONY: install clean distclean

# We need to fix the installation directory of the modules, as the previous 'modules_install'
# kernel rule copied them to <rootfs>/lib/modules/$(ATHEROS_BASEDIR)/<module-relative-path>
install: KRELEASE = $(shell $(MAKE) -C $(DEL_KERN_DIR) O=$(DEL_PROJ_DIR)/build/kernel -s kernelrelease)
install:
	@find $(INSTALL_MOD_PATH)/lib/modules/$(KRELEASE)/extra/$(ATHEROS_BASEDIR) -type f -name '*.ko' | \
		xargs -I modfile mv -f modfile $(INSTALL_MOD_PATH)/lib/modules/$(KRELEASE)/extra/
	@rm -rf $(INSTALL_MOD_PATH)/lib/modules/*/extra/$(ATHEROS_BASEDIR)
	@depmod -a -b $(INSTALL_MOD_PATH) $(KRELEASE)
	@rm -rf $(INSTALL_MOD_PATH)/lib/firmware/ath6k/AR6003/hw2.1.1 && \
		mkdir -p $(INSTALL_MOD_PATH)/lib/firmware/ath6k/AR6003/hw2.1.1
	@$(foreach fw, $(addprefix Firmware_Package/target/AR6003/hw2.1.1/, $(FIRMWARE)), \
		install -m 0644 $(fw) $(INSTALL_MOD_PATH)/lib/firmware/ath6k/AR6003/hw2.1.1/;)

clean:
	@rm -f $(shell find $(ATHEROS_SRC) -name '*.o' -o -name '*.o.cmd' -o -name '*.ko.cmd')
	@rm -f $(shell find $(ATHEROS_SRC) -name '*.mod.c' -o -name 'modules.order')
	@rm -rf $(ATHEROS_SRC)/{.tmp_versions,Module.symvers,$(ATHEROS_BASEDIR)/._md5sum.txt}

distclean: clean
	@rm -f $(shell find $(ATHEROS_SRC) -name '*.ko')

endif #ifneq ($(KERNELRELEASE),)
