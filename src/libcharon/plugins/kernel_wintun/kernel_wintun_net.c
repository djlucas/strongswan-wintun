/*
 * Copyright (C) 2026 DJ Lucas
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 */

#include "kernel_wintun_net.h"

typedef struct private_kernel_wintun_net_t private_kernel_wintun_net_t;

struct private_kernel_wintun_net_t {
	kernel_net_t public;
};

/* This deliberately inert backend lets charon run the Wintun data plane
 * before native Windows network management is added.  Queries return no
 * result, enumerations are empty, and mutations report NOT_SUPPORTED. */

METHOD(kernel_net_t, get_features, kernel_feature_t,
	private_kernel_wintun_net_t *this)
{
	return 0;
}

METHOD(kernel_net_t, get_source_addr, host_t*,
	private_kernel_wintun_net_t *this, host_t *dest, host_t *src)
{
	return NULL;
}

METHOD(kernel_net_t, get_nexthop, host_t*,
	private_kernel_wintun_net_t *this, host_t *dest, int prefix, host_t *src,
	char **iface)
{
	if (iface)
	{
		*iface = NULL;
	}
	return NULL;
}

METHOD(kernel_net_t, get_interface, bool,
	private_kernel_wintun_net_t *this, host_t *host, char **name)
{
	if (name)
	{
		*name = NULL;
	}
	return FALSE;
}

METHOD(kernel_net_t, create_address_enumerator, enumerator_t*,
	private_kernel_wintun_net_t *this, kernel_address_type_t which)
{
	return enumerator_create_empty();
}

METHOD(kernel_net_t, add_ip, status_t,
	private_kernel_wintun_net_t *this, host_t *virtual_ip, int prefix,
	char *iface)
{
	return NOT_SUPPORTED;
}

METHOD(kernel_net_t, del_ip, status_t,
	private_kernel_wintun_net_t *this, host_t *virtual_ip, int prefix,
	bool wait)
{
	return NOT_SUPPORTED;
}

METHOD(kernel_net_t, add_route, status_t,
	private_kernel_wintun_net_t *this, chunk_t dst_net, uint8_t prefixlen,
	host_t *gateway, host_t *src_ip, char *if_name, bool pass)
{
	return NOT_SUPPORTED;
}

METHOD(kernel_net_t, del_route, status_t,
	private_kernel_wintun_net_t *this, chunk_t dst_net, uint8_t prefixlen,
	host_t *gateway, host_t *src_ip, char *if_name, bool pass)
{
	return NOT_SUPPORTED;
}

METHOD(kernel_net_t, destroy, void,
	private_kernel_wintun_net_t *this)
{
	free(this);
}

kernel_net_t *kernel_wintun_net_create(void)
{
	private_kernel_wintun_net_t *this;

	INIT(this,
		.public = {
			.get_features = _get_features,
			.get_source_addr = _get_source_addr,
			.get_nexthop = _get_nexthop,
			.get_interface = _get_interface,
			.create_address_enumerator = _create_address_enumerator,
			.add_ip = _add_ip,
			.del_ip = _del_ip,
			.add_route = _add_route,
			.del_route = _del_route,
			.destroy = _destroy,
		},
	);

	return &this->public;
}
