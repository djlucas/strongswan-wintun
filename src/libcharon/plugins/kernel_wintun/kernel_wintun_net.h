/*
 * Copyright (C) 2026 DJ Lucas
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 */

/**
 * @defgroup kernel_wintun_net kernel_wintun_net
 * @{ @ingroup kernel_wintun
 */

#ifndef KERNEL_WINTUN_NET_H_
#define KERNEL_WINTUN_NET_H_

#include <kernel/kernel_net.h>

/**
 * Create the minimal kernel-wintun network backend.
 *
 * This backend intentionally provides no native address or route management.
 * It supplies the kernel_net_t interface required to run charon while the
 * complete Windows networking implementation is developed separately.
 *
 * @return		kernel network backend
 */
kernel_net_t *kernel_wintun_net_create(void);

#endif /** KERNEL_WINTUN_NET_H_ @}*/
