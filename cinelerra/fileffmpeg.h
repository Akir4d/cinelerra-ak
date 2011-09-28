
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

#ifndef FILEFFMPEG_H
#define FILEFFMPEG_H
#include "asset.inc" 
#include "filebase.h"
#include "file.inc"
#include "mutex.inc"
extern "C"
{
#include <libavformat/avformat.h>
}

// Decoding for all FFMPEG formats


class FileFFMPEG : public FileBase
{
public:
	FileFFMPEG(Asset *asset, File *file);
	~FileFFMPEG();

// Get format string for ffmpeg
	static char* get_format_string(Asset *asset);
	static int check_sig(Asset *asset);
	void reset();
	int open_file(int rd, int wr);
	int close_file();

	int64_t get_memory_usage();
	int colormodel_supported(int colormodel);
	int get_best_colormodel(Asset *asset, int driver);
	int read_frame(VFrame *frame);
	int read_samples(double *buffer, int64_t len);

	void dump_context(void *ptr);


// Streams to decode
	int audio_index;
	int video_index;
// Next read positions
	int64_t current_frame;
	int64_t current_sample;
// Last decoded positions
	int64_t decoded_frame;
	int64_t decoded_sample;
	static Mutex *ffmpeg_lock;
	int first_frame;
	int unsynced;
	double pts;
private:
	int debug;
	AVFormatContext *ffmpeg_file_context;
	AVFrame *ffmpeg_frame;
	void *ffmpeg_format;
	short *ffmpeg_samples;
};





#endif
