/*
 * GTKFILECHOOSER for CINELERRA-CV
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

#include "gtkfilechooser.h"
#include "loadmode.inc"
#include "mwindow.inc"
#include <libgen.h>
#include <pwd.h>


GwFileChooser::GwFileChooser()
{
	int fakeargc = 1;
	fakeargv = new char*[1];
	fakeargv[0] = new char [strlen("cinelerra-cv") + 1];
	strcpy(fakeargv[0], "cinelerra-cv");
#ifdef HAVE_GTKMM30
	gtk_wrapper = Gtk::Application::create(fakeargc, fakeargv, "cinelerra-cv.org");
	gtk_wrapper->release();
#else
	gtk_wrapper = new Gtk::Main(fakeargc, fakeargv, false);
#endif
}


GwFileChooser::~GwFileChooser()
{
#ifdef HAVE_GTKMM30
	gtk_wrapper->quit();
	gtk_wrapper.clear();
#else
	printf("\n  Starting Gtk Flush: ");
	int flush = 0;
	while(gtk_wrapper->events_pending())
	{
		gtk_wrapper->iteration(false);
		//Gdk::flush();
		flush++;
		printf("%d.", flush);
	}
	delete gtk_wrapper;
	printf(" quit\n");
#endif
	delete [] fakeargv[0];
	delete [] fakeargv;
}

GwFileChooserGui::GwFileChooserGui()
{
	pdialog = 0;
	// Identify wrapper as cinelerra
	dummy = new Gtk::Window;
	dummy->set_title("If you can see this window something went wrong");
	dummy->set_default_size(550, 20);
	dummy->set_can_default(true);
	dummy->iconify();
}

GwFileChooserGui::~GwFileChooserGui()
{
	dummy->show();
#ifdef HAVE_GTKMM30
	dummy->close();
#else
	dummy->hide();
#endif
	delete dummy;
}

int GwFileChooser::loadfiles(ArrayList<char*> &path_list,
		int &load_mode,
		char *default_path,
		int &filter)
{
	path_list.set_array_delete();
	std::vector<std::string> filenames;
	bool have_path = 0;
	int retval = 0;
	int result = 0;

	char *dirname_spot = 0;
	// Cinelerra not saves the last dir as default_path suggests
	// but only the last file as last dir.;
	struct stat s;
	if( stat(default_path, &s) == 0 )
	{
		if(strcmp(default_path, "~"))
		{
			if(S_ISREG(s.st_mode))
			{
				dirname_spot = new char[strlen(default_path) + 1];
				strcpy(dirname_spot, default_path);
				dirname(dirname_spot);
			}
			else if(strlen(default_path) < 3)
			{
				// default to home
				struct passwd *pw = getpwuid(getuid());
				dirname_spot = new char[strlen(pw->pw_dir) + 1];
				strcpy(dirname_spot, pw->pw_dir);
			}
			else
			{
				dirname_spot = new char[strlen(default_path) + 1];
				strcpy(dirname_spot, default_path);
			}

		}
	}
	else
	{
		// If no stat default to home
		struct passwd *pw = getpwuid(getuid());
		dirname_spot = new char[strlen(pw->pw_dir) + 1];
		strcpy(dirname_spot, pw->pw_dir);
	}

	GwFileChooserGui loadgui;
	loadgui.do_load_dialogs(filenames, dirname_spot, load_mode, filter, result);
	bool altclean = false;
	if(!filenames.empty())
	{
		strcpy(default_path, (char*)filenames[0].c_str());
		char *out_path;
		int i;
		int z;
		if(result != LOAD_PASTE) z = filenames.size();
			else z = 1;
		for(i = 0; i < z; i++)
		{
			char in_path[filenames[i].size()];
			strcpy(in_path, (char*)filenames[i].c_str());
			if(( stat(in_path, &s) == 0 ) && (S_ISDIR(s.st_mode))) retval = 1;
			int j;
			for(j = 0; j < path_list.total; j++)
			{
				if(!strcmp(in_path, path_list.values[j])) break;
			}

			if(j == path_list.total)
			{
				path_list.append(out_path = new char[strlen(in_path) + 1]);
				strcpy(out_path, in_path);
			}
		}
	}
	else
	{
		strcpy(default_path, dirname_spot);
		retval = 1;
	}

	int z = filenames.size();
	for(int i = 0; i < z; i++) filenames[i].clear();

	if(!filenames.empty()) filenames.clear();
	delete [] dirname_spot;

	if(result == 0) retval = 1;
	else load_mode = result;

	return retval;
}

void GwFileChooserGui::do_load_dialogs(std::vector<std::string> &filenames,
		char *default_path,
		int &load_mode,
		int &filter,
		int &result)
{
	Gtk::FileChooserDialog dialog("Please, choose one or more files then press one insertion strategy",
			Gtk::FILE_CHOOSER_ACTION_OPEN);
	dialog.set_transient_for(*dummy);
	dialog.set_parent(*dummy);
	//Add response buttons the the dialog:
	dialog.add_button("_Replace", LOAD_REPLACE);
	dialog.add_button("Repl+Conc", LOAD_REPLACE_CONCATENATE);
	dialog.add_button("_Concatenate", LOAD_CONCATENATE);
	dialog.add_button("_New Tracks", LOAD_NEW_TRACKS);
	dialog.add_button("_As Resource", LOAD_RESOURCESONLY);
	dialog.add_button("_Paste on", LOAD_PASTE);
	dialog.add_button("Cancel", Gtk::RESPONSE_CANCEL);
	dialog.set_default_response(load_mode);
	//Add filters, so that only certain file types can be selected:
#ifdef HAVE_GTKMM30
	Glib::RefPtr<Gtk::FileFilter> filter_xml = Gtk::FileFilter::create();
	Glib::RefPtr<Gtk::FileFilter> filter_video = Gtk::FileFilter::create();
	Glib::RefPtr<Gtk::FileFilter> filter_audio = Gtk::FileFilter::create();
	Glib::RefPtr<Gtk::FileFilter> filter_images = Gtk::FileFilter::create();
	Glib::RefPtr<Gtk::FileFilter> filter_any = Gtk::FileFilter::create();

	filter_xml->set_name("Project");
	filter_xml->add_pattern("*.xml");

	filter_video->set_name("Video");
	filter_video->add_mime_type("video/*");

	filter_audio->set_name("Audio");
	filter_audio->add_mime_type("audio/*");

	filter_images->set_name("Images");
	filter_images->add_mime_type("image/*");

	filter_any->set_name("Any files");
	filter_any->add_pattern("*");
#else
	Gtk::FileFilter filter_xml;
	Gtk::FileFilter filter_video;
	Gtk::FileFilter filter_audio;
	Gtk::FileFilter filter_images;
	Gtk::FileFilter filter_any;

	filter_xml.set_name("Project");
	filter_xml.add_pattern("*.xml");

	filter_video.set_name("Video");
	filter_video.add_mime_type("video/*");

	filter_audio.set_name("Audio");
	filter_audio.add_mime_type("audio/*");

	filter_images.set_name("Images");
	filter_images.add_mime_type("image/*");

	filter_any.set_name("Any files");
	filter_any.add_pattern("*");
#endif

	dialog.add_filter(filter_xml);
	dialog.add_filter(filter_video);
	dialog.add_filter(filter_audio);
	dialog.add_filter(filter_images);
	dialog.add_filter(filter_any);

	dialog.set_current_folder(default_path);
	if(filter == 1) dialog.set_filter(filter_xml);
	if(filter == 2) dialog.set_filter(filter_video);
	if(filter == 3) dialog.set_filter(filter_audio);
	if(filter == 4) dialog.set_filter(filter_images);
	if(filter == 5) dialog.set_filter(filter_any);
#ifdef HAVE_GTKMM30
	dialog.resize_to_geometry(400,400);
#else
	dialog.set_size_request(400,400);
#endif
	dialog.set_select_multiple(1);
	std::string path = dialog.get_current_folder();
	pdialog = &dialog;
	dialog.set_preview_widget(preview);
	dialog.signal_update_preview().connect(sigc::mem_fun(*this,
			&GwFileChooserGui::update_preview_cb));

	//Show the dialog and wait for a user response:
	result = dialog.run();
	pdialog = NULL;
	//Handle the response:
	filenames = dialog.get_filenames();
#ifdef HAVE_GTKMM30
	if(dialog.get_filter() == filter_xml) filter=1;
	if(dialog.get_filter() == filter_video) filter=2;
	if(dialog.get_filter() == filter_audio) filter=3;
	if(dialog.get_filter() == filter_images) filter=4;
	if(dialog.get_filter() == filter_any) filter=5;
#else
	if(!dialog.get_filter()->get_name().compare(filter_xml.get_name())) filter=1;
	if(!dialog.get_filter()->get_name().compare(filter_video.get_name())) filter=2;
	if(!dialog.get_filter()->get_name().compare(filter_audio.get_name())) filter=3;
	if(!dialog.get_filter()->get_name().compare(filter_images.get_name())) filter=4;
	if(!dialog.get_filter()->get_name().compare(filter_any.get_name())) filter=5;
#endif
	switch(result)
	{
	case LOAD_REPLACE:
		load_mode = LOAD_REPLACE;
		break;
	case LOAD_REPLACE_CONCATENATE:
		load_mode = LOAD_REPLACE_CONCATENATE;
		break;
	case LOAD_CONCATENATE:
		load_mode = LOAD_CONCATENATE;
		break;
	case LOAD_NEW_TRACKS:
		load_mode = LOAD_NEW_TRACKS;
		break;
	case LOAD_RESOURCESONLY:
		load_mode = LOAD_RESOURCESONLY;
		break;
	case LOAD_PASTE:
		load_mode = LOAD_PASTE;
		break;
	default:
	{
		result = 0;
		break;
	}
	}
}

void GwFileChooserGui::update_preview_cb()
{
	const char* filename;
	Glib::RefPtr<Gdk::Pixbuf> pixbuf;
	gboolean have_preview;

	preview.clear();

	std::string cast = pdialog->get_preview_filename();

	filename = cast.c_str();

	pixbuf = Gdk::Pixbuf::create_from_file(filename, 300, 300, true);
	//Gdk::Pixbuf::create_from_stream_at_scale(filename, 300, 300, true);
	have_preview = pixbuf.operator bool();

	preview.set(pixbuf);

	pdialog->set_preview_widget_active(have_preview);


}
