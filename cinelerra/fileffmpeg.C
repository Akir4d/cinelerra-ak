
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * Copyright (C) 2010 Monty <monty@xiph.org>
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

#define __STDC_CONSTANT_MACROS 1
#include "asset.h"
#include "bcsignals.h"
#include "clip.h"
#include "file.h"
#include "ffmpeg.h"
#include "fileffmpeg.h"
#include "mutex.h"
#include <unistd.h>
#include <stdint.h>
#include "videodevice.inc"

#include <string.h>

Mutex* FileFFMPEG::ffmpeg_lock = new Mutex("FileFFMPEG::ffmpeg_lock");

FileFFMPEG::FileFFMPEG(Asset *asset, File *file)
: FileBase(asset, file)
{
	ffmpeg_frame = 0;
	ffmpeg_audio_frame = 0;
	av_log_set_level(1);
	reset();
	if(asset->format == FILE_UNKNOWN)
		asset->format = FILE_FFMPEG;
}

FileFFMPEG::~FileFFMPEG()
{
	close_file();
}

void FileFFMPEG::reset()
{
	if(ffmpeg_frame) av_free(ffmpeg_frame);
	if(ffmpeg_audio_frame) av_free(ffmpeg_audio_frame);
	ffmpeg_frame = 0;
	ffmpeg_audio_frame = 0;
	ffmpeg_file_context = 0;
	ffmpeg_format = 0;
	audio_index = -1;
	video_index = -1;
	current_frame = 0;
	current_sample = 0;
	unsynced = 1;
}

char* FileFFMPEG::get_format_string(Asset *asset)
{
	unsigned char test[16];
	FILE *in = fopen(asset->path, "r");
	char *format_string = 0;

	if(in)
	{
		fread(test, sizeof(test), 1, in);
		if(test[0] == 0x1a &&
				test[1] == 0x45 &&
				test[2] == 0xdf &&
				test[3] == 0xa3)
		{
			format_string = (char*)"matroska";
		}

		fclose(in);
		return format_string;
	}

	return 0;
}

int FileFFMPEG::check_sig(Asset *asset)
{

	ffmpeg_lock->lock("FileFFMPEG::check_sig");
	avcodec_register_all();
	av_register_all();

	AVFormatContext *ffmpeg_file_context = 0;
	int result = avformat_open_input(
			&ffmpeg_file_context,
			asset->path,
			0,
			NULL);

	if(result >= 0)
	{
		result = avformat_find_stream_info(ffmpeg_file_context, NULL);

		if(result >= 0)
		{
			avformat_close_input(&ffmpeg_file_context);
			ffmpeg_lock->unlock();
			return 1;
		}

		ffmpeg_lock->unlock();
		return 0;
	}
	else
	{
		ffmpeg_lock->unlock();
		return 0;
	}
}

int FileFFMPEG::open_file(int rd, int wr)
{
	int result = 0;

	ffmpeg_lock->lock("FileFFMPEG::open_file");
	avcodec_register_all();
	av_register_all();

	if(rd){
		result = avformat_open_input(
				&ffmpeg_file_context,
				asset->path,
				0,
				NULL);

		if(result >= 0)
		{
			result = avformat_find_stream_info(ffmpeg_file_context, NULL);
		}
		else
		{
			ffmpeg_lock->unlock();
			return 1;
		}

		// Convert format to asset
		if(result >= 0)
		{
			result = 0;
			asset->format = FILE_FFMPEG;
			for(int i = 0; i < ffmpeg_file_context->nb_streams; i++)
			{
				AVStream *stream = ffmpeg_file_context->streams[i];
				AVCodecContext *decoder_context = stream->codec;
				switch(decoder_context->codec_type)
				{
				case AVMEDIA_TYPE_AUDIO:
					if(audio_index < 0)
					{
						audio_index = i;
						asset->audio_data = 1;
						asset->channels = decoder_context->channels;
						asset->sample_rate = decoder_context->sample_rate;
						asset->audio_length = (int64_t)(ffmpeg_file_context->duration *
								asset->sample_rate /
								AV_TIME_BASE);
						AVCodec *codec = avcodec_find_decoder(decoder_context->codec_id);
						if(!codec)
						{
							fprintf(stderr,"FileFFMPEG::open_file: audio codec 0x%x not found.\n",
									decoder_context->codec_id);
							asset->audio_data = 0;
							audio_index = -1;
						}
						else
						{
							avcodec_open2(decoder_context, codec, NULL);
						}
						strncpy(asset->acodec, codec->name, 4);
						asset->bits = av_get_bytes_per_sample(decoder_context->sample_fmt)*8;
					}
					break;

				case AVMEDIA_TYPE_VIDEO:
					if(video_index < 0)
					{
						video_index = i;
						asset->video_data = 1;
						asset->layers = 1;
						asset->width = decoder_context->width;
						asset->height = decoder_context->height;

						asset->frame_rate = (double)stream->r_frame_rate.num /
								stream->r_frame_rate.den;
						asset->video_length = (int64_t)(ffmpeg_file_context->duration *
								asset->frame_rate /
								AV_TIME_BASE);
						asset->aspect_ratio =
								(double)(decoder_context->coded_width*decoder_context->sample_aspect_ratio.num) /
								(decoder_context->coded_height * decoder_context->sample_aspect_ratio.den);
						AVCodec *codec = avcodec_find_decoder(decoder_context->codec_id);
						avcodec_open2(decoder_context, codec, NULL);
						strncpy(asset->vcodec, codec->name, 4);
					}
					break;

				default:
					break;
				}
			}
		}
		else
		{
			ffmpeg_lock->unlock();
			return 1;
		}
	}

	ffmpeg_lock->unlock();
	return result;
}

