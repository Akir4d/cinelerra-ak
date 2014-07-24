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
// Gtk Wrapper has to identify itself as cinelerra and
// so it has to read argc and argv as main.C does.
// This solution at this moment is best to prevent
// random gtk crash and freeze, also remove
// lot of warning form gtkmm.
// Dirty job to have fixed link to argc, argv
#ifndef FIXEDARG
#define FIXEDARG
int fixedargc;
char **fixedargv;
#endif

GtkWrapper::GtkWrapper()
{
}

GtkWrapper::~GtkWrapper()
{
}

void GtkWrapper::init(int argc, char* argv[])
{
	fixedargc = argc;
	fixedargv = argv;
}

int GtkWrapper::loadfiles_wrapper(std::vector<std::string> &filenames,
		int loadmodein,
		int &loadmodeout,
		char* path_defaultin,
		int filterin,
		int &filterout)
{
	//Init Gtk_wrapper
	Glib::RefPtr<Gtk::Application> gtk_wrapper;

	//Identify wrapper as cinelerra
	gtk_wrapper = Gtk::Application::create(fixedargc,fixedargv, "org.cinelerra-cv");

	// This is an alternative line to not do argc and argv dirty joke
	//gtk_wrapper = Gtk::Application::create ("cinelerra-cv",Gio::APPLICATION_FLAGS_NONE);

	int returnval = 0;
	if(gtk_wrapper)
	{
		//Because Gtk::Application::create starting with a main window, first release it.
		gtk_wrapper->release();

		GtkFileChooserWindow loadwindow;
		returnval = loadwindow.loadfiles(filenames,
				loadmodein,
				loadmodeout,
				path_defaultin,
				filterin,
				filterout);

		//Now run gui
		gtk_wrapper->run(loadwindow);

		//if not already exited
		if(!returnval)
		{
			//Be sure that gtk_wrapper exit
			loadwindow.close();
			if(gtk_wrapper) gtk_wrapper->release();

			//Now can quit safely
			if(gtk_wrapper) gtk_wrapper->quit();
		}
		else
		{
			returnval = 1;
		}
	}
	return returnval;
}
