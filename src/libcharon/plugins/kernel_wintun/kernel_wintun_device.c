/*
 * Copyright (C) 2026 DJ Lucas
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 */

#include "kernel_wintun_device.h"

#include <utils/debug.h>

#define WINTUN_DEFAULT_NAME "ipsec0"
#define WINTUN_TUNNEL_TYPE L"strongSwan"
#define WINTUN_MAX_ADAPTER_NAME 128
#define WINTUN_RING_CAPACITY 0x400000
#define WINTUN_MAX_PACKET_SIZE 0xFFFF

typedef struct private_kernel_wintun_device_t private_kernel_wintun_device_t;

struct private_kernel_wintun_device_t {
	tun_device_t public;
	kernel_wintun_context_t *context;
	kernel_wintun_adapter_handle_t adapter;
	kernel_wintun_session_handle_t session;
	char name[WINTUN_MAX_ADAPTER_NAME];
	host_t *address;
	uint8_t netmask;
	int mtu;
	bool shutting_down;
	bool terminal;
	bool counted;
};

/**
 * Produce the first concrete name from the tun_device_t name template.
 *
 * Patch 5 deliberately supports a single device.  Substituting zero preserves
 * the name expected by kernel-libipsec without implying that a collision may
 * be resolved by adopting another process's adapter.  Indexed allocation is
 * added when multiple devices are supported.
 */
static bool format_name(char *name, size_t size, const char *name_tmpl)
{
	const char *marker, *suffix;
	size_t prefix, suffix_len;

	name_tmpl = name_tmpl ?: WINTUN_DEFAULT_NAME;
	marker = strstr(name_tmpl, "%d");
	if (!marker)
	{
		suffix_len = strlen(name_tmpl);
		if (suffix_len >= size)
		{
			return FALSE;
		}
		memcpy(name, name_tmpl, suffix_len + 1);
		return TRUE;
	}
	prefix = marker - name_tmpl;
	suffix = marker + 2;
	suffix_len = strlen(suffix);
	if (prefix + 1 + suffix_len >= size)
	{
		return FALSE;
	}
	memcpy(name, name_tmpl, prefix);
	name[prefix] = '0';
	memcpy(name + prefix + 1, suffix, suffix_len + 1);
	return TRUE;
}

/**
 * Convert the strongSwan UTF-8 interface name to the UTF-16 Wintun API form.
 */
static bool widen_name(const char *name, wchar_t *wide, size_t count)
{
	if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name, -1, wide,
						 count))
	{
		DBG1(DBG_LIB, "converting Wintun adapter name '%s' failed: Windows "
			 "error %lu", name, GetLastError());
		return FALSE;
	}
	return TRUE;
}

METHOD(tun_device_t, read_packet, bool,
	private_kernel_wintun_device_t *this, chunk_t *packet)
{
	DBG1(DBG_LIB, "reading from Wintun device %s is not implemented",
		 this->name);
	return FALSE;
}

METHOD(tun_device_t, write_packet, bool,
	private_kernel_wintun_device_t *this, chunk_t packet)
{
	BYTE *buffer;
	bool success = FALSE;

	if (packet.len > WINTUN_MAX_PACKET_SIZE)
	{
		DBG1(DBG_LIB, "packet for Wintun device %s is too large: %zu bytes",
			 this->name, packet.len);
		return FALSE;
	}
	/* The whole allocate-copy-send transaction is intentionally serialized.
	 * Besides simplifying Wintun ring access, this mutex is the teardown drain
	 * barrier: once destroy() acquires it, all admitted sends have completed. */
	this->context->lifecycle->lock(this->context->lifecycle);
	if (!this->context->teardown_requested && !this->shutting_down &&
		!this->terminal)
	{
		buffer = this->context->api.allocate_send_packet(this->session,
													 packet.len);
		if (buffer)
		{
			memcpy(buffer, packet.ptr, packet.len);
			this->context->api.send_packet(this->session, buffer);
			success = TRUE;
		}
		else
		{
			DBG1(DBG_LIB, "allocating send packet for Wintun device %s failed: "
				 "Windows error %lu", this->name, GetLastError());
		}
	}
	this->context->lifecycle->unlock(this->context->lifecycle);
	return success;
}

METHOD(tun_device_t, set_address, bool,
	private_kernel_wintun_device_t *this, host_t *address, uint8_t netmask)
{
	host_t *clone;

	/* Address application belongs to kernel-wintun's kernel_net_t backend.
	 * Until that lands, retain the requested interface state for its callers. */
	clone = address->clone(address);
	this->context->lifecycle->lock(this->context->lifecycle);
	if (this->context->teardown_requested || this->shutting_down ||
		this->terminal)
	{
		this->context->lifecycle->unlock(this->context->lifecycle);
		clone->destroy(clone);
		return FALSE;
	}
	DESTROY_IF(this->address);
	this->address = clone;
	this->netmask = netmask;
	this->context->lifecycle->unlock(this->context->lifecycle);
	return TRUE;
}

METHOD(tun_device_t, get_address, host_t*,
	private_kernel_wintun_device_t *this, uint8_t *netmask)
{
	host_t *address;

	/* Match tun_device_t's existing borrowed-pointer contract.  The lock makes
	 * the state snapshot consistent, but callers must still serialize a later
	 * set_address() against their use of the returned pointer. */
	this->context->lifecycle->lock(this->context->lifecycle);
	if (netmask && this->address)
	{
		*netmask = this->netmask;
	}
	address = this->address;
	this->context->lifecycle->unlock(this->context->lifecycle);
	return address;
}

