/*
 *   LASH
 *
 *   Copyright (C) 2002 Robert Ham <rah@bash.sh>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <string.h>

#include "jack_patch.h"
#include "jack_mgr_client.h"
#include "common/safety.h"

jack_patch_t *
jack_patch_new(void)
{
	return lash_calloc(1, sizeof(jack_patch_t));
}

void
jack_patch_destroy(jack_patch_t *patch)
{
	if (patch) {
		lash_free(&patch->src_client);
		lash_free(&patch->dest_client);
		lash_free(&patch->src_port);
		lash_free(&patch->dest_port);
		lash_free(&patch->src_desc);
		lash_free(&patch->dest_desc);
		free(patch);
	}
}

static void
jack_patch_set_src_desc(jack_patch_t *patch)
{
	char *ptr = NULL;
	size_t len = 2;

	if (patch->src_desc)
		free(patch->src_desc);

	if (patch->src_client) {
		ptr = patch->src_client;
		len += strlen(ptr);
	} else {
		char id[37];
		uuid_unparse(patch->src_client_id, id);
		ptr = (char *) id;
		len += 36;
	}

	patch->src_desc = lash_malloc(1, len + strlen(patch->src_port));
	sprintf(patch->src_desc, "%s:%s", ptr, patch->src_port);
}

static void
jack_patch_set_dest_desc(jack_patch_t *patch)
{
	char *ptr = NULL;
	size_t len = 2;

	if (patch->dest_desc)
		free(patch->dest_desc);

	if (patch->dest_client) {
		ptr = patch->dest_client;
		len += strlen(ptr);
	} else {
		char id[37];
		uuid_unparse(patch->dest_client_id, id);
		ptr = (char *) id;
		len += 36;
	}

	patch->dest_desc = lash_malloc(1, len + strlen(patch->dest_port));
	sprintf(patch->dest_desc, "%s:%s", ptr, patch->dest_port);
}

void
jack_patch_create_xml(jack_patch_t *patch,
                      xmlNodePtr    parent)
{
	xmlNodePtr patchxml;
	char id_str[37];

	patchxml = xmlNewChild(parent, NULL, BAD_CAST "jack_patch", NULL);

	if (uuid_is_null(patch->src_client_id))
		xmlNewChild(patchxml, NULL, BAD_CAST "src_client",
		            BAD_CAST patch->src_client);
	else {
		uuid_unparse(patch->src_client_id, id_str);
		xmlNewChild(patchxml, NULL, BAD_CAST "src_client_id",
		            BAD_CAST id_str);
	}

	xmlNewChild(patchxml, NULL, BAD_CAST "src_port",
	            BAD_CAST patch->src_port);

	if (uuid_is_null(patch->dest_client_id))
		xmlNewChild(patchxml, NULL, BAD_CAST "dest_client",
		            BAD_CAST patch->dest_client);
	else {
		uuid_unparse(patch->dest_client_id, id_str);
		xmlNewChild(patchxml, NULL, BAD_CAST "dest_client_id",
		            BAD_CAST id_str);
	}

	xmlNewChild(patchxml, NULL, BAD_CAST "dest_port",
	            BAD_CAST patch->dest_port);
}

void
jack_patch_parse_xml(jack_patch_t *patch,
                     xmlNodePtr    parent)
{
	xmlNodePtr xmlnode;
	xmlChar *content;

	for (xmlnode = parent->children; xmlnode; xmlnode = xmlnode->next) {
		if (strcmp((const char *) xmlnode->name, "src_client") == 0) {
			content = xmlNodeGetContent(xmlnode);
			lash_strset(&patch->src_client, (const char *) content);
			xmlFree(content);
		} else if (strcmp((const char *) xmlnode->name, "src_client_id") == 0) {
			content = xmlNodeGetContent(xmlnode);
			/* If the XML data includes a valid client ID
			   the client name must always be unset */
			if (uuid_parse((char *) content, patch->src_client_id) == 0)
				lash_free(&patch->src_client);
			xmlFree(content);
		} else if (strcmp((const char *) xmlnode->name, "src_port") == 0) {
			content = xmlNodeGetContent(xmlnode);
			lash_strset(&patch->src_port, (const char *) content);
			xmlFree(content);
		} else if (strcmp((const char *) xmlnode->name, "dest_client") == 0) {
			content = xmlNodeGetContent(xmlnode);
			lash_strset(&patch->dest_client, (const char *) content);
			xmlFree(content);
		} else if (strcmp((const char *) xmlnode->name, "dest_client_id") == 0) {
			content = xmlNodeGetContent(xmlnode);
			/* If the XML data includes a valid client ID
			   the client name must always be unset */
			if (uuid_parse((char *) content, patch->dest_client_id) == 0)
				lash_free(&patch->dest_client);
			xmlFree(content);
		} else if (strcmp((const char *) xmlnode->name, "dest_port") == 0) {
			content = xmlNodeGetContent(xmlnode);
			lash_strset(&patch->dest_port, (const char *) content);
			xmlFree(content);
		}
	}

	jack_patch_set_src_desc(patch);
	jack_patch_set_dest_desc(patch);
}