int FileFFMPEG::close_file()
{
	ffmpeg_lock->lock("FileFFMPEG::close_file");

	if(ffmpeg_file_context)
	{
		if(video_index >= 0)
		{
			AVStream *stream = ffmpeg_file_context->streams[video_index];
			if(stream)
			{
				AVCodecContext *decoder_context = stream->codec;
				if(decoder_context) avcodec_close(decoder_context);
			}
		}

		if(audio_index >= 0)
		{
			AVStream *stream = ffmpeg_file_context->streams[audio_index];
			if(stream)
			{
				AVCodecContext *decoder_context = stream->codec;
				if(decoder_context) avcodec_close(decoder_context);
			}
		}

		avformat_close_input(&ffmpeg_file_context);
	}
	ffmpeg_file_context = 0;
	reset();
	ffmpeg_lock->unlock();
	return 0;
}


int64_t FileFFMPEG::get_memory_usage()
{
	return 0;
}


int FileFFMPEG::colormodel_supported(int colormodel)
{
	return colormodel;
}

int FileFFMPEG::get_best_colormodel(Asset *asset, int driver)
{
	switch(driver)
	{
	case PLAYBACK_X11:
		return BC_RGB888;
	case PLAYBACK_X11_XV:
	case PLAYBACK_ASYNCHRONOUS:
		return BC_YUV420P;
	case PLAYBACK_X11_GL:
		return BC_YUV888;
	default:
		return BC_YUV420P;
	}
}

int FileFFMPEG::seek_streams(AVFormatContext *s, int index, int target, int seekto)
{
	int flags = AVSEEK_FLAG_BACKWARD;
	int ret = 1;

	if(s->seek2any) flags |= AVSEEK_FLAG_ANY;

	ret = avformat_seek_file(s,
			index,
			INT64_MIN,
			target < seekto ? target : seekto,
			seekto > target ? seekto : INT64_MAX,
			flags);
	if (ret != 0)
	{
		ret = av_seek_frame(s, index, seekto, flags);
	}
	return ret;
}
#define EPSILON .0000001

