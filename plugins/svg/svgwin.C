
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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
#include "clip.h"
#include "svgwin.h"
#include "string.h"
#include "filexml.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

struct fifo_struct {
        int pid;
        int action;  // 1 = update from client, 2 = client closes
      };


#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

#include "empty_svg.h"



PLUGIN_THREAD_OBJECT(SvgMain, SvgThread, SvgWin)






SvgWin::SvgWin(SvgMain *client, int x, int y)
 : BC_Window(client->gui_string, 
 	x,
	y,
	320,
	180,
	300, 
	180,
	0, 
	0,
	1)
{ 
	dpi = 0;
	out_y = 0;
	out_h = 0;
	in_w = 0;
	in_h = 0;
	edit_svg_button= 0;
	new_svg_button= 0;
	in_y= 0;
	out_x= 0;
	svg_file_title= 0;
	out_w= 0;
	new_svg_thread= 0;
	this->client = client; 
	this->editing = 0;
}

SvgWin::~SvgWin()
{
}

int SvgWin::create_objects()
{
	int x = 10, y = 10;

	add_tool(new BC_Title(x, y, _("DPI")));
	y += 20;
	dpi = new SvgCoord(this, client, x, y, &client->config.dpi);
	dpi->create_objects();
	y += 30;

//	add_tool(new BC_Title(x, y, _("In Y:")));
	y += 20;
//	in_y = new SvgCoord(this, client, x, y, &client->config.in_y);
//	in_y->create_objects();
	y += 30;

//	add_tool(new BC_Title(x, y, _("In W:")));
	y += 20;
//	in_w = new SvgCoord(this, client, x, y, &client->config.in_w);
//	in_w->create_objects();
	y += 30;

//	add_tool(new BC_Title(x, y, _("In H:")));
	y += 20;
//	in_h = new SvgCoord(this, client, x, y, &client->config.in_h);
//	in_h->create_objects();
	y += 30;


	x += 150;
	y = 10;
	add_tool(new BC_Title(x, y, _("Out X:")));
	y += 20;
	out_x = new SvgCoord(this, client, x, y, &client->config.out_x);
	out_x->create_objects();
	y += 30;

	add_tool(new BC_Title(x, y, _("Out Y:")));
	y += 20;
	out_y = new SvgCoord(this, client, x, y, &client->config.out_y);
	out_y->create_objects();
	y += 30;

/*	add_tool(new BC_Title(x, y, _("Out W:")));
	y += 20;
	out_w = new SvgCoord(this, client, x, y, &client->config.out_w);
	out_w->create_objects();
	y += 30;

	add_tool(new BC_Title(x, y, _("Out H:")));
	y += 20;
	out_h = new SvgCoord(this, client, x, y, &client->config.out_h);
	out_h->create_objects();
	y += 30;
*/
	x -= 150;
	add_tool(new_svg_button = new NewSvgButton(client, this, x, y));
	add_tool(edit_svg_button = new EditSvgButton(client, this, x+190, y));
	add_tool(svg_file_title = new BC_Title(x, y+26, client->config.svg_file));

	x +=150;

	show_window();
	flush();
	return 0;
}

int SvgWin::close_event()
{
	set_done(1);
	return 1;
}

SvgCoord::SvgCoord(SvgWin *win, 
	SvgMain *client, 
	int x, 
	int y,
	float *value)
 : BC_TumbleTextBox(win,
 	*value,
	(float)0,
	(float)3000,
	x, 
	y, 
	100)
{
//printf("SvgWidth::SvgWidth %f\n", client->config.w);
	this->client = client;
	this->win = win;
	this->value = value;
}

SvgCoord::~SvgCoord()
{
}

int SvgCoord::handle_event()
{
	*value = atof(get_text());

	client->send_configure_change();
	return 1;
}

NewSvgButton::NewSvgButton(SvgMain *client, SvgWin *window, int x, int y)
 : BC_GenericButton(x, y, _("New/Open SVG..."))
{
	this->client = client;
	this->window = window;
	quit_now = 0;
}
int NewSvgButton::handle_event()
{
	window->editing_lock.lock();
	if (!window->editing) 
	{
		window->editing = 1;
		window->editing_lock.unlock();
		quit_now = 0;
		start();
	} else
	{
		// FIXME - display an error
		window->editing_lock.unlock();
	}

	return 1;
}

void NewSvgButton::run()
{
// ======================================= get path from user
	int result;
//printf("NewSvgButton::run 1\n");
	char directory[1024], filename[1024];
	sprintf(directory, "~");
	client->defaults->get("DIRECTORY", directory);

	result = 1;
// Loop until file is chosen
	do{
		NewSvgWindow *new_window;

		new_window = new NewSvgWindow(client, window, directory);
		new_window->create_objects();
		new_window->update_filter("*.svg");
		result = new_window->run_window();
		client->defaults->update("DIRECTORY", new_window->get_path(0));
		strcpy(filename, new_window->get_path(0));
		delete new_window;

// Extend the filename with .svg
		if(strlen(filename) < 4 || 
			strcasecmp(&filename[strlen(filename) - 4], ".svg"))
		{
			strcat(filename, ".svg");
		}

// ======================================= try to save it
		if((filename[0] == 0) || (result == 1)) {
			window->editing_lock.lock();
			window->editing = 0;
			window->editing_lock.unlock();
			return;              // no filename given
		}
		FILE *in;
		if(in = fopen(filename, "rb"))
		{
			fclose(in);
			// file exists
			result = 0; 
		} else
		{
			// create fresh file
			in = fopen(filename, "w+");
			unsigned long size;
			size = (((unsigned long)empty_svg[0]) << 24) +
				(((unsigned long)empty_svg[1]) << 16) +
				(((unsigned long)empty_svg[2]) << 8) +
				((unsigned long)empty_svg[3]);
			printf("in: %p size: %li\n", in, size);
			// FIXME this is not cool on different arhitectures
			
			fwrite(empty_svg+4, size,  1, in);
			fclose(in);
			result = 0;
		}
	} while(result);        // file doesn't exist so repeat
	

	window->svg_file_title->update(filename);
	window->flush();
	strcpy(client->config.svg_file, filename);
	client->need_reconfigure = 1;
	client->force_png_render = 1;
	client->send_configure_change();

// save it
	if(quit_now) window->set_done(0);
	window->editing_lock.lock();
	window->editing = 0;
	window->editing_lock.unlock();

	return;
}

