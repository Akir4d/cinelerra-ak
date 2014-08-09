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
#include <libgen.h>

GtkFileChooserMain::GtkFileChooserMain()
{
	int fakeargc = 1;
	char **fakeargv;
	fakeargv = new char*[1];
	// Identify wrapper as cinelerra
	gtk_wrapper = Gtk::Application::create(fakeargc,fakeargv, "org.cinelerra-cv.gtkwrapper");

}


GtkFileChooserMain::~GtkFileChooserMain()
{
	// Yes, the way to exit gtk3 is an hack.
	Gtk::Window dummy;
	dummy.show();
	gtk_wrapper->quit();
}

GtkFileChooserGui::GtkFileChooserGui()
{
	pdialog = 0;
}

GtkFileChooserGui::~GtkFileChooserGui()
{
	//pdialog->unreference();
}

void GtkFileChooserGui::do_load_dialogs(std::vector<std::string> &filenames, char *default_path, int &load_mode, int &filter, int &result)
{
	Gtk::FileChooserDialog dialog("Please choose one or more file, then press one insertion strategy",
				Gtk::FILE_CHOOSER_ACTION_OPEN);
		dialog.set_transient_for(*this);
		//dialog.unreference();
		//pri = &dialog;

		//Add response buttons the the dialog:
		dialog.add_button("_Replace", LOAD_REPLACE);
		dialog.add_button("_Replace+", LOAD_REPLACE_CONCATENATE);
		dialog.add_button("_Concatenate", LOAD_CONCATENATE);
		dialog.add_button("_New Tracks", LOAD_NEW_TRACKS);
		dialog.add_button("_As Resource", LOAD_RESOURCESONLY);
		dialog.add_button("_Paste", LOAD_PASTE);
		dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);

		dialog.set_default_response(load_mode);
		//Add filters, so that only certain file types can be selected:

		Glib::RefPtr<Gtk::FileFilter> filter_xml = Gtk::FileFilter::create();
		filter_xml->set_name("Project");
		filter_xml->add_pattern("*.xml");
		dialog.add_filter(filter_xml);

		Glib::RefPtr<Gtk::FileFilter> filter_video = Gtk::FileFilter::create();
		filter_video->set_name("Video");
		filter_video->add_mime_type("video/*");
		dialog.add_filter(filter_video);

		Glib::RefPtr<Gtk::FileFilter> filter_audio = Gtk::FileFilter::create();
		filter_audio->set_name("Audio");
		filter_audio->add_mime_type("audio/*");
		dialog.add_filter(filter_audio);

		Glib::RefPtr<Gtk::FileFilter> filter_images = Gtk::FileFilter::create();
		filter_images->set_name("Images");
		filter_images->add_mime_type("image/*");
		dialog.add_filter(filter_images);

		Glib::RefPtr<Gtk::FileFilter> filter_any = Gtk::FileFilter::create();
		filter_any->set_name("Any files");
		filter_any->add_pattern("*");
		dialog.add_filter(filter_any);
		dialog.set_current_folder(default_path);
		if(filter == 1) dialog.set_filter(filter_xml);
		if(filter == 2) dialog.set_filter(filter_video);
		if(filter == 3) dialog.set_filter(filter_audio);
		if(filter == 4) dialog.set_filter(filter_images);
		if(filter == 5) dialog.set_filter(filter_any);

		dialog.resize_to_geometry(400,400);

		dialog.set_select_multiple(1);
		std::string path = dialog.get_current_folder();
		pdialog = &dialog;
		dialog.set_preview_widget(preview);
		dialog.signal_update_preview().connect(sigc::mem_fun(*this,
								&GtkFileChooserGui::update_preview_cb));
		dialog.signal_hide().emission_stop();

		//Show the dialog and wait for a user response:
		result = dialog.run();

		//Handle the response:
		filenames = dialog.get_filenames();

		if(dialog.get_filter() == filter_xml) filter=1;
		if(dialog.get_filter() == filter_video) filter=2;
		if(dialog.get_filter() == filter_audio) filter=3;
		if(dialog.get_filter() == filter_images) filter=4;
		if(dialog.get_filter() == filter_any) filter=5;
}

int GtkFileChooserMain::loadfiles(ArrayList<char*> &path_list,
		int &load_mode,
		char *default_path,
		int &filter)
{
	path_list.set_array_delete();
	std::vector<std::string> filenames;
	bool have_path = 0;
	int retval = 1;
	int result = 0;
	char dirname_spot[strlen(default_path) + 1];
	// Cinelerra not saves the last dir as default_path suggests
	// but only the last file as last dir.
	printf("\ndefault_path: %s\n", default_path);
	strcpy(dirname_spot, default_path);
	struct stat s;
	if( stat(dirname_spot, &s) == 0 )
	{
		if(S_ISREG(s.st_mode))
		{
			dirname(dirname_spot);
		}

	}
	GtkFileChooserGui loadthread;
	loadthread.do_load_dialogs(filenames, dirname_spot, load_mode, filter, result);

			char *out_path;
			int i;
			int z = filenames.size();
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
			filenames.clear();


			switch(result)
			{
			case LOAD_REPLACE:
			case LOAD_REPLACE_CONCATENATE:
			case LOAD_CONCATENATE:
			case LOAD_NEW_TRACKS:
			case LOAD_RESOURCESONLY:
			case LOAD_PASTE:
			{
				load_mode = result;
				retval = 0;
				break;
			}
			default:
			{
				retval = 1;
				break;
			}
			}
	return retval;
}


void GtkFileChooserGui::update_preview_cb()
{
	const char* filename;
	Glib::RefPtr<Gdk::Pixbuf> pixbuf;
	gboolean have_preview;

	pixbuf.clear();
	preview.clear();

	std::string cast = pdialog->get_preview_filename();

	filename = cast.c_str();

	pixbuf = Gdk::Pixbuf::create_from_file(filename, 300, 300, true);
	have_preview = pixbuf.operator bool();

	preview.set(pixbuf);

	pdialog->set_preview_widget_active(have_preview);

}

