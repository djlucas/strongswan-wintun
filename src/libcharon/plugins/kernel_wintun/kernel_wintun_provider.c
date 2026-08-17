/*
 * Copyright (C) 2026 DJ Lucas
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 */

/* Include before strongSwan's memory wrappers redefine memcpy and memmove;
 * otherwise MinGW's inline wide-character functions use the wrapper macros. */
#include <wchar.h>

#include "kernel_wintun_provider.h"
#include "kernel_wintun_api.h"
#include "kernel_wintun_device.h"

#include <utils/debug.h>

#define WINTUN_DLL_NAME L"wintun.dll"

typedef struct private_kernel_wintun_provider_t
		private_kernel_wintun_provider_t;

struct private_kernel_wintun_provider_t {
	kernel_wintun_provider_t public;
	HMODULE module;
	kernel_wintun_context_t context;
};

/**
 * Load wintun.dll from beside the executable, without searching the current
 * working directory or PATH.
 */
static HMODULE load_module(void)
{
	wchar_t path[MAX_PATH];
	wchar_t *name;
	size_t remaining;
	DWORD len, err;
	HMODULE module;

	len = GetModuleFileNameW(NULL, path, countof(path));
	if (!len || len >= countof(path))
	{
		err = GetLastError();
		DBG1(DBG_LIB, "locating application directory for wintun.dll failed: "
			 "Windows error %lu", err);
		return NULL;
	}
	name = wcsrchr(path, L'\\');
	if (!name)
	{
		DBG1(DBG_LIB, "locating application directory for wintun.dll failed");
		return NULL;
	}
	name++;
	remaining = countof(path) - (name - path);
	if (remaining < countof(WINTUN_DLL_NAME))
	{
		DBG1(DBG_LIB, "application path is too long to locate wintun.dll");
		return NULL;
	}
	memcpy(name, WINTUN_DLL_NAME, sizeof(WINTUN_DLL_NAME));

	module = LoadLibraryExW(path, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR |
									 LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (!module)
	{
		err = GetLastError();
		if (err == ERROR_INVALID_PARAMETER)
		{
			DBG1(DBG_LIB, "loading wintun.dll failed: controlled DLL search "
				 "flags are unsupported (Windows 7 and Server 2008 R2 require "
				 "KB2533623)");
		}
		else
		{
			DBG1(DBG_LIB, "loading wintun.dll from the application directory "
				 "failed: Windows error %lu", err);
		}
	}
	return module;
}

METHOD(windows_tun_device_provider_t, create, tun_device_t*,
	private_kernel_wintun_provider_t *this, const char *name_tmpl)
{
	return kernel_wintun_device_create(&this->context, name_tmpl);
}

METHOD(kernel_wintun_provider_t, destroy, void,
	private_kernel_wintun_provider_t *this)
{
	u_int active_devices;

	this->context.lifecycle->lock(this->context.lifecycle);
	this->context.teardown_requested = TRUE;
	active_devices = this->context.active_devices;
	if (active_devices)
	{
		/* Device lifetime owns both counter transitions; the provider only reads
		 * the count while holding the shared lifecycle lock.  A nonzero count
		 * means plugin teardown violated the required lifetime order.  Retain the
		 * module, API table, mutex, and provider memory so surviving devices can
		 * reject new API admission and still destroy themselves safely.  This
		 * bounded leak is intentional.  Reaching zero later does not resume
		 * teardown; never move deferred unload into a device destructor. */
		DBG0(DBG_LIB, "kernel-wintun provider teardown requested with %u active "
			 "device%s; retaining provider state and wintun.dll",
			 active_devices, active_devices == 1 ? "" : "s");
		this->context.lifecycle->unlock(this->context.lifecycle);
		return;
	}
	this->context.lifecycle->unlock(this->context.lifecycle);

	if (!FreeLibrary(this->module))
	{
		DBG1(DBG_LIB, "unloading wintun.dll failed: Windows error %lu",
			 GetLastError());
	}
	else
	{
		if (GetModuleHandleW(WINTUN_DLL_NAME))
		{
			if (this->context.device_was_created)
			{
				/* WintunCloseAdapter() queues asynchronous orphan cleanup.  Runtime
				 * validation observed that the module may remain loaded temporarily
				 * after an adapter lifecycle, but this check cannot identify the
				 * reference owner. */
				DBG2(DBG_LIB, "wintun.dll remains loaded after adapter teardown; "
					 "Wintun cleanup may still be pending");
			}
			else
			{
				/* With no adapter lifecycle, kernel-wintun is expected to be the
				 * process's only Wintun loader. */
				DBG1(DBG_LIB, "wintun.dll remains loaded after provider teardown");
			}
		}
		else
		{
			DBG2(DBG_LIB, "unloaded wintun.dll");
		}
	}
	this->context.lifecycle->destroy(this->context.lifecycle);
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
		.context = {
			.lifecycle = mutex_create(MUTEX_TYPE_DEFAULT),
		},
	);
	this->module = load_module();
	if (!this->module)
	{
		this->context.lifecycle->destroy(this->context.lifecycle);
		free(this);
		return NULL;
	}
	if (!kernel_wintun_api_load(this->module, &this->context.api))
	{
		FreeLibrary(this->module);
		this->context.lifecycle->destroy(this->context.lifecycle);
		free(this);
		return NULL;
	}
	DBG1(DBG_LIB, "loaded wintun.dll and resolved Wintun API");

	return &this->public;
}