EditSvgButton::EditSvgButton(SvgMain *client, SvgWin *window, int x, int y)
 : BC_GenericButton(x, y, _("Edit"))
{
	fh_fifo = 0;
	this->client = client;
	this->window = window;
	quit_now = 0;
}

EditSvgButton::~EditSvgButton() {
	struct fifo_struct fifo_buf;
	fifo_buf.pid = getpid();
	fifo_buf.action = 3;
	quit_now = 1;
	write (fh_fifo, &fifo_buf, sizeof(fifo_buf)); // break the thread out of reading from fifo
}

int EditSvgButton::handle_event()
{
	
	window->editing_lock.lock();
	if (!window->editing && client->config.svg_file[0] != 0) 
	{
		window->editing = 1;
		window->editing_lock.unlock();
		start();
	} else
	{
		// FIXME - display an error
		window->editing_lock.unlock();
	}
	return 1;
}

void EditSvgButton::run()
{
// ======================================= get path from user
	Timer pausetimer;
	long delay;
	int result;
	time_t last_update;
	int32_t last_time;
	int32_t new_time;
	struct stat st_svg;
	char filename_svg[1024];
	char filename_png[1024];
	char filename_fifo[1024];
	struct fifo_struct fifo_buf;
	SvgInkscapeThread *inkscape_thread = new SvgInkscapeThread(client, window);
	
	strcpy(filename_svg, client->config.svg_file);
	result = stat (filename_svg, &st_svg);
	last_update = st_svg.st_mtime;
	last_time = st_svg.st_mtim.tv_sec;
	if (result) 
		last_update = 0;	

	strcpy(filename_fifo, filename_svg);
	strcpy(filename_png, filename_svg);

	strcat(filename_fifo, ".fifo");
	strcat(filename_png, ".png");

	if (mkfifo(filename_fifo, S_IRWXU) != 0) {
		perror("Error while creating fifo file");
	} 
	fh_fifo = open(filename_fifo, O_RDWR);
	fifo_buf.action = 0;
	inkscape_thread->fh_fifo = fh_fifo;
	inkscape_thread->start();
	printf("\nrunning state: %d\n", inkscape_thread->running());
	printf("\nquit_now_ %d\n", quit_now);
	while (inkscape_thread->running() && (!quit_now)) {

		result = stat (filename_svg, &st_svg);
		new_time = st_svg.st_mtim.tv_sec;
		pausetimer.delay(300);

		if (last_time != new_time) {
			printf(_("Inkscape has Update file\n"));
			result = stat (filename_svg, &st_svg);
				remove(filename_png);
				client->send_configure_change();
				last_time = new_time;
		} else 
		if (fifo_buf.action == 2) {
			printf(_("Inkscape has exited\n"));
		} else
		if (fifo_buf.action == 3) {
			printf(_("Plugin window has closed\n"));
			delete inkscape_thread;
			close(fh_fifo);
			return;
		}
	}
	inkscape_thread->join();
	close(fh_fifo);
	window->editing_lock.lock();
	window->editing = 0;
	window->editing_lock.unlock();

}

SvgInkscapeThread::SvgInkscapeThread(SvgMain *client, SvgWin *window)
 : Thread(1)
{
	fh_fifo = 0;
	this->client = client;
	this->window = window;
}

SvgInkscapeThread::~SvgInkscapeThread()
{
	// what do we do? kill inkscape?
	cancel();
}

void SvgInkscapeThread::run()
{
// Runs the inkscape
	char command[1024];
	char filename_png[1024];
	strcpy(filename_png, client->config.svg_file);
	strcat(filename_png, ".png");

	sprintf(command, "inkscape %s",
			client->config.svg_file, filename_png);
	printf(_("Running external SVG editor: %s\n"), command);		
	enable_cancel();
	system(command);
	printf(_("External SVG editor finished\n"));
	{
		struct fifo_struct fifo_buf;
		fifo_buf.pid = getpid();
		fifo_buf.action = 2;
		write (fh_fifo, &fifo_buf, sizeof(fifo_buf));
	}
	remove(filename_png);
	client->send_configure_change();
	disable_cancel();
	return;
}



NewSvgWindow::NewSvgWindow(SvgMain *client, SvgWin *window, char *init_directory)
 : BC_FileBox(0,
 	BC_WindowBase::get_resources()->filebox_h / 2,
 	init_directory, 
	_("SVG Plugin: Pick SVG file"), 
	_("Open an existing SVG file or create a new one"))
{ 
	this->window = window; 
}

NewSvgWindow::~NewSvgWindow() {}







