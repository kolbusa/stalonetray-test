/* ************************************
 * vim:tabstop=4:shiftwidth=4
 * xutils.c
 * Sun, 05 Mar 2006 17:56:56 +0600
 * ************************************
 * misc X11 utilities
 * ************************************/

#include "config.h"

#include <X11/Xmd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <unistd.h>

#include "xutils.h"

#include "xembed.h"
#include "debug.h"
#include "common.h"

/* error handling code taken from xembed spec */
int trapped_error_code = 0;
static int (*old_error_handler) (Display *, XErrorEvent *);

 
static int error_handler(Display *dpy, XErrorEvent *error)
{
	static char buffer[1024];
#ifdef DEBUG
	XGetErrorText(dpy, error->error_code, buffer, sizeof(buffer) - 1);
	DBG(0, ("error: %s\n", buffer));
#endif
	trapped_error_code = error->error_code;
	return 0;
}

void trap_errors()
{
	trapped_error_code = 0;
	old_error_handler = XSetErrorHandler(error_handler);
	DBG(9, ("old_error_handler = %p\n", old_error_handler));
}

int untrap_errors(Display *dpy)
{
#ifdef DEBUG
	/* in order to catch up with errors in async mode 
	 * XXX: this slows things down...*/
	XSync(dpy, False);
#endif
	XSetErrorHandler(old_error_handler);
	return trapped_error_code;
}

int wait_for_event_helper(Display *dpy, XEvent *ev, XPointer data)
{
	/* XAnyEvent does not suffice :( */

	DBG(9, ("serial: %u\n", ev->xany.serial));

	if ((ev->type == ((struct EventData *)data)->type || ev->type == DestroyNotify)
	    && ev->xmap.window == ((struct EventData *)data)->w)
	{
		return True;
	}

	return False;
}


int wait_for_event_serial(Window w, int type, unsigned long *serial)
{
	XEvent ev;
	struct EventData ed;

	ed.type = type;
	ed.w = w;

	DBG(6, ("type = %d, window = 0x%x\n", type, w));

	XPeekIfEvent(tray_data.dpy, &ev, &wait_for_event_helper, (XPointer) &ed);

	DBG(9, ("serial = %u\n", ev.xany.serial));

	if (ev.type == type) {
		*serial = ev.xany.serial;
		return SUCCESS;
	}
	else
		return FAILURE;
}

int wait_for_event(Window w, int type)
{
	unsigned long serial = 0;
	return wait_for_event_serial(w, type, &serial);
}

static Window timestamp_wnd;
static Atom timestamp_atom = None;

Bool wait_timestamp(Display *dpy, XEvent *xevent, XPointer data)
{
	Window wnd = *(Window *)data;

	if (xevent->type == PropertyNotify &&
	    xevent->xproperty.window == wnd &&
		xevent->xproperty.atom == timestamp_atom)
	{
		return True;
	}

	return False;
}

Time get_server_timestamp(Display *dpy, Window wnd)
{
	unsigned char c = 's';
	XEvent xevent;

	if (timestamp_atom == None) 
		timestamp_atom = XInternAtom(dpy, "STALONETRAY_TIMESTAMP", False);
		
	XChangeProperty(dpy, wnd, timestamp_atom, timestamp_atom, 8, PropModeReplace, &c, 1);

	timestamp_wnd = wnd;
	XIfEvent(dpy, &xevent, wait_timestamp, (XPointer)&timestamp_wnd);

	return xevent.xproperty.time;
}

int send_client_msg32(Display *dpy, Window dst, Window wnd, Atom type, long data0, long data1, long data2, long data3, long data4)
{
	XEvent ev;
	ev.xclient.type = ClientMessage;
	ev.xclient.serial = 0;
	ev.xclient.send_event = True;
	ev.xclient.message_type = type;
	ev.xclient.window = wnd;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = data0;
	ev.xclient.data.l[1] = data1;
	ev.xclient.data.l[2] = data2;
	ev.xclient.data.l[3] = data3;
	ev.xclient.data.l[4] = data4;
/*	return XSendEvent(dpy, dst, False, SubstructureRedirectMask | SubstructureNotifyMask | StructureNotifyMask, &ev);*/
	return XSendEvent(dpy, dst, False, 0xFFFFFF, &ev);
}