int FileFFMPEG::read_frame(VFrame *frame)
{
	int error = 0;
	ffmpeg_lock->lock("FileFFMPEG::read_frame");
	AVStream *stream = ffmpeg_file_context->streams[video_index];
	AVCodecContext *decoder_context = stream->codec;
	AVFormatContext *avcontext = ffmpeg_file_context;

	// file start time, adjusted to the stream's timebase
	int64_t stream_start = av_rescale_q(avcontext->start_time, AV_TIME_BASE_Q,
			stream->time_base);

#define SEEK_THRESHOLD 32
#define SEEK_BACK_START 16
#define SEEK_BACK_LIMIT 512
	int adj = 0;
	if(file->current_frame < 1) adj = 1;
	int64_t target =
			(file->current_frame + adj)/ asset->frame_rate *
			stream->time_base.den/stream->time_base.num + stream_start;
	int64_t target_min = target -
			(1.*stream->time_base.den/stream->time_base.num/asset->frame_rate) +
			.5 + EPSILON;
	int64_t target_abort = target +
			(1.*stream->time_base.den/stream->time_base.num/asset->frame_rate);

	int seek_back = 0; // yes, zero not SEEK_BACK_START
	int pre_sync;
	int got_keyframe = 0;
	int got_it = 0;

	if((file->current_frame <= current_frame) ||
			(file->current_frame > current_frame + SEEK_THRESHOLD) ||
			unsynced) pre_sync = 0;
	else pre_sync = 1;


	if((file->current_frame != current_frame + 1) || unsynced)
	{
		unsynced = 0;

		while(!got_it && !error && seek_back <= SEEK_BACK_LIMIT){
			got_keyframe = 1;

			// Due to reordering and lax/unknown A/V sync in a given file,
			// the ffmpeg frame seek is not particularly accurate, nor is
			// the slush is well bounded. In addition, assuming pessimal
			// behavior will badly hurt performance on files that don't show
			// the imprecision...

			// If the next PTS we want is behind us (or *well* ahead), we
			// seek in an 'expanding circle'; if the PTS we finally get from
			// good data is too late, backtrack further and try again.
			// Limit the damage.

			// Otherwise, we simply read ahead to the desired point.

			if(!pre_sync)
			{
				int64_t seekto = target - seek_back *
						stream->time_base.den /
						stream->time_base.num /
						asset->frame_rate;

				if(seekto <= stream_start + EPSILON)
				{
					seekto = 0;
					seek_back = SEEK_BACK_LIMIT;
				}

				got_keyframe = 0;

				if(0) fprintf(stderr,"adjusted=%.03f \n",1.*(seekto-stream_start)*
						stream->time_base.num/stream->time_base.den);
				if(seek_streams(avcontext,
						video_index,
						target,
						seekto) < 0)
				{
					error = 1;
					fprintf(stderr,"FileFFMPEG::read_frame SEEK FAILED!!!\n");
				}
				avcodec_flush_buffers(decoder_context);
			}
			// read till we successfully decode *some* picture.  If the
			// resulting picture has a pts preceeding our syncpoint, we've
			// achieved pre_sync.
			// Note: we're having ffmpeg reorder for us, so we're properly
			// working according to opaque_reordered PTS

			while(!error)
			{
				AVPacket packet;
				int got_pic = 0;

				error = av_read_frame(avcontext, &packet);

				if(!error && packet.flags != AV_PKT_FLAG_CORRUPT)
				{
					if(packet.size > 0 && packet.stream_index == video_index)
					{
						// After the seek, we don't usually get a keyframe
						// (libavcodec documentation be damned).  Do not submit a
						// non-keyframe first packet to the decoder, as eg the
						// mpeg2 decoder is known to misidentify non-keyframes as
						// keyframes, resulting in garbled output.
						if(packet.flags == AV_PKT_FLAG_KEY)
						{
							got_keyframe = 1;
						}


						// starting past where we want to be?
						if(!pre_sync &&
								packet.pts != AV_NOPTS_VALUE &&
								target_abort < EPSILON &&
								packet.pts > target_abort)
						{
							av_free_packet(&packet);
							break;
						}

						if(got_keyframe)
						{
							int64_t packet_pts = packet.pts;
							int64_t packet_dts = packet.dts;

							if(!ffmpeg_frame) ffmpeg_frame = av_frame_alloc();

							decoder_context->reordered_opaque = packet_pts;
							int result = avcodec_decode_video2(decoder_context,
									ffmpeg_frame,
									&got_pic,
									&packet);
							av_free_packet(&packet);

							if(!ffmpeg_frame->data[0] || result<=0) got_pic = 0;

							// although we checked that the packet was marked
							// keyframe above, some backends (eg, AVI) also mis-mark
							// packets depending on seek mode.  Between the packet
							// check and the frame check below, we should be sure to avoid
							// all false positives
							if(got_pic && !pre_sync &&
									!ffmpeg_frame->key_frame) got_pic = 0;

							if(got_pic)
							{
								int64_t returned_pts = ffmpeg_frame->reordered_opaque;

								if(returned_pts == AV_NOPTS_VALUE){
									if (packet_dts != AV_NOPTS_VALUE){
										returned_pts= packet_dts;
									}
									else
									{
										if (packet_dts != AV_NOPTS_VALUE)
											returned_pts= packet_pts;
										else{
											fprintf(stderr,"FileFFMPEG::read_frame Missing Timestamps during SYNC!\n"
													"\tAborting sync on this video stream for now!\n");
											pre_sync = 1;
											got_it = 1;
											current_frame = file->current_frame;
											break;
										}
									}
								}

								if(returned_pts <= target && !pre_sync) pre_sync = 1;

								// is the PTS in the landing zone
								if(pre_sync && returned_pts >= target_min)
								{
									got_it = 1;
									current_frame = file->current_frame;
									break;
								}

								if(returned_pts > target && !pre_sync)
									break;
							}
						}
					}
					else
					{
						av_free_packet(&packet);
					}
				}
			}
			if(seek_back==0)
			{
				seek_back = SEEK_BACK_START;
			}
			else
			{
				seek_back<<=1;
			}
			pre_sync = 0;
		}
		if(error || !got_it) unsynced = 1;
		if(error)
		{
			// at this point we have failed
			ffmpeg_lock->unlock();
			return error;
		}
	}
	else
	{
		// this is the nominal, non-seek case.  Read a frame, don't bother checking sync
		while(!error)
		{
			AVPacket packet;
			int got_pic = 0;

			error = av_read_frame(avcontext,&packet);

			if(!error){
				if(packet.size > 0 && packet.stream_index == video_index){

					if(!ffmpeg_frame) ffmpeg_frame = av_frame_alloc();

					decoder_context->reordered_opaque = packet.pts;
					int result =
							avcodec_decode_video2(decoder_context,
									ffmpeg_frame,
									&got_pic,
									&packet);
					av_free_packet(&packet);
					if(!ffmpeg_frame->linesize[0] || !result) got_pic = 0;

					if(got_pic)
					{
						int64_t returned_pts = ffmpeg_frame->reordered_opaque - stream_start;
						current_frame++;
						got_it = 1;
						break;
					}
				}else{
					av_free_packet(&packet);
				}
			}
		}
		if(error)
		{
			ffmpeg_lock->unlock();
			return error;
		}
	}

	// Convert colormodel: Use ffmpeg, as it's not clear that the
	// quicktime code knows anything about alternate chroma siting
	if(got_it){

		FFMPEG::convert_cmodel((AVPicture *)ffmpeg_frame, decoder_context->pix_fmt,
				decoder_context->width, decoder_context->height, frame);
	}

	ffmpeg_lock->unlock();
	return error;
}

