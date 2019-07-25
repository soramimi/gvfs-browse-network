/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright(C) 2006-2007 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or(at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: David Zeuthen <davidz@redhat.com>
 */

// modified 2019 S.Fuchita (@soramimi_jp)

#include <glib.h>
#include <locale.h>
#include <string.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

static gboolean show_hidden = FALSE;
static gboolean follow_symlinks = FALSE;
static gboolean show_version = FALSE;

static gint sort_info_by_name(GFileInfo *a, GFileInfo *b)
{
	const char *na;
	const char *nb;

	na = g_file_info_get_name(a);
	nb = g_file_info_get_name(b);

	if (na == NULL) na = "";
	if (nb == NULL) nb = "";

	return strcmp(na, nb);
}

static void do_tree(GFile *f, int level, guint64 pattern)
{
	GFileEnumerator *enumerator;
	GError *error = NULL;
	unsigned int n;
	GFileInfo *info;

	info = g_file_query_info(f,
							 G_FILE_ATTRIBUTE_STANDARD_TYPE ","
							 G_FILE_ATTRIBUTE_STANDARD_TARGET_URI,
							 0,
							 NULL, NULL);
	if (info) {
		if (g_file_info_get_attribute_uint32(info, G_FILE_ATTRIBUTE_STANDARD_TYPE) == G_FILE_TYPE_MOUNTABLE) {
			/* don't process mountables; we avoid these by getting the target_uri below */
			g_object_unref(info);
			return;
		}
		g_object_unref(info);
	}

	enumerator = g_file_enumerate_children(f,
										   G_FILE_ATTRIBUTE_STANDARD_NAME ","
										   G_FILE_ATTRIBUTE_STANDARD_TYPE ","
										   G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN ","
										   G_FILE_ATTRIBUTE_STANDARD_IS_SYMLINK ","
										   G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET ","
										   G_FILE_ATTRIBUTE_STANDARD_TARGET_URI ","
										   G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
										   0,
										   NULL,
										   &error);
	if (enumerator) {
		GList *l;
		GList *info_list;

		info_list = NULL;
		while ((info = g_file_enumerator_next_file(enumerator, NULL, NULL))) {
			if (g_file_info_get_is_hidden(info) && !show_hidden) {
				g_object_unref(info);
			} else {
				info_list = g_list_prepend(info_list, info);
			}
		}
		g_file_enumerator_close(enumerator, NULL, NULL);

		info_list = g_list_sort(info_list,(GCompareFunc) sort_info_by_name);

		for (l = info_list; l; l = l->next) {
			const char *name;
			const char *disp;
			const char *target_uri;
			GFileType type;
			gboolean is_last_item;

			info = l->data;
			is_last_item = (l->next == NULL);

			name = g_file_info_get_name(info);
			disp = g_file_info_get_display_name(info);
			type = g_file_info_get_attribute_uint32(info, G_FILE_ATTRIBUTE_STANDARD_TYPE);
			if (name) {

				g_print("%s", name);

				target_uri = g_file_info_get_attribute_string(info, G_FILE_ATTRIBUTE_STANDARD_TARGET_URI);
				if (target_uri) {
					g_print(" : %s", target_uri);
				} else {
					if (g_file_info_get_is_symlink(info)) {
						const char *target;
						target = g_file_info_get_symlink_target(info);
						g_print("%s", target);
					}
				}

				g_print(" : %s", disp);

				g_print("\n");

				if ((type & G_FILE_TYPE_DIRECTORY) && (follow_symlinks || !g_file_info_get_is_symlink(info))) {
					guint64 new_pattern;
					GFile *child;

					if (is_last_item)
						new_pattern = pattern;
					else
						new_pattern = pattern | (1 << level);

					child = NULL;
					if (target_uri) {
						if (follow_symlinks) {
							child = g_file_new_for_uri(target_uri);
						}
					} else {
						child = g_file_get_child(f, name);
					}

					if (child) {
						do_tree(child, level + 1, new_pattern);
						g_object_unref(child);
					}
				}
			}
			g_object_unref(info);
		}
		g_list_free(info_list);
	} else {
		g_print("[%s]\n", error->message);

		g_error_free(error);
	}
}

static void tree(GFile *f)
{
	char *uri;

	uri = g_file_get_uri(f);
	g_print("%s\n", uri);
	g_free(uri);

	do_tree(f, 0, 0);
}

int main()
{
	GFile *file;

	setlocale (LC_ALL, "");

	file = g_file_new_for_commandline_arg("network:///");
	tree(file);

	return 0;
}
