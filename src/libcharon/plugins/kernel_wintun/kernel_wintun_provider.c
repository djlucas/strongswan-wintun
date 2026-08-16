/*
 * Copyright (C) 2026 DJ Lucas
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 */

#include "kernel_wintun_provider.h"

#include <utils/debug.h>

typedef struct private_kernel_wintun_provider_t
		private_kernel_wintun_provider_t;

struct private_kernel_wintun_provider_t {
	kernel_wintun_provider_t public;
};

METHOD(windows_tun_device_provider_t, create, tun_device_t*,
	private_kernel_wintun_provider_t *this, const char *name_tmpl)
{
	DBG1(DBG_LIB, "kernel-wintun TUN device creation is not implemented");
	return NULL;
}

METHOD(kernel_wintun_provider_t, destroy, void,
	private_kernel_wintun_provider_t *this)
{
	free(this);
}

kernel_wintun_provider_t *kernel_wintun_provider_create(void)
{
	private_kernel_wintun_provider_t *this;

	INIT(this,
		.public = {
			.provider = {
				.create = _create,
			},
			.destroy = _destroy,
		},
	);

	return &this->public;
}