int icon_repaint(struct TrayIcon *ti)
{
	XEvent xe;

	trap_errors();
	
	XClearWindow(tray_data.dpy, ti->l.p);

	XClearWindow(tray_data.dpy, ti->w);

	xe.type = VisibilityNotify;

	xe.xvisibility.window = ti->w;
	xe.xvisibility.state = VisibilityFullyObscured;

	XSendEvent(tray_data.dpy, ti->w, True, NoEventMask, &xe); 

	xe.xvisibility.state = VisibilityUnobscured;

	XSendEvent(tray_data.dpy, ti->w, True, NoEventMask, &xe); 

	xe.type = Expose;
	xe.xexpose.window = ti->w;
	xe.xexpose.x = 0;
	xe.xexpose.y = 0;
	xe.xexpose.width = ti->l.wnd_sz.x;
	xe.xexpose.height = ti->l.wnd_sz.y;
	xe.xexpose.count = 0;

	XClearWindow(tray_data.dpy, ti->w);
		
	XSendEvent(tray_data.dpy, ti->w, True, NoEventMask, &xe); 

	if (untrap_errors(tray_data.dpy)) 
		DBG(0, ("could not ask  0x%x for repaint (%d)\n", ti->w, trapped_error_code));

	return NO_MATCH;
}

int set_window_size(struct TrayIcon *ti, struct Point dim)
{
	XSizeHints xsh;
	
	xsh.flags = PSize;
	xsh.width = dim.x;
	xsh.height = dim.y;

	trap_errors();
	
	XSetWMNormalHints(tray_data.dpy, ti->w, &xsh);

	XResizeWindow(tray_data.dpy, ti->w, dim.x, dim.y);

	if (untrap_errors(tray_data.dpy)) {
		DBG(0, ("failed to force 0x%x size to (%d, %d) (error code %d)\n", 
				ti->w, dim.x, dim.y, trapped_error_code));
		ti->l.wnd_sz.x = 0;
		ti->l.wnd_sz.y = 0;
		return FAILURE;
	}
	ti->l.wnd_sz = dim;
	return SUCCESS;
}

#define NUM_GETSIZE_RETRIES	100
int get_window_size(struct TrayIcon *ti, struct Point *res)
{
	XWindowAttributes xwa;
	int atmpt = 0;

	res->x = FALLBACK_SIZE;
	res->y = FALLBACK_SIZE;
	
	for(;;) {
		trap_errors();

		XGetWindowAttributes(tray_data.dpy, ti->w, &xwa);
	
		if (untrap_errors(tray_data.dpy)) {
			DBG(0, ("failed to get 0x%x attributes (error code %d)\n", ti->w, trapped_error_code));
			return FAILURE;
		}
	
		res->x = xwa.width;
		res->y = xwa.height;

		if (res->x * res->y > 1) {
			DBG(4, ("success (%dx%d)\n", res->x, res->y));
			return SUCCESS;
		}

		if (atmpt++ > NUM_GETSIZE_RETRIES) {
			DBG(0, ("timeout waiting for 0x%x to specify its size, using fallback (%d,%d)\n", 
					ti->w, FALLBACK_SIZE, FALLBACK_SIZE));
			return SUCCESS;
		}
		usleep(500);
	};
}

