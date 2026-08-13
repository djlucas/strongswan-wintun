/*
 * Copyright (C) 2026 DJ Lucas
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

/**
 * @defgroup windows_tun_provider windows_tun_provider
 * @{ @ingroup networking
 */

#ifndef WINDOWS_TUN_PROVIDER_H_
#define WINDOWS_TUN_PROVIDER_H_

#ifdef WIN32

#include <networking/tun_device.h>

typedef struct windows_tun_device_provider_t windows_tun_device_provider_t;

/**
 * Key used to register a Windows TUN device provider with library_t.
 */
#define WINDOWS_TUN_DEVICE_PROVIDER "windows-tun-device-provider"

/**
 * Windows TUN device provider registered with library_t.
 *
 * The registry stores a borrowed pointer.  A registered provider must outlive
 * all TUN devices it creates.
 */
struct windows_tun_device_provider_t {

	/**
	 * Create a TUN device using the given name template.
	 *
	 * @param name_tmpl	name template, or NULL for the default
	 * @return			TUN device, or NULL if creation failed
	 */
	tun_device_t *(*create)(windows_tun_device_provider_t *this,
							const char *name_tmpl);
};


#endif /* WIN32 */

#endif /** WINDOWS_TUN_PROVIDER_H_ @}*/
