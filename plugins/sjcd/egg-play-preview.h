/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

/*
 * EggPlayPreview GTK+ Widget - egg-play-preview.h
 *
 * Copyright (C) 2008 Luca Cavalli <luca.cavalli@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Authors: Luca Cavalli <luca.cavalli@gmail.com>
 */

#ifndef __EGG_PLAY_PREVIEW_H__
#define __EGG_PLAY_PREVIEW_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define EGG_TYPE_PLAY_PREVIEW          (egg_play_preview_get_type ())
#define EGG_PLAY_PREVIEW(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), EGG_TYPE_PLAY_PREVIEW, EggPlayPreview))
#define EGG_PLAY_PREVIEW_CLASS(obj)    (G_TYPE_CHECK_CLASS_CAST ((obj), EGG_PLAY_PREVIEW, EggPlayPreviewClass))
#define EGG_IS_PLAY_PREVIEW(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EGG_TYPE_PLAY_PREVIEW))
#define EGG_IS_PLAY_PREVIEW_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), EFF_TYPE_PLAY_PREVIEW))
#define EGG_PLAY_PREVIEW_GET_CLASS     (G_TYPE_INSTANCE_GET_CLASS ((obj), EGG_TYPE_PLAY_PREVIEW, EggPlayPreviewClass))

#define EGG_PLAYER_PREVIEW_WIDTH 100

typedef struct _EggPlayPreview        EggPlayPreview;
typedef struct _EggPlayPreviewClass   EggPlayPreviewClass;
typedef struct _EggPlayPreviewPrivate EggPlayPreviewPrivate;

struct _EggPlayPreview {
	GtkBox parent;

	gchar *file;

	/*< private >*/
	EggPlayPreviewPrivate *priv;
};

struct _EggPlayPreviewClass {
	GtkBoxClass parent_class;

	/* signals */
	void (* play)  (EggPlayPreview *play_preview);
	void (* pause) (EggPlayPreview *play_preview);
	void (* stop)  (EggPlayPreview *play_preview);
};

GType      egg_play_preview_get_type     (void) G_GNUC_CONST;
GtkWidget *egg_play_preview_new          (void);
GtkWidget *egg_play_preview_new_with_uri (const gchar *uri);
void       egg_play_preview_set_uri      (EggPlayPreview *play_preview,
										  const gchar *uri);
void       egg_play_preview_set_position (EggPlayPreview *play_preview,
										  gint position);
gchar     *egg_play_preview_get_uri      (EggPlayPreview *play_preview);
gchar     *egg_play_preview_get_title    (EggPlayPreview *play_preview);
gchar     *egg_play_preview_get_artist   (EggPlayPreview *play_preview);
gchar     *egg_play_preview_get_album    (EggPlayPreview *play_preview);
gint       egg_play_preview_get_position (EggPlayPreview *play_preview);
gint       egg_play_preview_get_duration (EggPlayPreview *play_preview);

G_END_DECLS

#endif /* __EGG_PLAY_PREVIEW_H__ */