/* from FVWM:fvwm/colorset.c:172 */
Pixmap get_root_pixmap(Atom prop)
{
	Atom type;
	int format;
	unsigned long length, after;
	unsigned char *reteval = NULL;
	int ret;
	Pixmap pix = None;
	Window root_window = XRootWindow(tray_data.dpy, DefaultScreen(tray_data.dpy));

	ret = XGetWindowProperty(tray_data.dpy, root_window, prop, 0L, 1L, False, XA_PIXMAP,
			   &type, &format, &length, &after, &reteval);
	if (ret == Success && type == XA_PIXMAP && format == 32 && length == 1 &&
	    after == 0)
	{
		pix = (Pixmap)(*(long *)reteval);
	}
	if (reteval)
	{
		XFree(reteval);
	}
	DBG(8, ("pixmap: 0x%x\n", pix));
	return pix;
}

/* from FVWM:fvwm/colorset.c:195 + optimized */
void update_root_pmap()
{
	static Atom xa_xrootpmap_id = None;
	Atom xa_rootpix;
	unsigned int w = 0, h = 0;
	XID dummy;
	Pixmap pix;
	
	if (xa_xrootpmap_id == None)
	{
		xa_xrootpmap_id = XInternAtom(tray_data.dpy,"_XROOTPMAP_ID", False);
	}
	XSync(tray_data.dpy, False);

	xa_rootpix = tray_data.xa_xrootpmap_id != None ? tray_data.xa_xrootpmap_id : xa_xrootpmap_id;
	
	pix = get_root_pixmap(tray_data.xa_xrootpmap_id);

	trap_errors();

	if (pix && !XGetGeometry(
		tray_data.dpy, pix, &dummy, (int *)&dummy, (int *)&dummy,
		&w, &h, (unsigned int *)&dummy, (unsigned int *)&dummy))
	{
		DBG(0, ("invalid background pixmap\n"));
		pix = None;
	}

	if (untrap_errors(tray_data.dpy)) {
		DBG(0, ("could not update root background pixmap(%d)\n", trapped_error_code));
		return;
	}
	
	tray_data.root_pmap = pix;
	tray_data.root_pmap_width = w;
	tray_data.root_pmap_height = h;
	
	DBG(4, ("got new root pixmap: 0x%lx (%ix%i)\n", tray_data.root_pmap, w, h));
}

int hide_window(struct TrayIcon *ti)
{
	trap_errors();

	DBG(6, ("hiding 0x%x\n", ti->w));
	
	XUnmapWindow(tray_data.dpy, ti->l.p);
	
	if (untrap_errors(tray_data.dpy)) 
		DBG(0, ("could not unmap mid-parent\n"));

	return SUCCESS;
}

int show_window(struct TrayIcon *ti)
{
	trap_errors();

	DBG(6, ("unhiding 0x%x\n", ti->w));

	icon_repaint(ti);
	
	XMapRaised(tray_data.dpy, ti->l.p);

	if (untrap_errors(tray_data.dpy)) 
		DBG(0, ("could not unmap mid-parent\n"));
	
	return SUCCESS;
}

int get_window_abs_coords(Display *dpy, Window dst, int *x, int *y)
{
	Window root, parent, *wjunk = NULL;
	int x_, y_, x__, y__;
	unsigned int junk;

	trap_errors();

	XGetGeometry(dpy, dst, &root, &x_, &y_, &junk, &junk, &junk, &junk);
	XQueryTree(dpy, dst, &root, &parent, &wjunk, &junk);

	if (junk != 0) XFree(wjunk);

	if (untrap_errors(dpy)) {
		return FAILURE;
	}

	if (parent == root) {
		*x = x_;
		*y = y_;
	} else {
		if (get_window_abs_coords(dpy, parent, &x__, &y__)) {
			*x = x_ + x__;
			*y = y_ + y__;
		} else
			return FAILURE;
	}
	
	return SUCCESS;
}

#ifdef DEBUG
/* XXX: this function should provide basic info
 * about the window, like 
 * 	- size
 * 	- position
 * 	- name */
void dump_win_info(Display *dpy, Window w)
{
}
#endif