METHOD(tun_device_t, up, bool,
	private_kernel_wintun_device_t *this)
{
	bool success;

	/* Wintun adapters require no separate up operation.  Native interface
	 * configuration is implemented by the kernel_net_t backend. */
	this->context->lifecycle->lock(this->context->lifecycle);
	success = !this->context->teardown_requested && !this->shutting_down &&
			  !this->terminal;
	this->context->lifecycle->unlock(this->context->lifecycle);
	return success;
}

METHOD(tun_device_t, set_mtu, bool,
	private_kernel_wintun_device_t *this, int mtu)
{
	bool success = FALSE;

	/* Retain the requested MTU for tun_device_t.  Applying it to the Windows
	 * interface belongs to the kernel_net_t backend. */
	if (mtu <= 0 || mtu > WINTUN_MAX_PACKET_SIZE)
	{
		return FALSE;
	}
	this->context->lifecycle->lock(this->context->lifecycle);
	if (!this->context->teardown_requested && !this->shutting_down &&
		!this->terminal)
	{
		this->mtu = mtu;
		success = TRUE;
	}
	this->context->lifecycle->unlock(this->context->lifecycle);
	return success;
}

METHOD(tun_device_t, get_mtu, int,
	private_kernel_wintun_device_t *this)
{
	return this->mtu;
}

METHOD(tun_device_t, get_name, char*,
	private_kernel_wintun_device_t *this)
{
	return this->name;
}

METHOD(tun_device_t, get_fd, int,
	private_kernel_wintun_device_t *this)
{
	/* Patch 6 supplies the Winsock readiness descriptor used by WSAPoll(). */
	return -1;
}

METHOD(tun_device_t, destroy, void,
	private_kernel_wintun_device_t *this)
{
	kernel_wintun_context_t *context = this->context;

	context->lifecycle->lock(context->lifecycle);
	this->shutting_down = TRUE;
	context->lifecycle->unlock(context->lifecycle);

	/* Patch 6 joins the receive worker and releases queue/readiness resources
	 * here, before the session destruction boundary.  Keeping that work outside
	 * the lifecycle mutex is deliberate: the worker must be allowed to finish
	 * without teardown waiting for it while holding its API admission lock. */
	context->lifecycle->lock(context->lifecycle);
	if (this->session)
	{
		context->api.end_session(this->session);
		this->session = NULL;
	}
	if (this->adapter)
	{
		context->api.close_adapter(this->adapter);
		this->adapter = NULL;
	}
	/* Device lifetime owns both count transitions.  The provider only observes
	 * the count under this same lock.  If abnormal provider teardown pinned the
	 * context, reaching zero here deliberately does not resume provider teardown
	 * or unload the DLL from a device destructor. */
	if (this->counted)
	{
		context->active_devices--;
		this->counted = FALSE;
	}
	context->lifecycle->unlock(context->lifecycle);

	DESTROY_IF(this->address);
	free(this);
}

tun_device_t *kernel_wintun_device_create(kernel_wintun_context_t *context,
										   const char *name_tmpl)
{
	private_kernel_wintun_device_t *this;
	wchar_t name[WINTUN_MAX_ADAPTER_NAME];

	INIT(this,
		.public = {
			.read_packet = _read_packet,
			.write_packet = _write_packet,
			.set_address = _set_address,
			.get_address = _get_address,
			.up = _up,
			.set_mtu = _set_mtu,
			.get_mtu = _get_mtu,
			.get_name = _get_name,
			.get_fd = _get_fd,
			.destroy = _destroy,
		},
		.context = context,
		.mtu = 1500,
	);
	if (!format_name(this->name, sizeof(this->name), name_tmpl) ||
		!widen_name(this->name, name, countof(name)))
	{
		DBG1(DBG_LIB, "invalid Wintun adapter name template");
		free(this);
		return NULL;
	}

	/* Construction is one lifecycle transaction.  Adapter creation may be
	 * slow, but serializing it prevents construction, sends, and provider
	 * teardown from interleaving.  All failures converge on one unlock path. */
	context->lifecycle->lock(context->lifecycle);
	if (context->teardown_requested)
	{
		goto failed;
	}
	this->adapter = context->api.create_adapter(name, WINTUN_TUNNEL_TYPE, NULL);
	if (!this->adapter)
	{
		/* Do not adopt an existing adapter.  A name collision indicates another
		 * live owner, and sharing it would provide no lifecycle coordination. */
		DBG1(DBG_LIB, "creating Wintun adapter %s failed: Windows error %lu",
			 this->name, GetLastError());
		goto failed;
	}
	this->session = context->api.start_session(this->adapter,
											 WINTUN_RING_CAPACITY);
	if (!this->session)
	{
		DBG1(DBG_LIB, "starting Wintun session for %s failed: Windows error %lu",
			 this->name, GetLastError());
		goto failed;
	}

	/* A partially constructed device is never counted and therefore never
	 * decrements.  Publish the count only after every fallible step succeeded. */
	context->active_devices++;
	context->device_was_created = TRUE;
	this->counted = TRUE;
	context->lifecycle->unlock(context->lifecycle);
	DBG1(DBG_LIB, "created Wintun device: %s", this->name);
	return &this->public;

failed:
	if (this->session)
	{
		context->api.end_session(this->session);
	}
	if (this->adapter)
	{
		context->api.close_adapter(this->adapter);
	}
	context->lifecycle->unlock(context->lifecycle);
	free(this);
	return NULL;
}
