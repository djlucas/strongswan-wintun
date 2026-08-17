/*
 * Copyright (C) 2026 DJ Lucas
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 */

#include "kernel_wintun_plugin.h"
#include "kernel_wintun_net.h"
#include "kernel_wintun_provider.h"

#include <library.h>
#include <utils/debug.h>

typedef struct private_kernel_wintun_plugin_t private_kernel_wintun_plugin_t;

struct private_kernel_wintun_plugin_t {
	kernel_wintun_plugin_t public;
	kernel_wintun_provider_t *provider;
};

METHOD(plugin_t, get_name, char*,
	private_kernel_wintun_plugin_t *this)
{
	return "kernel-wintun";
}

METHOD(plugin_t, get_features, int,
	private_kernel_wintun_plugin_t *this, plugin_feature_t *features[])
{
	static plugin_feature_t f[] = {
		/* Keep an unconditional feature loaded even if another network backend
		 * wins registration.  The provider may already be borrowed by a TUN. */
		PLUGIN_PROVIDE(CUSTOM, "kernel-wintun"),
		PLUGIN_CALLBACK(kernel_net_register, kernel_wintun_net_create),
			PLUGIN_PROVIDE(CUSTOM, "kernel-net"),
	};
	*features = f;
	return countof(f);
}

METHOD(plugin_t, destroy, void,
	private_kernel_wintun_plugin_t *this)
{
	lib->set(lib, WINDOWS_TUN_DEVICE_PROVIDER, NULL);
	DBG1(DBG_LIB, "unregistered Windows TUN device provider for kernel-wintun");
	this->provider->destroy(this->provider);
	free(this);
}

/**
 * Check whether a plugin is selected by the modular runtime configuration.
 */
static bool plugin_selected(const char *name)
{
	/* Mirror modular_pluginlist(): non-zero priorities and true both load. */
	return lib->settings->get_int(lib->settings, "%s.plugins.%s.load", 0,
								  lib->ns, name) != 0 ||
		   lib->settings->get_bool(lib->settings, "%s.plugins.%s.load", FALSE,
								   lib->ns, name);
}

PLUGIN_DEFINE(kernel_wintun)
{
	private_kernel_wintun_plugin_t *this;
	kernel_wintun_provider_t *provider;
	bool kernel_iph, kernel_wfp;

	if (!lib->settings->get_bool(lib->settings, "%s.load_modular", FALSE,
								 lib->ns))
	{
		DBG1(DBG_LIB, "kernel-wintun requires modular plugin loading");
		return NULL;
	}
	kernel_iph = plugin_selected("kernel-iph");
	kernel_wfp = plugin_selected("kernel-wfp");

	if (kernel_iph && kernel_wfp)
	{
		DBG0(DBG_LIB, "kernel-wintun disabled: kernel-iph is also enabled; "
			 "if kernel-wfp is enabled charon may continue using the legacy "
			 "kernel-wfp + kernel-iph backend");
		return NULL;
	}
	if (kernel_iph)
	{
		DBG1(DBG_LIB, "kernel-wintun disabled: kernel-iph is also enabled");
		return NULL;
	}
	if (kernel_wfp)
	{
		DBG1(DBG_LIB, "kernel-wintun and kernel-wfp are both enabled; "
			 "continuing because failing kernel-wintun here would make silent "
			 "fallback to the legacy kernel-wfp data plane more likely");
	}

	provider = kernel_wintun_provider_create();
	if (!provider)
	{
		return NULL;
	}
	if (!lib->set(lib, WINDOWS_TUN_DEVICE_PROVIDER, &provider->provider))
	{
		if (lib->get(lib, WINDOWS_TUN_DEVICE_PROVIDER))
		{
			DBG1(DBG_LIB, "kernel-wintun failed to register: Windows TUN "
				 "device provider key is already held");
		}
		else
		{
			DBG1(DBG_LIB, "kernel-wintun failed to register Windows TUN "
				 "device provider");
		}
		provider->destroy(provider);
		return NULL;
	}
	DBG1(DBG_LIB, "registered Windows TUN device provider for kernel-wintun");

	INIT(this,
		.public = {
			.plugin = {
				.get_name = _get_name,
				.get_features = _get_features,
				.destroy = _destroy,
			},
		},
		.provider = provider,
	);

	return &this->public.plugin;
}
