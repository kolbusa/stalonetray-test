/* -------------------------------
 * vim:tabstop=4:shiftwidth=4
 * tray.h
 * Wed, 29 Sep 2004 23:10:02 +0700
 * -------------------------------
 * Common tray routines
 * -------------------------------*/

#ifndef _TRAY_H_
#define _TRAY_H_

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "icons.h"

int tray_set_constraints(int width, int height);
int tray_grow(struct Point dsz);

#define tray_grow_check(dsz) \
	(dsz.x >= 0 && dsz.y >= 0 && \
	(!dsz.x || (settings.grow_gravity & GRAV_H &&\
				(!settings.max_tray_width || \
				settings.max_tray_width >= tray_data.xsh.width + dsz.x))) && \
	(!dsz.y || (settings.grow_gravity & GRAV_V && \
				(!settings.max_tray_height || \
				settings.max_tray_height >= tray_data.xsh.height + dsz.y))))

typedef struct {
	Window tray;
	int reparented;
	Window parent;

	Display *dpy;
	XSizeHints xsh;

	int grow_freeze;
	int grow_issued;

	Atom xa_tray_selection;
	Atom xa_tray_opcode;
	Atom xa_tray_data;
	Atom xa_wm_delete_window;
	Atom xa_wm_protocols;
	Atom xa_wm_take_focus;
	Atom xa_xembed;

	Atom xa_xrootpmap_id;
	Atom xa_xsetroot_id;
	Pixmap root_pmap;
	unsigned int root_pmap_width;
	unsigned int root_pmap_height;
} TrayData;

extern TrayData tray_data;

#endif