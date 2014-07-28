/*
 * GTKWRAPPER for CINELERRA-CV
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
#include <gtkmm.h>
#include "gtkwrapper.h"
#include "gtkfilechooser.h"
#include "arraylist.h"

GtkWrapper::GtkWrapper()
{
}

GtkWrapper::~GtkWrapper()
{
}

void GtkWrapper::init(int argc, char* argv[])
{
}

int GtkWrapper::loadfiles_wrapper(ArrayList<char*> &path_list,
		int loadmodein,
		int &loadmodeout,
		char* path_defaultin,
		char* path_defaultout,
		int filterin,
		int &filterout)
{
	int fakeargc = 1;
	char **fakeargv;
	fakeargv = new char*[1];
	//Init Gtk_wrapper
	Glib::RefPtr<Gtk::Application> gtk_wrapper;

	//Identify wrapper as cinelerra
	gtk_wrapper = Gtk::Application::create(fakeargc,fakeargv, "org.cinelerra-cv.gtkwrapper");

	int returnval = 0;
	if(gtk_wrapper)
	{
		//Because Gtk::Application::create starting with a main window, first release it.
		gtk_wrapper->release();

		GtkFileChooserWindow loadwindow;
		returnval = loadwindow.loadfiles(path_list,
				loadmodein,
				loadmodeout,
				path_defaultin,
				path_defaultout,
				filterin,
				filterout);

		gtk_wrapper->run(loadwindow);
		gtk_wrapper->release();
		gtk_wrapper->quit();

	}
	return returnval;
}