jack_patch_t *
jack_patch_dup(const jack_patch_t *other)
{
	jack_patch_t *patch;

	patch = lash_calloc(1, sizeof(jack_patch_t));

	lash_strset(&patch->src_client, other->src_client);
	lash_strset(&patch->src_port, other->src_port);
	lash_strset(&patch->dest_client, other->dest_client);
	lash_strset(&patch->dest_port, other->dest_port);

	uuid_copy(patch->src_client_id, other->src_client_id);
	uuid_copy(patch->dest_client_id, other->dest_client_id);

	jack_patch_set_src_desc(patch);
	jack_patch_set_dest_desc(patch);

	return patch;
}

#ifdef HAVE_JACK_DBUS

jack_patch_t *
jack_patch_new_with_all(uuid_t     *src_uuid,
                        uuid_t     *dest_uuid,
                        const char *src_name,
                        const char *dest_name,
                        const char *src_port,
                        const char *dest_port)
{
	jack_patch_t *patch;

	patch = lash_calloc(1, sizeof(jack_patch_t));

	if (src_uuid)
		uuid_copy(patch->src_client_id, *src_uuid);
	else
		patch->src_client = lash_strdup(src_name);

	if (dest_uuid)
		uuid_copy(patch->dest_client_id, *dest_uuid);
	else
		patch->dest_client = lash_strdup(dest_name);

	patch->src_port = lash_strdup(src_port);
	patch->dest_port = lash_strdup(dest_port);

	jack_patch_set_src_desc(patch);
	jack_patch_set_dest_desc(patch);

	return patch;
}

jack_patch_t *
jack_patch_find_by_description(struct list_head *patch_list,
                               uuid_t           *src_uuid,
                               const char       *src_name,
                               const char       *src_port,
                               uuid_t           *dest_uuid,
                               const char       *dest_name,
                               const char       *dest_port)
{
	struct list_head *node;

 	/* Iterate the patch list looking for one that matches the desription */
	list_for_each (node, patch_list) {
		jack_patch_t *patch = list_entry(node, jack_patch_t, siblings);

		/* Check if source and destination ports match */
		if (strcmp(patch->src_port, src_port) != 0
		    || strcmp(patch->dest_port, dest_port) != 0)
			continue;

		/* Check if source clients match */
		if (src_uuid) {
			if (uuid_is_null(patch->src_client_id)
			    || uuid_compare(*src_uuid, patch->src_client_id) != 0)
				continue;
		} else if (strcmp(patch->src_client, src_name) != 0)
			continue;

		/* Check if destination clients match */
		if (dest_uuid) {
			if (uuid_is_null(patch->dest_client_id)
			    || uuid_compare(*dest_uuid, patch->dest_client_id) != 0)
				continue;
		} else if (strcmp(patch->dest_client, dest_name) != 0)
			continue;

		/* Match found */
		return patch;
	}

	return NULL;
}

