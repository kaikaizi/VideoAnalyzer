/*  Copyright (C) 2012  Liu Lukai	(liulukai@gmail.com)
    This file is part of VideoAnalyzer.

    VideoAnalyzer is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/ 
#pragma once
#include "hist.hh"
#include <boost/smart_ptr.hpp>
extern "C"{
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libswscale/swscale.h>
}
extern char msg[256];

struct cvDisplay:boost::noncopyable{
   typedef int64_t value_type;
   cvDisplay(volatile const unsigned*count,const IplImage* img, volatile const
	   value_type* pts, CvVideoWriter*write,const char* url):cnt(0),write(write),
   display(img,url,count),t(boost::ref(display)),vote(pts){}
   ~cvDisplay(){
	if(write)cvReleaseVideoWriter(&write);
	join_thread(t);
   }
   void operator()();
protected:
   unsigned cnt;
   CvVideoWriter* write;
   struct Display{  /* avoid conflicting w. frame writer */
	volatile const unsigned* count;
	const IplImage*image;
	const char* banner;
	Display(const IplImage* img,const char* banner,volatile const unsigned* count):
	   count(count),image(img),banner(banner){}
	void operator()()const;
   } display;
   boost::thread t;
   /* ugly hack for avcodec pts: frame drops */
   template<typename value_type>struct Voter:boost::noncopyable{
	explicit Voter(volatile const value_type*pts):pts(pts),winner(0),
	prev(0),key(0),age(0),prevFrames(0){}
	int operator()();
   protected:
	volatile const value_type* pts;
	value_type winner,prev,key;   /* currently determined pts */
	std::vector<value_type> Keys;
	int age, prevFrames;
	const static int timeout=8, max_delay_frame=60;
   };
   Voter<value_type> vote;
};

class RtpDecoder:boost::noncopyable{
public:
   const char* name;
   explicit RtpDecoder(const char*strm)throw(std::runtime_error):name(strm),
   stream_index(0),context(avformat_alloc_context()),
   ccontext(avcodec_alloc_context3(0)){init();}
   ~RtpDecoder(){
	av_dict_free(&ccontext->metadata);
	avformat_close_input(&context);
	avformat_free_context(context);
	avcodec_close(ccontext);
   }
   AVCodecContext* getCodecContext(){return ccontext;}
   AVFormatContext* getFormatContext(){return context;}
   const AVCodecContext*const getCodecContext()const{return ccontext;}
   const AVFormatContext*const getFormatContext()const{return context;}
   const AVCodec*const getCodec()const{return codec;}
   const int& getStreamIndex()const{return stream_index;}
   static bool initialized(){return has_init;}
protected:
   static bool has_init;
   AVFormatContext* context;
   AVCodecContext *ccontext, *codec_rf;
   AVCodec* codec;
   int stream_index;
   void init()throw(std::runtime_error);
};

// TODO: cannot figure out how to pipe read packets to output
class VideoCopier:boost::noncopyable{
public:
   VideoCopier(const RtpDecoder& rtp, const char*fname)throw(std::runtime_error):
	rtp(rtp),context(avformat_alloc_context()),ccontext(avcodec_alloc_context3(0)),
	codec_rf(rtp.getFormatContext()->streams[rtp.getStreamIndex()]->codec),
	codec(avcodec_find_encoder(codec_rf->codec_id)),
	fmt(av_guess_format(0,fname,0)){init(fname);}
   ~VideoCopier(){
	av_write_trailer(context);
	av_dict_free(&ccontext->metadata);
	avcodec_close(ccontext);
	if(fmt&&!(fmt->flags&AVFMT_NOFILE)&&context->pb)
	   avio_close(context->pb);
	avformat_free_context(context);
   }
   void write_frame(AVPacket*const packet)throw(std::runtime_error){
	// crashes before here, after init()
	if(av_write_frame(context,packet)<0){
	   sprintf(msg,"RtpDecoder::init: open2 failed.\n");
	   throw std::runtime_error(msg);
	}
   }
protected:
   const RtpDecoder& rtp;
   AVFormatContext* context;
   AVCodecContext *ccontext, *codec_rf;
   const AVCodec* codec;
   AVOutputFormat* fmt;
   void init(const char*)throw(std::runtime_error);
};
/* NOTE: line 341-359, ffmpeg/libavformats/rtpdec_jpeg.c for
 * error msgs on frame dropping */
class VideoCodec:boost::noncopyable{
public: /* VideoDecoder must outlives this */
   explicit VideoCodec(RtpDecoder& rtp):rtp(rtp),pic(avcodec_alloc_frame()),
   picbgr(avcodec_alloc_frame()),width(rtp.getCodecContext()->width),
   height(rtp.getCodecContext()->height),picture_buf(reinterpret_cast<uint8_t*>
	   (av_malloc(avpicture_get_size(destFormat,width, height)))),
   img_context_ctx(sws_getContext(width,height,rtp.getCodecContext()->pix_fmt,width,
		height,destFormat,SWS_BICUBIC,0,0,0)),indx(1),check(0),
   img(cvCreateImageHeader(cv::Size(width,height),8,3)){init();}
   ~VideoCodec(){
	av_free(pic); av_free(picbgr); av_free(picture_buf);
	sws_freeContext(img_context_ctx);
	cvReleaseImageHeader(&img);
	join_thread(*tp);
	av_free_packet(&packet);
   }
   void operator()(bool show,const char*,VideoCopier*);
   const AVPacket& getPacket()const{return packet;}
protected:
   RtpDecoder& rtp;
   AVFrame* pic, *picbgr;
   const int &width,&height;
   uint8_t* picture_buf;
   SwsContext* img_context_ctx;
   int check;
   unsigned indx;
   boost::shared_ptr<boost::thread> tp;
   boost::shared_ptr<cvDisplay> display;
   IplImage* img;
   AVPacket packet;
   static enum AVPixelFormat destFormat;	/* line 66, libavutil/pixfmt.h */
   static char Codec[5];
   friend class loadConf;
   void init(){
	av_init_packet(&packet);
	avpicture_fill(reinterpret_cast<AVPicture*>(picbgr),picture_buf,destFormat,
		width,height);
	img->imageData=reinterpret_cast<char*>(picbgr->data[0]);
   }
};

int rtsp_client(const char* url, const char* fname);
