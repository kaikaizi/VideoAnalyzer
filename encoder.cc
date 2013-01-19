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

#include <xvid.h>
#include "encoder.hh"
#include "dynan.hh"
extern char msg[256];
extern bool verbose;

struct Encoder::Xvid_t{
   Xvid_t(const cv::Size&, const int&);
   ~Xvid_t(){
	if(!xvid_encore(&enc.handle,XVID_ENC_DESTROY,0,0))	// XXX
	   fprintf(stderr,"encoder dtor: encoder destruction failed.");
	assert(0);
	if(!xvid_decore(&dec.handle,XVID_DEC_DESTROY,0,0))
	   fprintf(stderr,"encoder dtor: decoder destruction failed.");
   }
   inline void dump_stat(bool decoder)const;
   inline static void retCode(int);
   xvid_gbl_init_t gbl_init;
   xvid_dec_create_t dec;
   xvid_enc_create_t enc;
   xvid_enc_stats_t enc_stat;
   xvid_dec_stats_t dec_stat;
   xvid_enc_frame_t enc_frame;
   xvid_dec_frame_t dec_frame;
   const int sz;
};

Encoder::Xvid_t::Xvid_t(const cv::Size& size, const int& f):sz(size.width*size.height){
   memset(&gbl_init,0,sizeof(xvid_gbl_init_t));
   memset(&enc,0,sizeof(xvid_enc_create_t)); memset(&dec,0,sizeof(xvid_dec_create_t));
   memset(&enc_stat,0,sizeof(xvid_enc_stats_t));
   memset(&dec_stat,0,sizeof(xvid_dec_stats_t));
   memset(&enc_frame,0,sizeof(xvid_enc_frame_t));
   memset(&dec_frame,0,sizeof(xvid_dec_frame_t));
   gbl_init.debug = XVID_DEBUG_ERROR | XVID_DEBUG_STARTCODE |
	XVID_DEBUG_HEADER | XVID_DEBUG_TIMECODE;
   dec.fourcc=f;
   enc.width=dec.width=size.width; enc.height=dec.height=size.height;
   enc.version = dec.version = gbl_init.version = enc_stat.version =
	dec_stat.version = enc_frame.version = dec_frame.version = XVID_VERSION;
   enc.fincr=1; enc.fbase=15;	// fps=fincr/fbase ?
   enc.profile = XVID_PROFILE_S_L0; enc.global = XVID_GLOBAL_PACKED;
   dec_stat.type = XVID_TYPE_NOTHING;
   enc_frame.vop_flags = XVID_VOP_INTER4V | XVID_VOP_TOPFIELDFIRST |
	XVID_VOP_ALTERNATESCAN | XVID_VOP_RD_BVOP | XVID_VOP_DEBUG;
   enc_frame.vol_flags = XVID_VOL_MPEGQUANT | XVID_VOL_EXTRASTATS |
	XVID_VOL_QUARTERPEL | XVID_VOL_GMC | XVID_VOL_INTERLACING;
   enc_frame.motion = XVID_ME_ADVANCEDDIAMOND16 | XVID_ME_ADVANCEDDIAMOND8;
   enc_frame.type = XVID_TYPE_AUTO;
}

void Encoder::Xvid_t::dump_stat(bool decoder)const{
   if(decoder){
	if(dec_stat.type>0)printf("Decoder general flags=%x, time_base=%d, "
		"time_increment=%d, qscale_stride=%d\n", dec_stat.data.vop.general,
		dec_stat.data.vop.time_base, dec_stat.data.vop.time_increment,
		dec_stat.data.vop.qscale_stride);
	else printf("Decoder general flags=%x, width=%d, height=%d, par=%d, "
		"par_width=%d, par_heigh=%d\n", dec_stat.data.vol.general,
		dec_stat.data.vol.width, dec_stat.data.vol.height, dec_stat.data.vol.par,
		dec_stat.data.vol.par_width, dec_stat.data.vol.par_height);
   }else printf("Encoder type=%d, frame quantizer=%d, vol_flags=%x, vop_flags=%x, "
	   "frame length=%d, header lenght=%d bytes, #block_intra=%d, #block_inter=%d, "
	   "#block_notCoded=%d, SSE_YUV=(%d %d %d)\n", enc_stat.type, enc_stat.quant,
	   enc_stat.vol_flags, enc_stat.vop_flags, enc_stat.length, enc_stat.hlength,
	   enc_stat.kblks, enc_stat.mblks, enc_stat.ublks, enc_stat.sse_y,
	   enc_stat.sse_u, enc_stat.sse_v);
}

