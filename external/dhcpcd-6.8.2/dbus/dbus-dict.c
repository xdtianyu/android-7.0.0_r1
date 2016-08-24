/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2015 Roy Marples <roy@marples.name>
 * All rights reserved

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <arpa/inet.h>

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include <dbus/dbus.h>

#include "../config.h"
#include "dbus-dict.h"

static dbus_bool_t
append_sanitized_string(DBusMessageIter *iter, const char *value)
{
	dbus_bool_t ret;
	int len = strlen(value);
	char *sanitized_value = NULL;
	int i;

	for (i = 0; i < len; i++) {
		if (isascii(value[i]) || isprint(value[i])) {
			if (sanitized_value)
				sanitized_value[i] = value[i];
		} else {
			if (sanitized_value == NULL) {
				sanitized_value = malloc(len + 1);
				if (sanitized_value == NULL) {
					syslog(LOG_ERR, "DBus string parameter "
					       "sanitization failed due to "
					       "malloc failure");
					return FALSE;
				}
				memcpy(sanitized_value, value, i);
			}
			sanitized_value[i] = '?';
		}
	}
	if (sanitized_value) {
		syslog(LOG_ERR, "DBus string parameter sanitization"
                       " was invoked");
		sanitized_value[i] = '\0';
		ret = dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING,
	            &sanitized_value);

		free(sanitized_value);
	} else {
		ret = dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING,
	            &value);
	}

	return ret;
}

static int
append_config_value(DBusMessageIter *entry, int type,
    const char *data)
{
	int retval;
	DBusMessageIter var;
	unsigned char byte;
	dbus_uint16_t u16;
	dbus_uint32_t u32;
	dbus_int16_t i16;
	dbus_int32_t i32;
	struct in_addr in;

	retval = -1;
	switch (type) {
	case DBUS_TYPE_BOOLEAN:
		if (*data == '0' || *data == '\0')
			u32 = 0;
		else
			u32 = 1;
		dbus_message_iter_open_container(entry, DBUS_TYPE_VARIANT,
		    DBUS_TYPE_BOOLEAN_AS_STRING, &var);
		if (dbus_message_iter_append_basic(&var,
			DBUS_TYPE_BOOLEAN, &u32))
			retval = 0;
		break;
	case DBUS_TYPE_BYTE:
		byte = strtoul(data, NULL, 0);
		dbus_message_iter_open_container(entry, DBUS_TYPE_VARIANT,
		    DBUS_TYPE_BYTE_AS_STRING, &var);
		if (dbus_message_iter_append_basic(&var, DBUS_TYPE_BYTE,
			&byte))
			retval = 0;
		break;
	case DBUS_TYPE_STRING:
		dbus_message_iter_open_container(entry, DBUS_TYPE_VARIANT,
		    DBUS_TYPE_STRING_AS_STRING, &var);
		if (append_sanitized_string(&var, data))
			retval = 0;
		break;
	case DBUS_TYPE_INT16:
		i16 = strtol(data, NULL, 0);
		dbus_message_iter_open_container(entry, DBUS_TYPE_VARIANT,
		    DBUS_TYPE_INT16_AS_STRING, &var);
		if (dbus_message_iter_append_basic(&var,
			DBUS_TYPE_INT16, &i16))
			retval = 0;
		break;
	case DBUS_TYPE_UINT16:
		u16 = strtoul(data, NULL, 0);
		dbus_message_iter_open_container(entry, DBUS_TYPE_VARIANT,
		    DBUS_TYPE_UINT16_AS_STRING, &var);
		if (dbus_message_iter_append_basic(&var,
			DBUS_TYPE_UINT16, &u16))
			retval = 0;
		break;
	case DBUS_TYPE_INT32:
		i32 = strtol(data, NULL, 0);
		dbus_message_iter_open_container(entry, DBUS_TYPE_VARIANT,
		    DBUS_TYPE_INT32_AS_STRING, &var);
		if (dbus_message_iter_append_basic(&var,
			DBUS_TYPE_INT32, &i32))
			retval = 0;
		break;
	case DBUS_TYPE_UINT32:
		if (strchr(data, '.') != NULL && inet_aton(data, &in) == 1)
			u32 = in.s_addr;
		else
			u32 = strtoul(data, NULL, 0);
		dbus_message_iter_open_container(entry, DBUS_TYPE_VARIANT,
		    DBUS_TYPE_UINT32_AS_STRING, &var);
		if (dbus_message_iter_append_basic(&var,
			DBUS_TYPE_UINT32, &u32))
			retval = 0;
		break;
	default:
		retval = 1;
		break;
	}
	if (retval == 0)
		dbus_message_iter_close_container(entry, &var);
	else if (retval == 1)
		retval = 0;

	return retval;
}

