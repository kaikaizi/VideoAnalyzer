#include <cstdint>
#include <iostream>
#include <boost/thread.hpp>
#include <boost/utility.hpp>
extern "C"{
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libswscale/swscale.h>
}
#include <cv.h>
#include <highgui.h>

char errMsg[512], FourCC[]="DIV3";
inline void cvDisplay(volatile const int*count,const IplImage* img,
	CvVideoWriter*write){
   /* stage=0: prepare writer; 1: write a frame, including EOF */
   int cnt=0;
   while(*count>=0){
	if(cnt!=*count){
	   cnt=*count; cvShowImage("stream",img);
	   cvWriteFrame(write, img); 
	}
	cvWaitKey(5);
   }
   cvReleaseVideoWriter(&write);
}

class RtpDecoder:public boost::noncopyable{
public:
   explicit RtpDecoder(const char*strm)throw(std::runtime_error):stream_index(0),
	context(avformat_alloc_context()),ccontext(avcodec_alloc_context3(0))
   {init(strm);}
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
   void init(const char*)throw(std::runtime_error);
};
bool RtpDecoder::has_init;
void RtpDecoder::init(const char* strm)throw(std::runtime_error){
   if(!has_init){	/* invoked only once */
	has_init=true;
	av_register_all(); avformat_network_init();
   }
   if(avformat_open_input(&context,strm,0,0)){
	sprintf(errMsg,"RtpDecoder::init: cannot open %s.\n",strm);
	throw std::runtime_error(errMsg);
   }
   if(avformat_find_stream_info(context,0)<0){
	sprintf(errMsg,"RtpDecoder::init: Cannot find stream info.\n");
	throw std::runtime_error(errMsg);
   }
   while(stream_index<context->nb_streams &&
	   context->streams[stream_index]->codec->codec_type!=
	   AVMEDIA_TYPE_VIDEO)++stream_index;
   codec_rf=context->streams[stream_index]->codec;
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

// TODO: cannot figure out how to pipe read contents to output
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
	puts("Here");
   }
   void write_frame(AVPacket*const packet)throw(std::runtime_error){
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
	if(context->streams[cnt]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
	   context->streams[cnt]->sample_aspect_ratio.num=context->streams[cnt]->
		codec->sample_aspect_ratio.num;
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

class VideoCodec:public boost::noncopyable{
public: /* VideoDecoder must outlives this */
   explicit VideoCodec(RtpDecoder& rtp):rtp(rtp),pic(avcodec_alloc_frame()),
   picbgr(avcodec_alloc_frame()),width(rtp.getCodecContext()->width),
   height(rtp.getCodecContext()->height),picture_buf(reinterpret_cast<uint8_t*>
	   (av_malloc(avpicture_get_size(destFormat,width, height)))),
   img_context_ctx(sws_getContext(width,height,rtp.getCodecContext()->pix_fmt,width,
		height,destFormat,SWS_BICUBIC,0,0,0)),indx(0),check(0),tp(0),
   img(cvCreateImageHeader(cv::Size(width,height),8,3)){init();}
   ~VideoCodec(){
	av_free(pic); av_free(picbgr); av_free(picture_buf);
	sws_freeContext(img_context_ctx);
	cvReleaseImageHeader(&img);
	if(tp){
	   tp->join(); delete tp;
	}
	av_free_packet(&packet);
   }
   typedef void(*disp_handle_t)(volatile const int*,const IplImage*,CvVideoWriter*);
   void operator()(disp_handle_t handle,const char*fname){
	CvVideoWriter *write;
	if(!tp&&handle){
	   check_exist(fname);
	   tp=new boost::thread(boost::bind(handle,&indx,img,write=cvCreateVideoWriter(
			   fname,CV_FOURCC(FourCC[0],FourCC[1],FourCC[2],FourCC[3]),15,
			   cv::Size(width,height),3)));
	}
	while(av_read_frame(rtp.getFormatContext(),&packet)>=0)
	   if(packet.stream_index==rtp.getStreamIndex() &&
		   avcodec_decode_video2(rtp.getCodecContext(),pic,&check,&packet)>=0
		   && check){
		puts("OK");
		if(tp)sws_scale(img_context_ctx,pic->data,pic->linesize,0,height,
			picbgr->data,picbgr->linesize);
		if(++indx>=std::numeric_limits<int>::max())indx=0;
	   }
	indx=-1; /* signals finish */
   }
   const AVPacket& getPacket()const{return packet;}
protected:
   RtpDecoder& rtp;
   AVFrame* pic, *picbgr;
   const int &width,&height;
   uint8_t* picture_buf;
   SwsContext* img_context_ctx;
   int indx,check;
   boost::thread* tp;
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
	   fprintf(stderr,"Warning: overwriting existing file %s.\n",fname);
	   fclose(fp);
	}
   }
};
enum AVPixelFormat VideoCodec::destFormat=PIX_FMT_BGR24;

int main(int argc, char* argv[]) {
   if(argc-3)return
	fprintf(stderr,"Usage: %s rtsp://xxx.yy.zz.m:port/file output_file\n",
		argv[0]);
   try{
	RtpDecoder dec(argv[1]);
	VideoCodec vd(dec);
	vd(cvDisplay,argv[2]);
// 	VideoCopier cp(dec,"new.avi");
// 	vd(cvDisplay,&cp);
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