void Encoder::Xvid_t::retCode(int code){
   if(code<0)switch(code){
	case XVID_ERR_FAIL: fputs("general fault\n", stderr); return;
	case XVID_ERR_MEMORY: fputs("memory allocation error\n", stderr); return;
	case XVID_ERR_FORMAT: fputs("file format error\n", stderr); return;
	case XVID_ERR_VERSION: fputs("structure version not supported\n", stderr); return;
	case XVID_ERR_END: fputs("encoder: end of stream reached\n", stderr); return;
	default: fprintf(stderr,"unknown error code %d\n",code);
   }
}

Encoder::Encoder(const char* nm[2],const VideoProp& prop, const int& codec)
   throw(ErrMsg):rem(0),used(1),px(new Xvid_t(prop.prop.size,codec)),
   stream(new char[buf_size]),plane(new char[px->sz*4]),fsrc(fopen(nm[0],"rb")),
   fdest(fopen(nm[1],"wb")){
   if(!nm || !nm[0] || !nm[1])throw
	ErrMsg("encoder ctor: invalid input/output file names.");
   if(!fsrc || !fdest){
	sprintf(msg,"encoder ctor: cannot open file %s for %s.\n",
		fsrc?nm[1]:nm[0], fsrc?"writing":"reading");
	if(fsrc)fclose(fsrc);
	else if(fdest)fclose(fdest);
	throw ErrMsg(msg);
   }
   if(xvid_global(0,XVID_GBL_INIT, &px->gbl_init,0))
	throw ErrMsg("ctor::xvid_global failed.");
   px->enc_frame.bitstream=px->dec_frame.bitstream=stream;
   px->enc_frame.length=px->dec_frame.length=buf_size;
   if(xvid_decore(0,XVID_DEC_CREATE, &px->dec,0))
	throw ErrMsg("encoder ctor: decoder creation failed.");
   px->enc_frame.input.stride[0] = px->dec_frame.output.stride[0] = px->dec.width;
   px->enc_frame.input.plane[1] = px->dec_frame.output.plane[1] = reinterpret_cast<char*>
	(px->enc_frame.input.plane[0]=px->dec_frame.output.plane[0]=plane) + px->sz;
   px->enc_frame.input.plane[2] = px->dec_frame.output.plane[2] = reinterpret_cast<char*>
	(px->dec_frame.output.plane[1]) + px->sz;
   px->enc_frame.input.plane[3] = px->dec_frame.output.plane[3] = reinterpret_cast<char*>
	(px->dec_frame.output.plane[2]) + px->sz;
   if(xvid_encore(0, XVID_ENC_CREATE, &px->enc, 0))
	throw ErrMsg("encoder ctor: encoder creation failed.");
   if(::verbose)puts("Converting...");
//    int nb=xvid_encore(px->enc.handle, XVID_ENC_ENCODE, &px->enc_frame, &px->enc_stat);
//    printf("nb=%d\n",nb);
//    for(int indx=0; indx<15; ++indx)fwrite(stream,1,used,fdest);

   while(used){
	if(!(consumed=fread(stream,1,buf_size-rem,fsrc)))break;
	rem += consumed - (used=xvid_decore(px->dec.handle, XVID_DEC_DECODE,
		   &px->dec_frame, &px->dec_stat));
	if(used<0){
	   fputs("Decoder: ",stderr);
	   Xvid_t::retCode(used); break;
	}else if(used)
	   if((ret=xvid_encore(px->enc.handle, XVID_ENC_ENCODE, &px->enc_frame,
		&px->enc_stat)) < 0){
		fputs("Encoder: ",stderr); Xvid_t::retCode(ret);
	   }
	   else fwrite(stream,1,used,fdest);
   }
   if(::verbose)puts("Done");
}

Encoder::~Encoder(){
   fclose(fsrc); fclose(fdest);
   delete px;
   delete[]plane; delete[]stream;
}
