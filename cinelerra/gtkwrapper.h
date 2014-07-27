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

#ifndef GTKMM_GTKWRAPPER_H
#define GTKMM_GTKWRAPPER_H
#include "arraylist.h"
class GtkWrapper
{
public:
	GtkWrapper();
	~GtkWrapper();
		
	void init(int argc, char* argv[]);
	int loadfiles_wrapper(ArrayList<char*> &path_list,
			  	  	 int loadmodein,
			  	  	 int &loadmodeout,
					 char* path_defaultin,
					 char* path_defaultout,
			  	  	 int filterin,
			  	  	 int &filterout);
};
#endif
