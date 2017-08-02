/*
 * TI pm33xx platform data
 *
 * Copyright (C) 2016 Texas Instruments, Inc.
 *
 * Dave Gerlach <d-gerlach@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _LINUX_PLATFORM_DATA_PM33XX_H
#define _LINUX_PLATFORM_DATA_PM33XX_H

/*
 * WFI Flags for sleep code control
 *
 * These flags allow PM code to exclude certain operations from happening
 * in the low level ASM code found in sleep33xx.S and sleep43xx.S
 *
 * WFI_FLAG_FLUSH_CACHE: Flush the ARM caches and disable caching. Only
 *			 needed when MPU will lose context.
 * WFI_FLAG_SELF_REFRESH: Let EMIF place DDR memory into self-refresh and
 *			  disable EMIF.
 * WFI_FLAG_SAVE_EMIF: Save context of all EMIF registers and restore in
 *		       resume path. Only needed if PER domain loses context
 *		       and must also have WFI_FLAG_SELF_REFRESH set.
 * WFI_FLAG_WAKE_M3: Disable MPU clock or clockdomain to cause wkup_m3 to
 *		     execute when WFI instruction executes.
 * WFI_FLAG_RTC_ONLY: Configure the RTC to enter RTC+DDR mode.
 */
#define WFI_FLAG_FLUSH_CACHE		BIT(0)
#define WFI_FLAG_SELF_REFRESH		BIT(1)
#define WFI_FLAG_SAVE_EMIF		BIT(2)
#define WFI_FLAG_WAKE_M3		BIT(3)
#define WFI_FLAG_RTC_ONLY		BIT(4)

#ifndef __ASSEMBLER__
struct am33xx_pm_sram_addr {
	void (*do_wfi)(void);
	unsigned long *do_wfi_sz;
	unsigned long *resume_offset;
	unsigned long *emif_sram_table;
	unsigned long *rtc_base_virt;
	phys_addr_t rtc_resume_phys_addr;
};

struct am33xx_pm_platform_data {
	int     (*init)(int (*idle)(u32 wfi_flags));
	int     (*deinit)(void);
	int	(*soc_suspend)(unsigned int state, int (*fn)(unsigned long),
			       unsigned long args);
	int	(*cpu_suspend)(int (*fn)(unsigned long), unsigned long args);
	struct  am33xx_pm_sram_addr *pm_sram_addr;
	void (*save_context)(void);
	void (*restore_context)(void);
	void (*prepare_rtc_suspend)(void);
	void (*prepare_rtc_resume)(void);
	int (*check_off_mode_enable)(void);
	void __iomem *(*get_rtc_base_addr)(void);
};
#endif /* __ASSEMBLER__ */
#endif /* _LINUX_PLATFORM_DATA_PM33XX_H */
