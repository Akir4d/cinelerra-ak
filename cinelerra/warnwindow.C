
/*
 * CINELERRA warning window based on tooltip window code
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * Copyright (C) 2014 Paolo Rampino <akir4d at gmail dot com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "keys.h"
#include "language.h"
#include "mainsession.h"
#include "mwindow.h"
#include "preferences.h"
#include "theme.h"
#include "warnwindow.h"





WarnWindow::WarnWindow(MWindow *mwindow, char *set_warn, int slotn)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	this->s_warn = set_warn;
	this->nslot = slotn;
}

BC_Window* WarnWindow::new_gui()
{
	BC_DisplayInfo display_info;
	int x = display_info.get_abs_cursor_x();
	int y = display_info.get_abs_cursor_y();
	WarnWindowGUI *gui = this->gui = new WarnWindowGUI(mwindow,
		this,
		x,
		y,
		s_warn,
		nslot);
	gui->create_objects();
	return gui;
}

WarnWindowGUI::WarnWindowGUI(MWindow *mwindow,
	WarnWindow *thread,
	int x,
	int y,
	char *g_warn,
	int nslot)
 : BC_Window(PROGRAM_NAME ": Warning",
 	x,
	y,
 	500,
	150,
	500,
	150,
	0,
	0,
	1)
{
	this->mwindow = mwindow;
	this->thread = thread;
	this->get_warn = g_warn;
	this->slotn = nslot;
}

void WarnWindowGUI::create_objects()
{
	int x = 10, y = 10;
SET_TRACE
	add_subwindow(warn_text = new BC_Title(x, y, get_warn));
	y = get_h() - 30;
SET_TRACE
	BC_CheckBox *checkbox; 
	add_subwindow(checkbox = new WarnDisable(mwindow, this, x, y, slotn));
SET_TRACE
	BC_Button *button;
	y = get_h() - WarnClose::calculate_h(mwindow) - 10;
	x = get_w() - WarnClose::calculate_w(mwindow) - 10;
	add_subwindow(button = new WarnClose(mwindow, this, x, y));
SET_TRACE
	x += button->get_w() + 10;

	show_window();
	raise_window();
}

int WarnWindowGUI::keypress_event()
{
	switch(get_keypress())
	{
		case RETURN:
		case ESC:
		case 'w':
			set_done(0);
			break;
	}
	return 0;
}

WarnDisable::WarnDisable(MWindow *mwindow, WarnWindowGUI *gui, int x, int y, int slotn)
 : BC_CheckBox(x, 
 	y, 
 	slotn, _("Show this warning again."))
{
	this->mwindow = mwindow;
	this->gui = gui;
	this->nslot = slotn;
}

int WarnDisable::handle_event()
{
	switch(nslot)
	{
	case 1:
		mwindow->preferences->warning_slot1 = get_value();
		break;
	case 2:
		mwindow->preferences->warning_slot2 = get_value();
		break;
	case 3:
		mwindow->preferences->warning_slot2 = get_value();
		break;
	}
	printf("\n %d %d\n", nslot, get_value());
	return 1;
}

WarnClose::WarnClose(MWindow *mwindow, WarnWindowGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("close_tip"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Close"));
}

int WarnClose::handle_event()
{
	gui->set_done(0);
	return 1;
}

int WarnClose::calculate_w(MWindow *mwindow)
{
	return mwindow->theme->get_image_set("close_tip")[0]->get_w();
}
int WarnClose::calculate_h(MWindow *mwindow)
{
	return mwindow->theme->get_image_set("close_tip")[0]->get_h();
}