static int
append_config_byte_array(DBusMessageIter *entry, const char *data)
{
	DBusMessageIter var, array;
	dbus_bool_t ok = TRUE;
	uint8_t u8, u8_2;
	size_t len;
	const char *it, *end;
	const char *tsa, *ts;

	tsa = DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_BYTE_AS_STRING;
	ts = DBUS_TYPE_BYTE_AS_STRING;

	dbus_message_iter_open_container(entry, DBUS_TYPE_VARIANT, tsa, &var);
	dbus_message_iter_open_container(&var, DBUS_TYPE_ARRAY, ts, &array);

	len = strlen(data);
	it = data;
	end = data + len;

	/* "a12" is treated as "0a12" */
	if (len & 1) {
		ok = (sscanf(it++, "%1hhx", &u8) == 1) &&
			dbus_message_iter_append_basic(&array, DBUS_TYPE_BYTE,
						       &u8);
	}

	while (ok && it < end) {
		/* sscanf("1z", "%2hhx", &u8) will store 0x01 in u8 and
		 * will return 1 */
		ok = (sscanf(it++, "%1hhx", &u8) == 1) &&
			(sscanf(it++, "%1hhx", &u8_2) == 1);
		if (!ok)
			break;

		u8 = (u8 << 4) | u8_2;
		ok = dbus_message_iter_append_basic(&array, DBUS_TYPE_BYTE, &u8);
	}

	dbus_message_iter_close_container(&var, &array);
	dbus_message_iter_close_container(entry, &var);
	return ok ? 0 : -1;
}

static int
append_config_array(DBusMessageIter *entry, int type, const char *data)
{
	int retval;
	char *ns, *p, *tok;
	const char *tsa, *ts;
	DBusMessageIter var, array;
	dbus_bool_t ok;
	dbus_uint32_t u32;
	struct in_addr in;

	if (type == DBUS_TYPE_BYTE)
		return append_config_byte_array(entry, data);

	switch (type) {
	case DBUS_TYPE_STRING:
		tsa = DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_STRING_AS_STRING;
		ts = DBUS_TYPE_STRING_AS_STRING;
		break;
	case DBUS_TYPE_UINT32:
		tsa = DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_UINT32_AS_STRING;
		ts = DBUS_TYPE_UINT32_AS_STRING;
		break;
	default:
		return -1;
	}

	ns = p = strdup(data);
	if (ns == NULL)
		return -1;
	retval = 0;

	dbus_message_iter_open_container(entry, DBUS_TYPE_VARIANT, tsa, &var);
	dbus_message_iter_open_container(&var, DBUS_TYPE_ARRAY, ts, &array);
	while ((tok = strsep(&p, " ")) != NULL) {
		if (*tok == '\0')
			continue;
		switch(type) {
		case DBUS_TYPE_STRING:
			ok = append_sanitized_string(&array, tok);
			break;
		case DBUS_TYPE_UINT32:
			if (strchr(tok, '.') != NULL &&
			    inet_aton(tok, &in) == 1)
				u32 = in.s_addr;
			else
				u32 = strtoul(tok, NULL, 0);
			ok = dbus_message_iter_append_basic(&array,
			    DBUS_TYPE_UINT32, &u32);
			break;
		default:
			ok = FALSE;
			break;
		}
		if (!ok)
			break;
	}
	dbus_message_iter_close_container(&var, &array);
	dbus_message_iter_close_container(entry, &var);
	free(ns);
	return retval;
}

int
dict_append_config_item(DBusMessageIter *iter, const struct o_dbus *op,
    const char *data)
{
	int retval;
	DBusMessageIter entry;

	retval = 0;
	if (*data == '\0')
		return retval;
	dbus_message_iter_open_container(iter,
	    DBUS_TYPE_DICT_ENTRY,
	    NULL,
	    &entry);
	append_sanitized_string(&entry, op->name);
	if (op->type == DBUS_TYPE_ARRAY)
		retval = append_config_array(&entry, op->sub_type, data);
	else
		retval = append_config_value(&entry, op->type, data);
	dbus_message_iter_close_container(iter, &entry);
	return retval;
}
