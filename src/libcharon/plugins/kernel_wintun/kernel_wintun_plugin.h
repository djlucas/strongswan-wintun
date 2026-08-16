/*
 * Copyright (C) 2026 DJ Lucas
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 */

/**
 * @defgroup kernel_wintun kernel_wintun
 * @ingroup cplugins
 *
 * @defgroup kernel_wintun_plugin kernel_wintun_plugin
 * @{ @ingroup kernel_wintun
 */

#ifndef KERNEL_WINTUN_PLUGIN_H_
#define KERNEL_WINTUN_PLUGIN_H_

#include <plugins/plugin.h>

typedef struct kernel_wintun_plugin_t kernel_wintun_plugin_t;

/**
 * Windows Wintun TUN device provider plugin.
 */
struct kernel_wintun_plugin_t {

	/**
	 * Implements plugin interface.
	 */
	plugin_t plugin;
};

#endif /** KERNEL_WINTUN_PLUGIN_H_ @}*/
