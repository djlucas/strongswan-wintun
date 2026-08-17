/*
 * Copyright (C) 2026 DJ Lucas
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 */

/**
 * @defgroup kernel_wintun_device kernel_wintun_device
 * @{ @ingroup kernel_wintun
 */

#ifndef KERNEL_WINTUN_DEVICE_H_
#define KERNEL_WINTUN_DEVICE_H_

#include "kernel_wintun_api.h"

#include <networking/tun_device.h>
#include <threading/mutex.h>

typedef struct kernel_wintun_context_t kernel_wintun_context_t;

/**
 * Provider state shared with every Wintun-backed TUN device.
 *
 * The lifecycle mutex serializes Wintun API transactions, device-count
 * transitions, and provider teardown admission.  Queue synchronization is a
 * separate domain added with the receive path.
 */
struct kernel_wintun_context_t {
	kernel_wintun_api_t api;
	mutex_t *lifecycle;
	bool teardown_requested;
	u_int active_devices;
	bool device_was_created;
};

/**
 * Create a Wintun-backed TUN device.
 *
 * @param context	provider context retained for the device lifetime
 * @param name_tmpl	device name template, or NULL for the default
 * @return		TUN device, or NULL if creation failed
 */
tun_device_t *kernel_wintun_device_create(kernel_wintun_context_t *context,
										   const char *name_tmpl);

#endif /** KERNEL_WINTUN_DEVICE_H_ @}*/
