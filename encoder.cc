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

#include "encoder.hh"
extern bool verbose;

void cvDisplay::operator()(){
   while(*display.count)
	if(cnt!=*display.count&&(cnt=*display.count)&&write)
	   for(int indx=vote(); indx>0; --indx)
		cvWriteFrame(write,display.image);
}
void cvDisplay::Display::operator()()const{
   while(*count){
	cvShowImage(banner,image);
	cvWaitKey(5);
   }
}
template<typename T>
int cvDisplay::Voter<T>::operator()(){
   if(*pts<=0)return 0;
   const value_type ckey=*pts-prev; prev=*pts;
   if(ckey<=0)return 0;
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
   int cur=std::min(max_delay_frame,
	   static_cast<int>(round((ckey+0.)/winner)));
   return cur==max_delay_frame && prevFrames==max_delay_frame ?
	0 : prevFrames=cur;
}

bool RtpDecoder::has_init;
void RtpDecoder::init()throw(std::runtime_error){
   if(!has_init){	/* invoked only once */
	has_init=true;
	av_log_set_level(AV_LOG_QUIET); /* line 130, libavutil/log.h */
	av_register_all(); avformat_network_init();
   }
   if(avformat_open_input(&context,name,0,0)){
	sprintf(msg,"RtpDecoder::init: cannot open %s.\n",name);
	throw std::runtime_error(msg);
   }
   if(avformat_find_stream_info(context,0)<0){
	sprintf(msg,"RtpDecoder::init: Cannot find stream info.\n");
	throw std::runtime_error(msg);
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
	sprintf(msg,"RtpDecoder::init: Codec not found.\n");
	throw std::runtime_error(msg);
   }
   codec_rf->flags|=codec->capabilities&CODEC_CAP_TRUNCATED;
   avcodec_get_context_defaults3(ccontext,codec);
   avcodec_copy_context(ccontext,codec_rf);
   av_dict_set(&ccontext->metadata,"b","2M",0);
   if(avcodec_open2(ccontext,codec,&ccontext->metadata)<0){
	sprintf(msg,"RtpDecoder::init: open2 failed.\n");
	throw std::runtime_error(msg);
   }
}

enum AVPixelFormat VideoCodec::destFormat=PIX_FMT_BGR24;
char VideoCodec::Codec[5]="DIV3";
void VideoCopier::init(const char*fname)throw(std::runtime_error){
   if(!fmt){
	sprintf(msg,"VideoCopier::init: Output format not found.\n");
	throw std::runtime_error(msg);
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
   sprintf(msg,"%dx%d",ccontext->width,ccontext->height);
   av_dict_set(&ccontext->metadata,"video_size",msg,0);
   if(!(fmt->flags&AVFMT_NOFILE)&&avio_open(&context->pb,context->filename,
		AVIO_FLAG_WRITE)<0){
	sprintf(msg,"VideoCopier::init: avio open failed.\n");
	throw std::runtime_error(msg);
   }
   for(int cnt=0;cnt<context->nb_streams;++cnt)
	if(context->streams[cnt]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
	   context->streams[cnt]->sample_aspect_ratio.num=context->streams[cnt]->
		codec->sample_aspect_ratio.num;
	   context->streams[cnt]->sample_aspect_ratio.den=context->streams[cnt]->
		codec->sample_aspect_ratio.den;
	}
   if(!codec){
	sprintf(msg,"VideoCopier::init: Codec not found.\n");
	throw std::runtime_error(msg);
   }else if(avcodec_open2(ccontext,codec,&ccontext->metadata)<0){
	sprintf(msg,"VideoCopier::init: open2 failed.\n");
	throw std::runtime_error(msg);
   }else if(avformat_write_header(context,&ccontext->metadata)<0){
	sprintf(msg,"VideoCopier::init: cannot write header.\n");
	throw std::runtime_error(msg);
   }
}
void VideoCodec::operator()(bool show,const char*fname,VideoCopier*vc){
   CvVideoWriter *write=vc||!fname ? 0:
	cvCreateVideoWriter(fname,CV_FOURCC(Codec[0],Codec[1],
		   Codec[2],Codec[3]),15,cv::Size(width,height),3);
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

int rtsp_client(const char* url, const char* fname){
   RtpDecoder dec(url);
   VideoCodec vd(dec);
   vd(true,strcmp(fname,"-")?fname:0,0);
   return 0;
}

// AVOutputFormat: line 380, libavformat/avformat.h
// AVStream: line 646
// AVFormatContext: line 941
// AVFrame: line 1099, libavformat/avformat.h
// AVCodecContext: line 1561 libavcodec/avcodec.h
