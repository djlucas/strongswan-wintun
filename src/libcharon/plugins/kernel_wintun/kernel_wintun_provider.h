/*
 * Copyright (C) 2026 DJ Lucas
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 */

/**
 * @defgroup kernel_wintun_provider kernel_wintun_provider
 * @{ @ingroup kernel_wintun
 */

#ifndef KERNEL_WINTUN_PROVIDER_H_
#define KERNEL_WINTUN_PROVIDER_H_

#include <networking/windows_tun_provider.h>

typedef struct kernel_wintun_provider_t kernel_wintun_provider_t;

/**
 * Windows TUN device provider implemented by the kernel-wintun plugin.
 */
struct kernel_wintun_provider_t {

	/**
	 * Implements the Windows TUN device provider interface.
	 */
	windows_tun_device_provider_t provider;

	/**
	 * Destroy this Windows TUN device provider.
	 */
	void (*destroy)(kernel_wintun_provider_t *this);
};

/**
 * Create a Windows Wintun TUN device provider.
 *
 * @return			Windows TUN device provider
 */
kernel_wintun_provider_t *kernel_wintun_provider_create(void);

#endif /** KERNEL_WINTUN_PROVIDER_H_ @}*/
