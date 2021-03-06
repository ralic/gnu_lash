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

#ifndef __LASHD_ALSA_CLIENT_H__
#define __LASHD_ALSA_CLIENT_H__

#include "../config.h"

#ifdef HAVE_ALSA

# include <uuid/uuid.h>

# include "types.h"
# include "common/klist.h"

struct _alsa_client
{
  struct list_head siblings;
  unsigned char   client_id;
  uuid_t          id;
  struct list_head patches;
  struct list_head old_patches;
  struct list_head backup_patches;
};

alsa_client_t * alsa_client_new ();
void            alsa_client_destroy (alsa_client_t * client);

void alsa_client_set_id          (alsa_client_t * client, uuid_t id);
void alsa_client_set_client_id   (alsa_client_t * client, unsigned char client_id);

void               alsa_client_dup_patches     (const alsa_client_t * client, struct list_head * dest);
struct list_head * alsa_client_get_patches     (alsa_client_t * client);
unsigned char alsa_client_get_client_id   (const alsa_client_t * client);
void          alsa_client_get_id          (const alsa_client_t * client, uuid_t id);

void alsa_client_free_patches (alsa_client_t * client);
void alsa_client_free_backup_patches (alsa_client_t * client);

#endif /* HAVE_ALSA */

#endif /* __LASHD_ALSA_CLIENT_H__ */
