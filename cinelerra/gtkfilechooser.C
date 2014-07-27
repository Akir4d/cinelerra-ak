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

GtkFileChooserWindow::GtkFileChooserWindow()
{
}


GtkFileChooserWindow::~GtkFileChooserWindow()
{
}


int GtkFileChooserWindow::loadfiles(ArrayList<char*> &path_list,
		int loadmodein,
		int &loadmodeout,
		char* path_defaultin,
		char* path_defaultout,
		int filterin,
		int &filterout)
{
	path_list.set_array_delete();
	std::vector<std::string> filenames;
	bool have_path = 0;

	Gtk::FileChooserDialog dialog("Please choose one or more file, then press one insertion strategy",
			Gtk::FILE_CHOOSER_ACTION_OPEN);

	dialog.set_transient_for(*this);

	int retval = 1;

	//Add response buttons the the dialog:
	dialog.add_button("_Replace", LOAD_REPLACE);
	dialog.add_button("_Replace+", LOAD_REPLACE_CONCATENATE);
	dialog.add_button("_Concatenate", LOAD_CONCATENATE);
	dialog.add_button("_New Tracks", LOAD_NEW_TRACKS);
	dialog.add_button("_As Resource", LOAD_RESOURCESONLY);
	dialog.add_button("_Paste", LOAD_PASTE);
	dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);

	dialog.set_default_response(loadmodein);
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
	dialog.set_current_folder(path_defaultin);
	printf("\n %s \n", path_defaultin);
	if(filterin == 1) dialog.set_filter(filter_xml);
	if(filterin == 2) dialog.set_filter(filter_video);
	if(filterin == 3) dialog.set_filter(filter_audio);
	if(filterin == 4) dialog.set_filter(filter_images);
	if(filterin == 5) dialog.set_filter(filter_any);

	dialog.resize_to_geometry(400,400);

	dialog.set_select_multiple(1);
	std::string path = dialog.get_current_folder();
/*FixMe preview
	Gtk::Image preview;

	dialog.set_preview_widget(preview);
	g_signal_connect(GTK_DIALOG((&dialog)), "update_preview", G_CALLBACK(update_preview_cb), GTK_IMAGE((&preview)));
*/
	//Show the dialog and wait for a user response:
	int result = dialog.run();

	//Handle the response:
	filenames = dialog.get_filenames();

	char dirname_spot[filenames[0].size()];

			strcpy(dirname_spot, (char*)filenames[0].c_str());
			struct stat s;
			if( stat(dirname_spot, &s) == 0 )
			{
				if(S_ISDIR(s.st_mode))
				{
					strcpy(path_defaultout, dirname_spot);
					retval = 1;
				}
				else if(S_ISREG(s.st_mode))
				{
					dirname(dirname_spot);
					strcpy(path_defaultout, dirname_spot);
				}
				else
				{
					retval = 1;
				}

			}

			char *out_path;
			int i;
			int z = filenames.size();
			for(i = 0; i < z; i++)
			{
				char in_path[filenames[i].size()];
				strcpy(in_path, (char*)filenames[i].c_str());

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

	if(dialog.get_filter() == filter_xml) filterout=1;
	if(dialog.get_filter() == filter_video) filterout=2;
	if(dialog.get_filter() == filter_audio) filterout=3;
	if(dialog.get_filter() == filter_images) filterout=4;
	if(dialog.get_filter() == filter_any) filterout=5;

	switch(result)
	{
	case LOAD_REPLACE:
	case LOAD_REPLACE_CONCATENATE:
	case LOAD_CONCATENATE:
	case LOAD_NEW_TRACKS:
	case LOAD_RESOURCESONLY:
	case LOAD_PASTE:
	{
		loadmodeout = result;
		retval = 0;
		break;
	}
	default:
	{
		loadmodeout = loadmodein;
		retval = 1;
		break;
	}
	}
	return retval;
}

/*
void GtkFileChooserWindow::update_preview_cb(Gtk::FileChooserDialog dialog, Gtk::Image preview)
{
	const char* filename;
	Glib::RefPtr<Gdk::Pixbuf> pixbuf;
	gboolean have_preview;

	std::string cast = dialog.get_preview_filename();

	filename = cast.c_str();

	pixbuf = Gdk::Pixbuf::create_from_file(filename, 200, 300, NULL);
	//have_preview = (pixbuf != NULL);

	preview.set(pixbuf);

	dialog.set_preview_widget_active(have_preview);

}
*/
