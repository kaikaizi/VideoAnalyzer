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
#include <cstdint>
#include <iostream>
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <boost/foreach.hpp>
#include <boost/smart_ptr.hpp>
extern "C"{
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libswscale/swscale.h>
}
#include <cv.h>
#include <highgui.h>

#define For BOOST_FOREACH

char errMsg[512], FourCC[]="DIV3";

struct cvDisplay:public boost::noncopyable{
   typedef int64_t value_type;
   cvDisplay(volatile const unsigned*count,const IplImage* img, volatile const
	   value_type* pts, CvVideoWriter*write,const char* url):cnt(0),write(write),
   display(img,url,count),t(boost::ref(display)),vote(pts){}
   ~cvDisplay(){
	if(write)cvReleaseVideoWriter(&write);
	if(t.joinable())t.join();
   }
   void operator()(){
	while(*display.count)
	   if(cnt!=*display.count){
		cnt=*display.count;
		if(write)
		   for(int indx=vote(); indx; --indx)
			cvWriteFrame(write,display.image);
	   }
   }
protected:
   unsigned cnt;
   CvVideoWriter* write;
   struct Display{  /* avoid conflicting w. frame writer */
	volatile const unsigned* count;
	const IplImage*image;
	const char* banner;
	Display(const IplImage* img,const char* banner,volatile const unsigned* count):
	   count(count),image(img),banner(banner){}
	void operator()()const{
	   while(*count){
		cvShowImage(banner,image);
		cvWaitKey(5);
	   }
	}
   } display;
   boost::thread t;
   /* ugly hack for avcodec pts: frame drops */
   template<typename value_type>struct Voter:public boost::noncopyable{
	explicit Voter(volatile const value_type*pts):pts(pts),winner(0),
	prev(0),key(0),age(0){}
	int operator()(){
	   if(*pts<=0)return 1;
	   const value_type ckey=*pts-prev; prev=*pts;
	   if(ckey<=0)return 1;
	   bool found=false;
	   For(value_type iter, Keys)
		if(fabs(ckey-iter)/iter<.4){
		   found=true; break;
		}
	   if(!found)Keys.push_back(ckey);
	   if(++age>timeout || !key){
		age=0;
		winner=*std::min_element(Keys.begin(),Keys.end());
	   }
	   return static_cast<int>(round((ckey+0.)/winner));
	}
   protected:
	volatile const value_type* pts;
	value_type winner,prev,key;   /* currently determined pts */
	std::vector<value_type> Keys;
	int age;
	const static int timeout=8;
   };
   Voter<value_type> vote;
};

class RtpDecoder:public boost::noncopyable{
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
bool RtpDecoder::has_init;
void RtpDecoder::init()throw(std::runtime_error){
   if(!has_init){	/* invoked only once */
	has_init=true;
	av_log_set_level(AV_LOG_QUIET); /* line 130, libavutil/log.h */
	av_register_all(); avformat_network_init();
   }
   if(avformat_open_input(&context,name,0,0)){
	sprintf(errMsg,"RtpDecoder::init: cannot open %s.\n",name);
	throw std::runtime_error(errMsg);
   }
   if(avformat_find_stream_info(context,0)<0){
	sprintf(errMsg,"RtpDecoder::init: Cannot find stream info.\n");
	throw std::runtime_error(errMsg);
   }
   while(stream_index<context->nb_streams &&
	   context->streams[stream_index]->codec->codec_type!=
	   AVMEDIA_TYPE_VIDEO)
// 	AVDISCARD_NONE AVDISCARD_DEFAULT AVDISCARD_NONREF AVDISCARD_BIDIR
// 	   AVDISCARD_NONKEY AVDISCARD_ALL
// 	context->streams[stream_index]->discard=AVDISCARD_BIDIR;
	++stream_index;
   codec_rf=context->streams[stream_index]->codec;
   context->streams[stream_index]->cur_dts=AV_NOPTS_VALUE;
   context->flags|=AVFMT_FLAG_DISCARD_CORRUPT|AVFMT_FLAG_MP4A_LATM;
   av_read_play(context);
   if(!(codec=avcodec_find_decoder(codec_rf->codec_id))){
	sprintf(errMsg,"RtpDecoder::init: Codec not found.\n");
	throw std::runtime_error(errMsg);
   }
   codec_rf->flags|=codec->capabilities&CODEC_CAP_TRUNCATED;
   avcodec_get_context_defaults3(ccontext,codec);
   avcodec_copy_context(ccontext,codec_rf);
   av_dict_set(&ccontext->metadata,"b","2M",0);
   if(avcodec_open2(ccontext,codec,&ccontext->metadata)<0){
	sprintf(errMsg,"RtpDecoder::init: open2 failed.\n");
	throw std::runtime_error(errMsg);
   }
}

// TODO: cannot figure out how to pipe read packets to output
class VideoCopier:public boost::noncopyable{
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
	   sprintf(errMsg,"RtpDecoder::init: open2 failed.\n");
	   throw std::runtime_error(errMsg);
	}
   }
