
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
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
#include "bcsignals.h"
#include "clip.h"
#include "file.h"
#include "ffmpeg.h"
#include "fileffmpeg.h"
#include "mutex.h"
#include <unistd.h>
#include "videodevice.inc"

#include <string.h>

Mutex* FileFFMPEG::ffmpeg_lock = new Mutex("FileFFMPEG::ffmpeg_lock");

FileFFMPEG::FileFFMPEG(Asset *asset, File *file)
  : FileBase(asset, file)
{
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
  ffmpeg_file_context = 0;
  ffmpeg_format = 0;
  ffmpeg_frame = 0;
  ffmpeg_samples = 0;
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
  char *ptr = strstr(asset->path, ".pcm");
  if(ptr) return 0;

  ffmpeg_lock->lock("FileFFMPEG::check_sig");
  avcodec_init();
  avcodec_register_all();
  av_register_all();

  AVFormatContext *ffmpeg_file_context = 0;
  AVFormatParameters params;
  bzero(&params, sizeof(params));
  int result = av_open_input_file(
                                  &ffmpeg_file_context,
                                  asset->path,
                                  0,
                                  0,
                                  &params);

  if(result >= 0)
    {
      result = av_find_stream_info(ffmpeg_file_context);


      if(result >= 0)
        {
          av_close_input_file(ffmpeg_file_context);
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
  AVFormatParameters params;
  bzero(&params, sizeof(params));

  ffmpeg_lock->lock("FileFFMPEG::open_file");
  avcodec_init();
  avcodec_register_all();
  av_register_all();

  if(rd){
      result = av_open_input_file(
                                  (AVFormatContext**)&ffmpeg_file_context,
                                  asset->path,
                                  0,
                                  0,
                                  &params);

      if(result >= 0)
        {
          result = av_find_stream_info((AVFormatContext*)ffmpeg_file_context);
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
          for(int i = 0; i < ((AVFormatContext*)ffmpeg_file_context)->nb_streams; i++)
            {
              AVStream *stream = ((AVFormatContext*)ffmpeg_file_context)->streams[i];
              AVCodecContext *decoder_context = stream->codec;
              switch(decoder_context->codec_type)
                {
                case CODEC_TYPE_AUDIO:
                  if(audio_index < 0)
                    {
                      audio_index = i;
                      asset->audio_data = 1;
                      asset->channels = decoder_context->channels;
                      asset->sample_rate = decoder_context->sample_rate;
                      asset->audio_length = (int64_t)(((AVFormatContext*)ffmpeg_file_context)->duration *
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
                          avcodec_thread_init(decoder_context, file->cpus);
                          avcodec_open(decoder_context, codec);
                        }
                      asset->bits = av_get_bits_per_sample_format(decoder_context->sample_fmt);
                    }
                  break;

                case CODEC_TYPE_VIDEO:
                  if(video_index < 0)
                    {

                      video_index = i;
                      asset->video_data = 1;
                      asset->layers = 1;
                      asset->width = decoder_context->width;
                      asset->height = decoder_context->height;

                      if(EQUIV(asset->frame_rate, 0)){

                        /* Look for Canon 24F; it's the oddball in the bunch */
                        if(decoder_context->coded_width == 1440 &&
                           decoder_context->coded_height == 1080 &&
                           decoder_context->coded_height == 1080 &&
                           1. * stream->avg_frame_rate.num / stream->avg_frame_rate.den <
                           1. * decoder_context->time_base.den / decoder_context->time_base.num /
                           decoder_context->ticks_per_frame){
                          asset->frame_rate = 24000/1001.;
                        }else{
                          /* otherwise, believe the codec. naturally, this can't work with VFR */
                          asset->frame_rate =
                            (double)decoder_context->time_base.den /
                            decoder_context->time_base.num /
                            decoder_context->ticks_per_frame;
                        }
                      }

                      asset->video_length = (int64_t)(((AVFormatContext*)ffmpeg_file_context)->duration *
                                                      asset->frame_rate /
                                                      AV_TIME_BASE);
                      asset->aspect_ratio =
                        (double)(decoder_context->coded_width*decoder_context->sample_aspect_ratio.num) /
                        (decoder_context->coded_height * decoder_context->sample_aspect_ratio.den);
                      AVCodec *codec = avcodec_find_decoder(decoder_context->codec_id);
                      avcodec_thread_init(decoder_context, file->cpus);
                      avcodec_open(decoder_context, codec);

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

  if(ffmpeg_file_context){
    AVStream *stream = ((AVFormatContext*)ffmpeg_file_context)->streams[video_index];
    AVCodecContext *decoder_context = stream->codec;
    if(decoder_context)
      avcodec_close(decoder_context);
    av_close_input_file((AVFormatContext*)ffmpeg_file_context);
  }
  if(ffmpeg_frame) av_free(ffmpeg_frame);
  if(ffmpeg_samples) free(ffmpeg_samples);
  ffmpeg_file_context = 0;
  reset();
  ffmpeg_lock->unlock();
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

#define EPSILON .000001

int FileFFMPEG::read_frame(VFrame *frame)
{
  int error = 0;
  ffmpeg_lock->lock("FileFFMPEG::read_frame");
  AVStream *stream = ((AVFormatContext*)ffmpeg_file_context)->streams[video_index];
  AVCodecContext *decoder_context = stream->codec;
  AVFormatContext *avcontext = (AVFormatContext *)ffmpeg_file_context;

  /* file start time, adjusted to the stream's timebase */
  int64_t stream_start = av_rescale_q(avcontext->start_time, AV_TIME_BASE_Q,
                                      stream->time_base);

#define SEEK_THRESHOLD 32
#define SEEK_BACK_START 16
#define SEEK_BACK_LIMIT 4096

  int64_t target =
    file->current_frame / asset->frame_rate *
    stream->time_base.den/stream->time_base.num + stream_start;
  int64_t target_min = target -
    (1.*stream->time_base.den/stream->time_base.num/asset->frame_rate)/2;

  int seek_back = SEEK_BACK_START;
  int pre_sync;
  int got_keyframe = 0;
  int got_it = 0;

  if((file->current_frame <= current_frame) ||
     (file->current_frame > current_frame + SEEK_THRESHOLD) ||
     unsynced)
    pre_sync = 0;
  else{
    pre_sync = 1;
  }

  if((file->current_frame != current_frame+1) || unsynced) {
    //fprintf(stderr,"video_pts SEEK %d %d requested=%.03f (min=%0.3f) ",
    //  file->current_frame, current_frame,
    //  1.*file->current_frame/asset->frame_rate,
    //  1.*(target_min-stream_start)*
    //      stream->time_base.num/stream->time_base.den);

    unsynced = 0;

    while(!got_it && !error && seek_back <= SEEK_BACK_LIMIT){
      got_keyframe = 1;

      /* Due to reordering and lax/unknown A/V sync in a given file,
         the ffmpeg frame seek is not particularly accurate, nor is
         the slush is well bounded. In addition, assuming pessimal
         behavior will badly hurt performance on files that don't show
         the imprecision...

         If the next PTS we want is behind us (or *well* ahead), we
         seek in an 'expanding circle'; if the PTS we finally get from
         good data is too late, backtrack further and try again.
         Limit the damage.

         Otherwise, we simply read ahead to the desired point.
      */

      if(!pre_sync){

        int64_t seekto = target - seek_back *
          stream->time_base.den/stream->time_base.num/asset->frame_rate;

        if(seekto <= stream_start+EPSILON){

          /* a number of files with truncated first GOPs will not be
             able to seek back to exactly the beginning correctly, but
             will be able to play from beginning.  Seeking to zero
             appears to be a special case that resets and plays from
             the first similarly as if the file was just opened, so it
             handles more files. This is common in AVCHD. */

          //fprintf(stderr,"ZERO ");
          seekto = 0;
          seek_back = SEEK_BACK_LIMIT;
        }

        got_keyframe = 0;

        //fprintf(stderr,"adjusted=%.03f ",1.*(seekto-stream_start)*
        //      stream->time_base.num/stream->time_base.den);

        avcodec_flush_buffers(decoder_context);

        /* BACKWARD flag makes AVCHD behave better */
        if(av_seek_frame(avcontext,
                         video_index,
                         seekto,
                         AVSEEK_FLAG_BACKWARD)){
          error = 1;
          fprintf(stderr,"FileFFMPEG::read_frame SEEK FAILED!!!\n");
        }

        avcodec_flush_buffers(decoder_context);
      }

      /* read till we successfully decode *some* picture.  If the
         resulting picture has a pts preceeding our syncpoint, we've
         achieved pre_sync. */
      /* Note: we're having ffmpeg reorder for us, so we're properly
         working according to opaque_reordered PTS */

      while(!error){
        AVPacket packet;
        int got_pic = 0;

        error = av_read_frame(avcontext,&packet);
        //if(error) fprintf(stderr,"PERROR ");
        if(!error && packet.size > 0 && packet.stream_index == video_index){

          //fprintf(stderr,"packet=%.03f ",
          //      1.*(packet.pts-stream_start)*
          //      stream->time_base.num/stream->time_base.den);


          /* After the seek, we don't usually get a keyframe
             (libavcodec documentation be damned).  Do not submit a
             non-keyframe first packet to the decoder, as eg the
             mpeg2 decoder is known to misidentify non-keyframes as
             keyframes, resulting in garbled output. */

          if(packet.flags&PKT_FLAG_KEY){
            got_keyframe = 1;
            //fprintf(stderr,"KEYFRAME ");
          }

          if(got_keyframe){

            if(!ffmpeg_frame)
              ffmpeg_frame = avcodec_alloc_frame();
            avcodec_get_frame_defaults((AVFrame*)ffmpeg_frame);

            decoder_context->reordered_opaque = packet.pts;
            int result =
              avcodec_decode_video(decoder_context,
                                   (AVFrame*)ffmpeg_frame,
                                   &got_pic,
                                   packet.data,
                                   packet.size);

            //fprintf(stderr,">>>VID results: reordered=%.03f pts=%.03f dts=%.03f result=%d got_pic=%d %s %s %s\n",
            //    (((AVFrame *)ffmpeg_frame)->reordered_opaque-stream_start)*
            //    1.*stream->time_base.num/stream->time_base.den,
            //    (packet.pts-stream_start)*
            //    1.*stream->time_base.num/stream->time_base.den,
            //    (packet.dts-stream_start)*
            //    1.*stream->time_base.num/stream->time_base.den,
            //    result,got_pic,
            //    ((((AVFrame*)ffmpeg_frame)->data[0])?"DATA":""),
            //    ((packet.flags&PKT_FLAG_KEY)?"PACKETKEY":""),
            //    ((((AVFrame*)ffmpeg_frame)->key_frame)?"FRAMEKEY":""));


            if(!((AVFrame*)ffmpeg_frame)->data[0] || result<=0) got_pic = 0;

            /* although we checked that the packet was marked
               keyframe above, some backends (eg, AVI) also mis-mark
               packets depending on seek mode.  Between the packet
               check and the frame check below, we should be sure to avoid
               all false positives */
            if(got_pic && !pre_sync && !((AVFrame*)ffmpeg_frame)->key_frame) got_pic = 0;

            if(got_pic){
              int64_t returned_pts =
                ((AVFrame *)ffmpeg_frame)->reordered_opaque;

              //fprintf(stderr,"delivered=%.03f ",
              //      1.*(returned_pts-stream_start)*
              //      stream->time_base.num/stream->time_base.den);


              if(returned_pts == AV_NOPTS_VALUE){
                if (packet.dts != AV_NOPTS_VALUE){
                  returned_pts= packet.dts;
                }else{
                  if (packet.dts != AV_NOPTS_VALUE)
                    returned_pts= packet.pts;
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

              if(returned_pts <= target && !pre_sync){
                //fprintf(stderr,"PRESYNC ");
                pre_sync = 1;
              }

              /* is the PTS in the landing zone */
              if(pre_sync && returned_pts >= target_min){
                //fprintf(stderr,"POSTSYNC");
                got_it = 1;
                current_frame = file->current_frame;
                break;
              }

              if(returned_pts > target && !pre_sync)
                break;
            }
          }
        }
      }

      seek_back<<=1;
      pre_sync = 0;
    }

    //fprintf(stderr,"\n");
    if(error || !got_it)
      unsynced = 1;

    if(error){
      /* at this point we have failed */
      ffmpeg_lock->unlock();
      return error;
    }
  }else{

    /* this is the nominal, non-seek case.  Read a frame, don't bother checking sync */

    while(!error){
      AVPacket packet;
      int got_pic = 0;

      error = av_read_frame(avcontext,&packet);

      if(!error && packet.size > 0){
        if(packet.stream_index == video_index){

          if(!ffmpeg_frame)
            ffmpeg_frame = avcodec_alloc_frame();
          avcodec_get_frame_defaults((AVFrame*)ffmpeg_frame);

          decoder_context->reordered_opaque = packet.pts;
          int result =
            avcodec_decode_video(decoder_context,
                                 (AVFrame*)ffmpeg_frame,
                                 &got_pic,
                                 packet.data,
                                 packet.size);

          if(!((AVFrame*)ffmpeg_frame)->data[0] || !result) got_pic = 0;

          if(got_pic){
            int64_t returned_pts =
              ((AVFrame *)ffmpeg_frame)->reordered_opaque-stream_start;
            //fprintf(stderr,"VIDEO: nominal pts=%.03f",
            //      1.*returned_pts*stream->time_base.num/stream->time_base.den);
            current_frame++;
            got_it = 1;
            break;
          }
        }
      }
    }

    //fprintf(stderr,"\n");
    if(error){
      ffmpeg_lock->unlock();
      return error;
    }
  }

  // Convert colormodel: Use ffmpeg, as it's not clear that the
  // quicktime code knows anything about alternate chroma siting
  if(got_it){

    FFMPEG::convert_cmodel((AVPicture *)ffmpeg_frame, decoder_context->pix_fmt,
                           decoder_context->width, decoder_context->height, frame);
  }else{
    /* No frame to transfer, but not due to an error; there really
       is no frame at this point. */
    //fprintf(stderr,"\nVIDEO: SENDING BACK NOTHING\n");
    unsigned char *buf=(unsigned char *)calloc(decoder_context->width*2,1);
    memset(buf+decoder_context->width,128,decoder_context->width);

    cmodel_transfer(frame->get_rows(), /* Leave NULL if non existent */
                    NULL,
                    frame->get_y(), /* Leave NULL if non existent */
                    frame->get_u(),
                    frame->get_v(),
                    buf,
                    buf+decoder_context->width,
                    buf+decoder_context->width,
                    0,        /* Dimensions to capture from input frame */
                    0,
                    decoder_context->width,
                    decoder_context->height,
                    0,       /* Dimensions to project on output frame */
                    0,
                    frame->get_w(),
                    frame->get_h(),
                    BC_YUV420P,
                    frame->get_color_model(),
                    0, /* When transfering BC_RGBA8888 to non-alpha
                          this is the background color in 0xRRGGBB
                          hex */
                    0,
                    frame->get_w());
    free(buf);
  }

  ffmpeg_lock->unlock();
  return error;
}

int FileFFMPEG::read_samples(double *buffer, int64_t len)
{
  int error = 0;
  ffmpeg_lock->lock("FileFFMPEG::read_samples");
  AVFormatContext *avcontext = (AVFormatContext *)ffmpeg_file_context;
  AVStream *stream = avcontext->streams[audio_index];
  AVCodecContext *decoder_context = stream->codec;
  int accumulation = 0;
  const int debug = 0;

  /* file start time, adjusted to the stream's timebase */
  int64_t stream_start = av_rescale_q(avcontext->start_time, AV_TIME_BASE_Q,
                                      stream->time_base);
  update_pcm_history(len);

  // Seek occurred
  if(decode_start != decode_end || unsynced) {

    //fprintf(stderr,"audio_pts SEEK requested=%.03lf(%.03lf) ",1.*decode_start / asset->sample_rate,
    //      1.*(decode_start+decode_len) / asset->sample_rate);


#define A_SEEK_THRESHOLD 1.
#define A_SEEK_BACK_START .2
#define A_SEEK_BACK_LIMIT 5.

    /* as with video, ffmpeg doesn't generally give us the specific
       PTSes we asked for.  It's usually several frames late. */

    int64_t target;
    double seek_back = A_SEEK_BACK_START;
    int pre_sync = 0;
    int got_it = 0;

    target = 1. * decode_start * stream->time_base.den / stream->time_base.num /
      asset->sample_rate + stream_start;

    unsynced = 0;
    while(!got_it && !error && seek_back <= A_SEEK_BACK_LIMIT){

      int64_t seekto = target - seek_back * stream->time_base.den / stream->time_base.num;
      accumulation = 0;

      if(seekto < stream_start){
        seekto = stream_start;
        seek_back = A_SEEK_BACK_LIMIT;
      }

      //fprintf(stderr,"adjusted=%.03lf ",1.f * (seekto-stream_start) * stream->time_base.num / stream->time_base.den);

      /* AV_SEEK_FLAG_ANY must be set for the AVI backend to seek in audio at all */
      if(av_seek_frame(avcontext,
                       audio_index,
                       seekto,
                       AVSEEK_FLAG_ANY)){
        error = 1;
        fprintf(stderr,"FileFFMPEG:read_samples SEEK FAILED!!!\n");
      }

      decode_end = decode_start;
      current_sample = file->current_sample;
      avcodec_flush_buffers(decoder_context);

      /* parallel video process; read forward until we see PTSes we
         like.  If we start too late, backtrack and try again. */

      while(!error && (!got_it || accumulation<decode_len)){
        AVPacket packet;

        error = av_read_frame(avcontext,&packet);

        if(!error && packet.size > 0 && packet.stream_index == audio_index){
          int packet_len = packet.size;
          uint8_t *packet_ptr = packet.data;
          int64_t pts_sample = AV_NOPTS_VALUE;

          if(packet.pts == AV_NOPTS_VALUE){
            fprintf(stderr,"FileFFMPEG::read_samples Missing Timestamps during SYNC!\n"
                    "\tAborting sync on this audio stream for now!\n");
            pts_sample = 1. * (seekto-stream_start) *
              stream->time_base.num*asset->sample_rate/stream->time_base.den;
          }else{
            pts_sample = 1. * (packet.pts-stream_start) *
              stream->time_base.num*asset->sample_rate/stream->time_base.den;
          }

          if(!pre_sync && packet.pts > target){
            if(seek_back*2.+EPSILON >= A_SEEK_BACK_LIMIT){
              /* we've searched as far back as permitted; we've found
                 no audio at the requested starting point so pad up to
                 this point and continue */
              int64_t padding = (packet.pts - stream_start) *
                asset->sample_rate * stream->time_base.num / stream->time_base.den -
                file->current_sample;

              /* don't assume the A/V sync offset will be less than
                 the maximum history depth */
              /* len and decode_len are equivalent here */
              if(padding<decode_len){
                pad_history(padding);
                accumulation += padding;
                pre_sync = 1;
                got_it = 1;
              }else{
                /* only account for the padding of the requested
                   range; we'll need to repeat this process next call */
                pad_history(decode_len);
                unsynced = 1;
                /* be sure */
                seek_back = A_SEEK_BACK_LIMIT+EPSILON;
                break;
                }
            }else{
              /* Normal case; backtrack and try again */
              break;
            }
          }

          //fprintf(stderr,"packet = %.03f:%d:%d:%d ",
          //      1.*(packet.pts-stream_start)/stream->time_base.den*stream->time_base.num,
          //      error,packet.size,packet.stream_index);

          while(packet_len > 0 && !error){
            int data_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
            if(!ffmpeg_samples) ffmpeg_samples = (short*)malloc(AVCODEC_MAX_AUDIO_FRAME_SIZE);
            int bytes_decoded = avcodec_decode_audio2(decoder_context,
                                                      ffmpeg_samples,
                                                      &data_size,
                                                      packet_ptr,
                                                      packet_len);
            if(bytes_decoded < 0){
              /* Do not set error; this is standard operating
                 procedure for eg mp3, where a packet often begins in
                 the middle of a frame.  Proper behavior is to handle
                 next packet. */
              break;
            }else{
              int samples_decoded = data_size /
                asset->channels /
                (av_get_bits_per_sample_format(decoder_context->sample_fmt)/8);

              packet_ptr += bytes_decoded;
              packet_len -= bytes_decoded;

              /* anything predating or at the target sample sets presync */
              if(!pre_sync && pts_sample <= file->current_sample){
                pre_sync = 1;
                //fprintf(stderr,"PRESYNC ");
              }

              /* Do any of the produced samples fall at the target or later? */
              if(got_it || pts_sample+samples_decoded > decode_start){
                if (!pre_sync) break;

                if(got_it || pts_sample>=decode_start){
                  /* all samples are past 'start'; use all samples */
                  append_history((void *)ffmpeg_samples,
                                 decoder_context->sample_fmt,
                                 0,
                                 samples_decoded);
                  accumulation += samples_decoded;
                }else{
                  /* decode partially precedes the range we want;
                     use only the samples at and past the start
                     marker */
                  int offset = decode_start - pts_sample;
                  if(offset<samples_decoded){
                    append_history((void *)ffmpeg_samples,
                                   decoder_context->sample_fmt,
                                   offset,
                                   samples_decoded-offset);
                    accumulation += samples_decoded-offset;

                  }
                }
                //if(!got_it)
                //fprintf(stderr,"delivered = %.03f(%.03f) ",
                //        1.*(packet.pts-stream_start)/stream->time_base.den*stream->time_base.num,
                //        1.*(packet.pts-stream_start)/stream->time_base.den*stream->time_base.num+
                //        1.*samples_decoded/asset->sample_rate);

                got_it=1;
              }
              pts_sample += samples_decoded;
            }
          }
        }
      }

      seek_back *= 2.;
      pre_sync = 0;
    }

    //fprintf(stderr,"\n");

  }else{

    /* nominal case; no seek, just keep reading samples */

    while(!error && accumulation<decode_len){
      AVPacket packet;

      error = av_read_frame(avcontext,&packet);

      if(!error && packet.size > 0 && packet.stream_index == audio_index){
        int packet_len = packet.size;
        uint8_t *packet_ptr = packet.data;
        int data_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
        if(!ffmpeg_samples) ffmpeg_samples = (short*)realloc(ffmpeg_samples, data_size);
        int bytes_decoded = avcodec_decode_audio2(decoder_context,
                                                    ffmpeg_samples,
                                                    &data_size,
                                                    packet_ptr,
                                                    packet_len);
        if(bytes_decoded < 0){
          error = 1;
        }else{
          int samples_decoded = data_size /
            asset->channels /
            (av_get_bits_per_sample_format(decoder_context->sample_fmt)/8);

          //if(accumulation==0)
            //fprintf(stderr,"nominal = %.03f(%.03f) ",
            //      1.*(packet.pts-stream_start)/stream->time_base.den*stream->time_base.num,
            //      1.*(packet.pts-stream_start)/stream->time_base.den*stream->time_base.num+
            //      1.*samples_decoded/asset->sample_rate);

          packet_ptr += bytes_decoded;
          packet_len -= bytes_decoded;

          append_history(ffmpeg_samples,
                         decoder_context->sample_fmt,
                         0,
                         samples_decoded);
          accumulation += samples_decoded;
        }
      }
    }

    /* at end of stream, audio may have ended early-- if we didn't get
       the full accumulation, pad */
    if(accumulation<decode_len){
      pad_history(decode_len-accumulation);
      //unsynced=1; error is set so this will be handled below
    }
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

