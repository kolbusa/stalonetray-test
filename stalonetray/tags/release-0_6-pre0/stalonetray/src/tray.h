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
#include "xembed.h"

#define PROGNAME PACKAGE

/* from System Tray Protocol Specification 
 * http://freedesktop.org/Standards/systemtray-spec/systemtray-spec-0.2.html */
#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2
#define STALONE_TRAY_DOCK_CONFIRMED 0xFFFF

#define	TRAY_SEL_ATOM "_NET_SYSTEM_TRAY_S"

#define TRAY_ORIENTATION_ATOM "_NET_SYSTEM_TRAY_ORIENTATION" 

#define _NET_SYSTEM_TRAY_ORIENTATION_HORZ 0
#define _NET_SYSTEM_TRAY_ORIENTATION_VERT 1

struct TrayData {
	Window tray;

	Display *dpy;
	XSizeHints xsh;

	Window old_sel_owner;

	int active;

	int grow_freeze;
	int grow_issued;

	Atom xa_tray_selection;
	Atom xa_tray_opcode;
	Atom xa_tray_data;
	Atom xa_wm_protocols;
	Atom xa_wm_delete_window;
	Atom xa_wm_take_focus;
	Atom xa_kde_net_system_tray_windows;

	Atom xa_xrootpmap_id;
	Atom xa_xsetroot_id;
	Pixmap root_pmap;

	unsigned int root_pmap_width;
	unsigned int root_pmap_height;

	struct XEMBEDData xembed_data;
};

extern struct TrayData tray_data;

#define tray_grow_check(dsz) \
	(dsz.x >= 0 && dsz.y >= 0 && \
	(!dsz.x || (settings.grow_gravity & GRAV_H &&\
				(!settings.max_tray_width || \
				settings.max_tray_width >= tray_data.xsh.width + dsz.x))) && \
	(!dsz.y || (settings.grow_gravity & GRAV_V && \
				(!settings.max_tray_height || \
				settings.max_tray_height >= tray_data.xsh.height + dsz.y))))
#define FALLBACK_SIZE	24

void tray_init_data();
void tray_create_window(int argc, char **argv);
void tray_acquire_selection();
void tray_show();

int tray_grow(struct Point dsz);

int tray_update_bg();
int tray_update_size_hints();
int tray_update_wm_hints();

#endif