protected:
   const RtpDecoder& rtp;
   AVFormatContext* context;
   AVCodecContext *ccontext, *codec_rf;
   const AVCodec* codec;
   AVOutputFormat* fmt;
   void init(const char*fname)throw(std::runtime_error);
};
void VideoCopier::init(const char*fname)throw(std::runtime_error){
   if(!fmt){
	sprintf(errMsg,"VideoCopier::init: Output format not found.\n");
	throw std::runtime_error(errMsg);
   }
   memcpy(context,rtp.getFormatContext(),sizeof*rtp.getFormatContext());
   memcpy(ccontext,rtp.getCodecContext(),sizeof*rtp.getCodecContext());
   fmt->video_codec=context->video_codec_id;
   fmt->audio_codec=context->audio_codec_id;
   fmt->subtitle_codec=context->subtitle_codec_id;
   fmt->video_codec=context->video_codec_id;
   context->oformat=fmt; context->iformat=0;
   snprintf(context->filename,sizeof(context->filename),"%s",fname);
   av_dict_set(&ccontext->metadata,"b","2M",0);
   sprintf(errMsg,"%dx%d",ccontext->width,ccontext->height);
   av_dict_set(&ccontext->metadata,"video_size",errMsg,0);
   if(!(fmt->flags&AVFMT_NOFILE)&&avio_open(&context->pb,context->filename,
		AVIO_FLAG_WRITE)<0){
	sprintf(errMsg,"VideoCopier::init: avio open failed.\n");
	throw std::runtime_error(errMsg);
   }
   for(int cnt=0;cnt<context->nb_streams;++cnt)
	if(context->streams[cnt]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
	   context->streams[cnt]->sample_aspect_ratio.num=context->streams[cnt]->
		codec->sample_aspect_ratio.num;
	   context->streams[cnt]->sample_aspect_ratio.den=context->streams[cnt]->
		codec->sample_aspect_ratio.den;
	}
   if(!codec){
	sprintf(errMsg,"VideoCopier::init: Codec not found.\n");
	throw std::runtime_error(errMsg);
   }else if(avcodec_open2(ccontext,codec,&ccontext->metadata)<0){
	sprintf(errMsg,"VideoCopier::init: open2 failed.\n");
	throw std::runtime_error(errMsg);
   }else if(avformat_write_header(context,&ccontext->metadata)<0){
	sprintf(errMsg,"VideoCopier::init: cannot write header.\n");
	throw std::runtime_error(errMsg);
   }
}

/* NOTE: line 341-359, ffmpeg/libavformats/rtpdec_jpeg.c for
 * error msgs on frame dropping */
class VideoCodec:public boost::noncopyable{
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
	if(tp)tp->join();
	av_free_packet(&packet);
   }
   void operator()(bool show,const char*fname,VideoCopier*vc=0){
	CvVideoWriter *write=vc||!fname ? 0:
	   cvCreateVideoWriter(fname,CV_FOURCC(FourCC[0],FourCC[1],FourCC[2],
			FourCC[3]),15,cv::Size(width,height),3);
	if(show){
	   display=boost::shared_ptr<cvDisplay>(new cvDisplay(&indx,img,
			&packet.pts,write,rtp.name));
	   check_exist(fname);
	   tp=boost::shared_ptr<boost::thread>(new boost::thread(
			boost::ref(*display)));
	}
	while(!av_read_frame(rtp.getFormatContext(),&packet)){
	   if(vc)vc->write_frame(&packet);
	   if(packet.stream_index==rtp.getStreamIndex() &&
		   avcodec_decode_video2(rtp.getCodecContext(),pic,&check,&packet)>=0
		   && check){
		if(tp)sws_scale(img_context_ctx,pic->data,pic->linesize,0,height,
			picbgr->data,picbgr->linesize);
		if(++indx==std::numeric_limits<unsigned>::max())indx=1;
	   }
	}
	indx=0; /* signals finish */
   }
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
   void init(){
	av_init_packet(&packet);
	avpicture_fill(reinterpret_cast<AVPicture*>(picbgr),picture_buf,destFormat,
		width,height);
	img->imageData=reinterpret_cast<char*>(picbgr->data[0]);
   }
   static void check_exist(const char*fname){
	FILE* fp=fopen(fname,"r");
	if(fp){
	   fprintf(stderr,"Warning: overwriting file %s.\n",fname);
	   fclose(fp);
	}
   }
};
enum AVPixelFormat VideoCodec::destFormat=PIX_FMT_BGR24;

int main(int argc, char* argv[]){
   if(argc<2)return fprintf(stderr,
	   "Usage: %s rtsp://xxx.yy.zz.m:port/file [output_file]\n", argv[0]);
   try{
	RtpDecoder dec(argv[1]);
	VideoCodec vd(dec);
	vd(true,argc>2?argv[2]:0,0);
/* 	if(argc>2){
	VideoCopier vc(dec,argv[2]);
	   vd(cvDisplay,argv[2],&vc);
 	}*/
   }catch(const std::exception ex){
	fprintf(stderr,"Error: %s\n%s",ex.what(),errMsg);
	return 1;
   }
}

// AVOutputFormat: line 380, libavformat/avformat.h
// AVStream: line 646
// AVFormatContext: line 941
// AVFrame: line 1099, libavformat/avformat.h
// AVCodecContext: line 1561 libavcodec/avcodec.h