int FileFFMPEG::read_samples(double *buffer, int64_t len)
{
	int error = 0;
	ffmpeg_lock->lock("FileFFMPEG::read_samples");
	AVFormatContext *avcontext = ffmpeg_file_context;
	AVStream *stream = avcontext->streams[audio_index];
	AVCodecContext *decoder_context = stream->codec;
	int accumulation = 0;
	const int debug = 0;

	// file start time, adjusted to the stream's timebase
	int64_t stream_start = av_rescale_q(avcontext->start_time,
			AV_TIME_BASE_Q,
			stream->time_base);
	update_pcm_history(len);

	// Seek occurred
	if(decode_start != decode_end || unsynced)
	{
#define A_SEEK_THRESHOLD 1.
#define A_SEEK_BACK_START .2
#define A_SEEK_BACK_LIMIT 5.

		// as with video, ffmpeg doesn't generally give us the specific
		// PTSes we asked for.  It's usually several frames late.

		int64_t target;
		double seek_back = A_SEEK_BACK_START;
		int pre_sync = 0;
		int got_it = 0;

		target = 1. * decode_start * stream->time_base.den / stream->time_base.num /
				asset->sample_rate + stream_start;

		unsynced = 0;
		while(!got_it && !error && seek_back <= A_SEEK_BACK_LIMIT)
		{
			int64_t seekto = target - seek_back * stream->time_base.den / stream->time_base.num;
			accumulation = 0;

			if(seekto < stream_start)
			{
				seekto = stream_start;
				seek_back = A_SEEK_BACK_LIMIT;
			}
			if(seek_streams(avcontext,
					audio_index,
					target,
					seekto))
			{
				error = 1;
				fprintf(stderr,"FileFFMPEG:read_samples SEEK FAILED!!!\n");
			}
			decode_end = decode_start;
			current_sample = file->current_sample;
			avcodec_flush_buffers(decoder_context);

			// parallel video process; read forward until we see PTSes we
			// like.  If we start too late, backtrack and try again.

			while(!error && (!got_it || accumulation<decode_len))
			{
				AVPacket packet;
				error = av_read_frame(avcontext,&packet);

				if(!error)
				{
					if(packet.size > 0 && packet.stream_index == audio_index)
					{
						int packet_len = packet.size;
						uint8_t *packet_ptr = packet.data;
						int64_t pts_sample = AV_NOPTS_VALUE;

						if(packet.pts == AV_NOPTS_VALUE)
						{
							fprintf(stderr,"FileFFMPEG::read_samples Missing Timestamps during SYNC!\n"
									"\tAborting sync on this audio stream for now!\n");
							pts_sample = 1. * (seekto-stream_start) *
									stream->time_base.num*asset->sample_rate /
									stream->time_base.den;
						}
						else
						{
							pts_sample = 1. * (packet.pts-stream_start) *
									stream->time_base.num*asset->sample_rate/stream->time_base.den;
						}

						if(!pre_sync && packet.pts > target){
							if(seek_back*2.+EPSILON >= A_SEEK_BACK_LIMIT){
								// we've searched as far back as permitted; we've found
								// no audio at the requested starting point so pad up to
								// this point and continue
								int64_t padding = (packet.pts - stream_start) *
										asset->sample_rate * stream->time_base.num / stream->time_base.den -
										file->current_sample;

								// don't assume the A/V sync offset will be less than
								// the maximum history depth
								// len and decode_len are equivalent here
								if(padding<decode_len)
								{
									pad_history(padding);
									accumulation += padding;
									pre_sync = 1;
									got_it = 1;
								}
								else
								{
									// only account for the padding of the requested
									// range; we'll need to repeat this process next call
									pad_history(decode_len);
									unsynced = 1;
									// be sure
									seek_back = A_SEEK_BACK_LIMIT+EPSILON;
									av_free_packet(&packet);
									break;
								}
							}
							else
							{
								// Normal case; backtrack and try again
								av_free_packet(&packet);
								break;
							}
						}

						while(packet_len > 0){
							int frame_decoded;
							if(!ffmpeg_audio_frame) ffmpeg_audio_frame = av_frame_alloc();
							int bytes_decoded = avcodec_decode_audio4(decoder_context,
									ffmpeg_audio_frame,
									&frame_decoded,
									&packet);
							if(!frame_decoded)
							{
								// Do not set error; this is standard operating
								// procedure for eg mp3, where a packet often begins in
								// the middle of a frame.  Proper behavior is to handle
								// next packet.
								break;
							}
							else
							{
								int samples_decoded = ffmpeg_audio_frame->nb_samples;

								packet_ptr += bytes_decoded;
								packet_len -= bytes_decoded;

								// anything predating or at the target sample sets presync
								if(!pre_sync && pts_sample <= file->current_sample) pre_sync = 1;

								// Do any of the produced samples fall at the target or later?
								if(got_it || pts_sample+samples_decoded > decode_start)
								{
									if (!pre_sync) break;

									if(got_it || pts_sample>=decode_start)
									{
										// all samples are past 'start'; use all samples
										append_history(ffmpeg_audio_frame,
												decoder_context->sample_fmt,
												0,
												samples_decoded);
										accumulation += samples_decoded;
									}
									else
									{
										// decode partially precedes the range we want;
										// use only the samples at and past the start
										// marker
										int offset = decode_start - pts_sample;
										if(offset<samples_decoded)
										{
											append_history(ffmpeg_audio_frame,
													decoder_context->sample_fmt,
													offset,
													samples_decoded-offset);
											accumulation += samples_decoded-offset;
										}
									}
									if(0 && !got_it)
										fprintf(stderr,"delivered = %.03f(%.03f) ",
												1.*(packet.pts-stream_start)/stream->time_base.den*stream->time_base.num,
												1.*(packet.pts-stream_start)/stream->time_base.den*stream->time_base.num+
												1.*samples_decoded/asset->sample_rate);

									got_it=1;
								}
								pts_sample += samples_decoded;
							}
						}
					}
					av_free_packet(&packet);
				}
			}

			seek_back *= 2.;
			pre_sync = 0;
		}
	}
	else
	{
		// nominal case; no seek, just keep reading samples

		while(!error && accumulation<decode_len)
		{
			AVPacket packet;
			error = av_read_frame(avcontext,&packet);

			if(!error)
			{
				if(packet.size > 0 && packet.stream_index == audio_index)
				{
					int packet_len = packet.size;
					uint8_t *packet_ptr = packet.data;
					int frame_decoded;
					if(!ffmpeg_audio_frame) ffmpeg_audio_frame = av_frame_alloc();
					int bytes_decoded = avcodec_decode_audio4(decoder_context,
							ffmpeg_audio_frame,
							&frame_decoded,
							&packet);
					if(!frame_decoded) error = 1;
					else
					{
						int samples_decoded = ffmpeg_audio_frame->nb_samples;;
						packet_ptr += bytes_decoded;
						packet_len -= bytes_decoded;

						append_history(ffmpeg_audio_frame,
								decoder_context->sample_fmt,
								0,
								samples_decoded);
						accumulation += samples_decoded;
					}
				}
				av_free_packet(&packet);
			}
		}

		// at end of stream, audio may have ended early-- if we didn't get
		// the full accumulation, pad
		if(accumulation < decode_len) pad_history(decode_len - accumulation);
	}


	read_history(buffer,
			file->current_sample,
			file->current_channel,
			len);

	current_sample += len;
	if(error) unsynced=1;

	ffmpeg_lock->unlock();
	return error;
}