#else /* !HAVE_JACK_DBUS */

/*
 * Find a client in the JACK manager client list whose name matches
 * the supplied name, and assign the given id to it.
 */
static void
jack_patch_set_client_id(struct list_head *jack_mgr_clients,
                         const char       *patch_client_name,
                         uuid_t            id)
{
	struct list_head *node;
	jack_mgr_client_t *client;

	/* find the client */
	list_for_each (node, jack_mgr_clients) {
		client = list_entry(node, jack_mgr_client_t, siblings);

		if (strcmp(client->name, patch_client_name) == 0) {
			uuid_copy(id, client->id);
			return;
		}
	}
}

void
jack_patch_set(jack_patch_t     *patch,
               struct list_head *jack_mgr_clients)
{
	jack_patch_set_client_id(jack_mgr_clients, patch->src_client,
	                         patch->src_client_id);
	jack_patch_set_client_id(jack_mgr_clients, patch->dest_client,
	                         patch->dest_client_id);
}

static bool
jack_patch_unset_client_id(jack_patch_t      *patch,
                           struct list_head  *jack_mgr_clients,
                           uuid_t             patch_client_id,
                           char             **client_name_pptr)
{
	struct list_head *node;
	jack_mgr_client_t *client;

	/* find the client */
	list_for_each (node, jack_mgr_clients) {
		client = list_entry(node, jack_mgr_client_t, siblings);

		if (uuid_compare(patch_client_id, client->id) == 0)
			break;
	}

	if (!jack_mgr_clients)
		return false;

	lash_strset(client_name_pptr, client->name);

	return true;
}

bool
jack_patch_unset(jack_patch_t     *patch,
                 struct list_head *jack_mgr_clients)
{
	int retval = true;

	if (!uuid_is_null(patch->src_client_id)) {
		if (jack_patch_unset_client_id(patch, jack_mgr_clients,
		                               patch->src_client_id,
		                               &patch->src_client))
			jack_patch_set_src_desc(patch);
		else
			retval = false;
	}

	if (!uuid_is_null(patch->dest_client_id)) {
		if (jack_patch_unset_client_id(patch, jack_mgr_clients,
		                               patch->dest_client_id,
		                               &patch->dest_client))
			jack_patch_set_dest_desc(patch);
		else if (retval)
			retval = false;
	}

	return retval;
}

#define switch_str(a, b) \
  str = a; \
  a = b; \
  b = str;

void
jack_patch_switch_clients(jack_patch_t *patch)
{
	char *str;
	uuid_t id;

	switch_str(patch->src_client, patch->dest_client);
	switch_str(patch->src_port, patch->dest_port);

	uuid_copy(id, patch->src_client_id);
	uuid_copy(patch->src_client_id, patch->dest_client_id);
	uuid_copy(patch->dest_client_id, id);

}

#undef switch_str

static void
jack_patch_set_client_and_port(const char  *jack_port_name,
                               char       **client_name_pptr,
                               char       **port_name_pptr)
{
	char *full_name;
	char *ptr;

	full_name = lash_strdup(jack_port_name);

	ptr = strchr(full_name, ':');
	*ptr = '\0';
	ptr++;

	lash_strset(client_name_pptr, full_name);
	lash_strset(port_name_pptr, ptr);

	free(full_name);
}

void
jack_patch_set_src(jack_patch_t *patch,
                   const char   *src)
{
	jack_patch_set_client_and_port(src, &patch->src_client,
	                               &patch->src_port);
	jack_patch_set_src_desc(patch);
}

void
jack_patch_set_dest(jack_patch_t *patch,
                    const char   *dest)
{
	jack_patch_set_client_and_port(dest, &patch->dest_client,
	                               &patch->dest_port);
	jack_patch_set_dest_desc(patch);
}

#endif

/* EOF */
