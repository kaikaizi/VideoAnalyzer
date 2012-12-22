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
#include "dynan.hh"
#include <fstream>
#include <ctime>
#include <iostream>
#include <boost/foreach.hpp>

extern char msg[256];
//++++++++++++++++++++++++++++++++++++++++
dynan::dynan(const IplImage** ppsrc, IplImage** ppdest, const unsigned& srcNum,
	const unsigned& destNum, const unsigned& insertLoc, const InterpMetric& ic,
	const InterpMethod& id, const short& aptSz)throw(ErrMsg): iFrameNum(srcNum),
   oFrameNum(destNum), width(ppsrc[0]->width), height(ppsrc[0]->height),
   imageSize(ppsrc[0]->imageSize), pDestImg(ppdest),
   iLoc(insertLoc), apertureSz(aptSz) {
	// insertLoc=1 --> insert after 1st image;
   // Sanity check
   if(iFrameNum<2) throw ErrMsg("dynan::ctor: input volume < 2.\n");
   if(ipd==Quad && iFrameNum<3)
	throw ErrMsg("dynan::ctor: input image volume < 3 with Quad method.\n");
   if(!iLoc || iLoc>=iFrameNum) {
	iLoc? sprintf(msg, "dynan::ctor: input image volume depth %d < insert location"
		" %d.\n", srcNum, insertLoc) :
	   sprintf(msg, "dynan::ctor: input image volume depth=0.\n");
	throw ErrMsg(msg);
   }
   int index;
   for(index=1; index<srcNum; ++index)
	if(ppsrc[index]->width != width ||
		ppsrc[index]->height != height) {
	   sprintf(msg,"dynan::ctor: input images %d/%d size unequal([%d %d]!=[%d %d]",
		   index-1, index, width, height, ppsrc[index]->width,
		   ppsrc[index]->height);
	   throw ErrMsg(msg);
	}
   for(index=0; index<destNum; ++index)
	if(ppdest[index]->width != width || ppdest[index]->height != height) {
	   sprintf(msg, "dynan::ctor: output images %d size unequal([%d %d]!=[%d %d])",
		   index, ppdest[index]->width, ppdest[index]->height, width, height);
	   throw ErrMsg(msg);
	}
   // end of Sanity check
   setIntMethod(ic,id);
   for(int index=0; index<srcNum; ++index) vSrcImg.push_back(*(ppsrc+index));
}

void dynan::setIntMethod(const InterpMetric& ic, const InterpMethod& id) {
   ipc = ic==cDefault? Pix:ic;
   ipd = id==dDefault? Linear:id;
}

void dynan::interpolate() {
   const cv::Size winSz(apertureSz, apertureSz); // aperture size used for optical flow
   if(ipc==Lucas_Kanades || ipc==Horn_Schunck ||
	   ipc==Block_Matching){	// classical methods
	const int bmSz=16, wid_used = ipc==Block_Matching? width/bmSz : width,
		hit_used = ipc==Block_Matching? height/bmSz : height;
	IplImage* velx=cvCreateImage(cvSize(wid_used,hit_used), IPL_DEPTH_32F,
		vSrcImg[0]->nChannels), *vely=cvCreateImage(cvSize(wid_used,hit_used),
		   IPL_DEPTH_32F, vSrcImg[0]->nChannels);
	if(ipc!=Block_Matching)
	   regInterp(velx, vely);
	cvReleaseImage(&velx);
	cvReleaseImage(&vely);
   } else if(ipc==Pyr_LK){	   // pyramid-methods
	unsigned char* prev_pyr = new unsigned char[(8+width)*height/3],
		   *cur_pyr = new unsigned char[(8+width)*height/3];
	CvPoint2D32f* prevPts=new CvPoint2D32f[imageSize],
	   *nextPts = new CvPoint2D32f[imageSize];
	char* status = new char[imageSize];
	float* err = new float[imageSize];
	for(int y=0; y<height; ++y)
	   for(int x=0; x<width; ++x)prevPts[y*width+x] = cv::Point(x,y);

	try {
	   cvCalcOpticalFlowPyrLK(vSrcImg[iLoc-1], vSrcImg[iLoc],
		   NULL/*prev_pyr*/, NULL/*cur_pyr*/,
		   prevPts, nextPts, imageSize, winSz, 3, status, err,
		   cv::TermCriteria(cv::TermCriteria::COUNT +
			cv::TermCriteria::EPS, 30,.1), 0);
	} catch(...) {
	   printf("Exception thrown when using cvCalcOpticalFlowPyrLK.\n");
	   delete[] prev_pyr; delete[] cur_pyr;
	   delete[] prevPts; delete[] nextPts;
	   delete[] status; delete[] err;
	   throw;
	}
   } else{	   // pixel-wise interploation
	unsigned char iFrame[iFrameNum], oFrame[oFrameNum];
	int index, dataIndex;
	for(int y=dataIndex=0; y<pDestImg[0]->height; ++y)
	   for(int x=0; x<pDestImg[0]->width; ++x, ++dataIndex) {
		for(index=0; index<iFrameNum; ++index)
		   iFrame[index] = vSrcImg[index]->imageData[dataIndex];
		interp<unsigned char>(iFrame, oFrame);
		for(index=0; index<oFrameNum; ++index)
		   pDestImg[index]->imageData[dataIndex] = oFrame[index];
	   }
   }
}

template<typename T>
void dynan::interp(const T* src, T* dest)const {
   int index; float grayVal, stepVal;
   switch(ipd) {
	case ZeroHold:
	   for(index=0; index<oFrameNum; ++index) dest[index]=src[iLoc-1];
	   break;
	case Linear:
	   stepVal = static_cast<float>(src[iLoc]-src[iLoc-1])/
		(oFrameNum+1);
	   for(index=0, grayVal=src[iLoc-1]+stepVal; index<oFrameNum;
		   ++index, grayVal+=stepVal)
		dest[index] = static_cast<T>(round(grayVal));
	   break;
	case Quad:		   // val[k] = A*k^2 + B*k + C, k=0 for 1st pt
	case ConstraintQuad:
	   // correct x[t] to linear if x[t-+1]<[t]<x[t+-1] is not satisfied
	   if(iFrameNum<3) {
		perror("dynan::interpolate: Quad method requires >3 src images.\n");
		return;
	   }
	   if(iLoc>1){
	   	// using 2 pts before insert location and 1 pt after
		if(src[0]+src[2]==2*src[1]){
		   // degraded (linear) case
		   stepVal = static_cast<float>(src[1]-src[0])/(oFrameNum+1);
		   for(index=0, grayVal=src[0]+stepVal; index<oFrameNum;
			   ++index, grayVal+=stepVal)
			dest[index] = static_cast<T>(round(grayVal));
		}else {
		   const short x0=iLoc-2, x1=iLoc-1, x2=iLoc;
		   float C=static_cast<float>(src[x0]),
			   A=(src[x2]+(iFrameNum+1)*C-(iFrameNum+2)*src[x1])/
				(iFrameNum*iFrameNum+3*iFrameNum+2),
			   B=src[x1] - A - C;
		   for(index=2; index<2+iFrameNum; ++index)
			dest[index-2] = static_cast<T>(round(A*index
				   *index+B*index+C));
		}
	   }else{  // using 1 pt before insert location and 2 pts after (iLoc==1)
		if(src[2]+src[0]==2*src[1]) {
		   stepVal = static_cast<float>(src[2]-src[1])/(oFrameNum+1);
		   for(index=0, grayVal=src[1]+stepVal; index<oFrameNum;
			   ++index, grayVal+=stepVal)
			dest[index] = static_cast<T>(round(grayVal));
		}else {
		   float C=static_cast<float>(src[0]),
			   A=(static_cast<float>(iFrameNum+1)*src[2] - static_cast<float>
				   (iFrameNum+2)*src[1]+C)/static_cast<float>
				(iFrameNum*iFrameNum+3*iFrameNum+2),
			   B=static_cast<float>(src[2])-src[1]-(2*iFrameNum+3)*A;
		   for(index=1; index<=iFrameNum; ++index)
			dest[index-1] = static_cast<T>(round(A*index*
				   index+B*index+C));
		}
	   }
	   if(ipd==Quad)break;
	   const short startIndex = iLoc>1?1:0;
	   for(index=0; index<iFrameNum; ++index)
		if(dest[index]<std::min(src[startIndex], src[startIndex+1]) ||
			dest[index]>std::max(src[startIndex], src[startIndex+1]))
		   dest[index] = src[startIndex] + static_cast<T>(static_cast<float>
			   (index+1)*(src[startIndex+1]-src[startIndex])/(iFrameNum+1));
	   break;
   }
}

void dynan::regInterp(const IplImage* const velx, const IplImage* const vely)const {
   int srcIndex, destIndex;
   int iFrameX[iFrameNum], iFrameY[iFrameNum],
	 oFrameX[oFrameNum], oFrameY[oFrameNum];
   const int rat=apertureSz<<1;
   for(int y=srcIndex=0; y<height; ++y)
	for(int x=0; x<width; ++x, ++srcIndex)
	   for(int index=0; index<oFrameNum; ++index) {
		int yy=velx->imageData[srcIndex]*(index+1)/(oFrameNum+1)/45+y,
		    xx=vely->imageData[srcIndex]*(index+1)/(oFrameNum+1)/45+x;
		yy = yy<0? 0:yy>=height? height-1:yy;
		xx = xx<0? 0:xx>=width? width-1:xx;
		destIndex = yy*width+xx;
		pDestImg[index]->imageData[destIndex]=
		   vSrcImg[iLoc-1]->imageData[srcIndex];
	   }
   unsigned char* buf = new unsigned char[width*height];
   for(int pass=0; pass<4; ++pass)
	for(int index=0; index<oFrameNum; ++index){
	   // fill 'lost' pixels
	   medianFill(pDestImg[index],buf);
	   memcpy(pDestImg[index]->imageData, buf, imageSize*sizeof(unsigned char));
	}
   delete[] buf;
}

void dynan::medianFill(const IplImage*const src, unsigned char*
	buffer, const int winsz)const{
   // finds median of non-zero neighbors
   memcpy(buffer, src->imageData, imageSize*sizeof(unsigned char));
   const int bar_sz=(winsz*2+1)*(winsz*2+1);
   int srcIndex, destIndex, elemCnt;
   unsigned char barrel[bar_sz];
   for(int y=srcIndex=0; y<height; ++y)
	for(int x=0; x<width; ++x, ++srcIndex)
	   if(src->imageData[srcIndex]==0){	// pix needs filling
		memset(barrel, bar_sz, sizeof(char));  // reset array
		elemCnt=0;
		for(int yy=y-winsz; yy<=y+winsz; ++yy)
		   if(yy>=0 && yy<height)	// non-border element
			for(int xx=x-winsz, destIndex=yy*width+xx;
				xx<=x+winsz; ++xx, ++destIndex)
			   if(xx>=0 && xx<width && src->imageData[destIndex]!=0)
				barrel[elemCnt++] = src->imageData[destIndex];
		if(--elemCnt>0) {
		   qsort(barrel, elemCnt, sizeof(unsigned char),
			   dynan::typeCmp<unsigned char>);
		   buffer[srcIndex] = barrel[elemCnt/2];
		}
	   }
}

template<typename T>
int dynan::typeCmp(const void* src, const void* dest) {
   return (*(T*)src - *(T*)dest);
}

//++++++++++++++++++++++++++++++++++++++++
// VideoProp

VideoProp::Props::Props(const int& width, const int& height, const int& fps, const
	int& fcount, const int& posFrame, const float& posMsec, const float& posRatio,
	const int& codec, const int& depth, const int& chan):width(width),
   height(height),fps(fps),fcount(fcount),posFrame(posFrame),posMsec(posMsec),
   posRatio(posRatio),codec(codec),depth(depth),chan(chan){}

VideoProp::VideoProp(CvCapture* cap):cap(cap),ip(cvQueryFrame(cap)),
   prop(static_cast<int>(cvGetCaptureProperty(cap, CV_CAP_PROP_FRAME_WIDTH)),
	   static_cast<int>(cvGetCaptureProperty(cap, CV_CAP_PROP_FRAME_HEIGHT)),
	   static_cast<int>(cvGetCaptureProperty(cap, CV_CAP_PROP_FPS)),
	   static_cast<int>(cvGetCaptureProperty(cap, CV_CAP_PROP_FRAME_COUNT)),
	   static_cast<int>(cvGetCaptureProperty(cap, CV_CAP_PROP_POS_FRAMES)),
	   cvGetCaptureProperty(cap, CV_CAP_PROP_POS_MSEC),
	   cvGetCaptureProperty(cap, CV_CAP_PROP_POS_AVI_RATIO),
	   static_cast<int>(cvGetCaptureProperty(cap, CV_CAP_PROP_FOURCC)),
	   ip->depth, ip->nChannels){ // rewind 1 fame
   cvSetCaptureProperty(cap,CV_CAP_PROP_POS_FRAMES,--prop.posFrame);
   prop.posMsec=cvGetCaptureProperty(cap, CV_CAP_PROP_POS_MSEC);
   prop.posRatio=cvGetCaptureProperty(cap,
	   CV_CAP_PROP_POS_AVI_RATIO);
}

void VideoProp::update(const bool& lazy)throw(ErrMsg){
   if(lazy){
	int frmpos=static_cast<int>(cvGetCaptureProperty(cap, CV_CAP_PROP_POS_FRAMES));
	if(frmpos==prop.posFrame) return;
	prop.posFrame = frmpos;
	prop.posMsec = cvGetCaptureProperty(cap, CV_CAP_PROP_POS_MSEC);
	prop.posRatio = cvGetCaptureProperty(cap, CV_CAP_PROP_POS_AVI_RATIO);
	return;
   }
   else if(!cap) throw ErrMsg("VideoProp::update(): cap reset NULL");
   ip=cvQueryFrame(this->cap=cap);
   prop.depth=ip->depth; prop.chan=ip->nChannels;
   prop.posFrame=static_cast<int>(cvGetCaptureProperty(cap, CV_CAP_PROP_POS_FRAMES));
   cvSetCaptureProperty(cap, CV_CAP_PROP_POS_FRAMES, --prop.posFrame);
   prop.width=static_cast<int>(cvGetCaptureProperty(cap, CV_CAP_PROP_FRAME_WIDTH));
   prop.height=static_cast<int>(cvGetCaptureProperty(cap, CV_CAP_PROP_FRAME_HEIGHT));
   prop.fps=static_cast<int>(cvGetCaptureProperty(cap, CV_CAP_PROP_FPS));
   prop.fcount=static_cast<int>(cvGetCaptureProperty(cap, CV_CAP_PROP_FRAME_COUNT));
   prop.posMsec=cvGetCaptureProperty(cap, CV_CAP_PROP_POS_MSEC);
   prop.posRatio=cvGetCaptureProperty(cap, CV_CAP_PROP_POS_AVI_RATIO);
   prop.codec=static_cast<int>(cvGetCaptureProperty(cap, CV_CAP_PROP_FOURCC));
}

void swap(VideoProp& fst, VideoProp& snd){
   VideoProp tmp(fst); fst=snd; snd=tmp;
}

// ++++++++++++++++++++++++++++++++++++++++
// frameSizeEq

frameSizeEq::frameSizeEq(IplImage* pfrm[2]):iframe1(pfrm[0]),iframe2(pfrm[1]),
   oframe1(iframe1),oframe2(iframe2),ratio((0.+iframe1->width)/iframe2->width),
   img1Cvt(iframe1->nChannels==3&&(*iframe1->imageData!=*(iframe1->imageData+1))),
   img2Cvt(iframe2->nChannels==3&&(*iframe2->imageData!=*(iframe2->imageData+1))),
   img1Created(ratio<1||img1Cvt),img2Created(ratio>1||img2Cvt){
   if(ratio != (iframe1->height+0.)/iframe2->height) {
	sprintf(msg, "\nframeSizeEq::ctor aspect ratio unequal: [%d %d]/[%d %d]\n",
		iframe1->width, iframe1->height, iframe2->width, iframe2->height);
	throw ErrMsg(msg);
   }
   if(img1Created){	   /* enlarge iframe1 */
	oframe1 = cvCreateImage(cvGetSize(iframe2),iframe2->depth,iframe2->nChannels);
	cvResize(iframe1, oframe1, InterpMethod);
	if(img1Cvt)cvt2Gray(oframe1);	// `cvCvtColor' doesn't support in-place conversion
   }
   if(img2Created) {
	oframe2 = cvCreateImage(cvGetSize(iframe1),iframe1->depth,iframe1->nChannels);
	cvResize(iframe2, oframe2, InterpMethod);
	if(img2Cvt)cvt2Gray(oframe2);
   }
}

void frameSizeEq::cvt2Gray(IplImage* img){
   char* c=img->imageData;
   const int sz=img->width*img->height;
   if(cvtGrayFast) for(int indx=0; indx<sz; ++indx, c+=3)memset(c+1,*c,2);
   else for(int indx=0; indx<sz; ++indx, c+=3)memset(c, static_cast<unsigned char>
		(*c*.0722+*(c+1)*.7152+*(c+2)*.2126), 3);
}

frameSizeEq::~frameSizeEq() {
   if(img1Created)cvReleaseImage(&oframe1);
   if(img2Created)cvReleaseImage(&oframe2);
}

void frameSizeEq::update(const bool& first) {
   if(first) {
	if(img1Created){
	   cvResize(iframe1, oframe1, InterpMethod);
	   if(img1Cvt)cvt2Gray(oframe1);
	}else oframe1 = iframe1;
   }else{
	if(img2Created){
	   cvResize(iframe2, oframe2, InterpMethod);
	   if(img2Cvt)cvt2Gray(oframe2);
	}else oframe2 = iframe2;
   }
}

// ++++++++++++++++++++++++++++++++++++++++

myROI* frameRegister::roi;
float frameRegister::heuristicSearchBound=.3;

frameRegister::frameRegister(const char* cap_str[2], const int& prange, const
	DiffMethod& me, const VideoDFT::Transform& tr, const Criterion& parm,
	const int& nbin, const bool* drop, const bool normVec[2],const bool& inc,
	const int& norm)throw(ErrMsg):diffParam(parm),
   tr(tr),regpos(0),Method(me),count(0),prev_srcPos(0),nbins(nbin),dft1(0),dft2(0),
   hist(0),diffNorm(norm),dropArray(drop),norm1(normVec[0]),norm2(normVec[1]),
   inc(inc),src_cap(cvCreateFileCapture(cap_str[0])),nfr1(static_cast<unsigned>(
		cvGetCaptureProperty(src_cap, CV_CAP_PROP_FRAME_COUNT))),
   dest_cap(cvCreateFileCapture(cap_str[1])),nfr2(static_cast<unsigned>(
		cvGetCaptureProperty(dest_cap,CV_CAP_PROP_FRAME_COUNT))),
   range(prange>0? prange : (nfr1>nfr2? nfr1:nfr2)), diffPos(new int[2*range+1]),
   diffVal(new float[2*range+1]), frame1(cvQueryFrame(src_cap)),
   frame2(cvQueryFrame(dest_cap)) {
   if(!(src_cap && dest_cap && frame1 && frame2)) {
	if(!(src_cap && dest_cap))sprintf(msg,
		"frameRegister::ctor: cannot open \"%s\"for capturing.\n",
		cap_str[src_cap?1:0]);
	else sprintf(msg, "frameRegister::ctor: cannot get a frame in \"%s\".\n",
		cap_str[src_cap?1:0]);
	throw ErrMsg(msg);
   }
   if(frame1->width/frame2->width != frame1->height/frame2->height) {
	sprintf(msg, "frameRegister::ctor: Aspect ratio unequal: [%d %d]/[%d %d]",
		frame1->width, frame1->height, frame2->width, frame2->height);
	throw ErrMsg(msg);
   }
   cvSetCaptureProperty(src_cap, CV_CAP_PROP_POS_AVI_RATIO, 0);
   cvSetCaptureProperty(dest_cap, CV_CAP_PROP_POS_AVI_RATIO, 0);
   IplImage* frms[]={frame1, frame2};	// to calc HistDiff
   fse = new frameSizeEq(frms);
   if(Method==HistDiff)	hist = new Hist(fse, nbins, Correlation, false);
   else if(Method==DtDiff)
	dft1 = new VideoDFT(frame1, tr, nbins, true),
	     dft2 = new VideoDFT(frame2, tr, nbins, true);
   if(dropArray)	   // subtracts 'missed' frames
	for(int cnt=0; cnt<nfr2; ++cnt)if(dropArray[cnt])--nfr2;
}

frameRegister::~frameRegister() {
   delete fse, delete hist;
   delete dft1, delete dft2;
   delete[] diffPos, delete[] diffVal;
   cvReleaseCapture(&src_cap), cvReleaseCapture(&dest_cap);
}

void frameRegister::setROI(myROI* roi_set){roi = roi_set;}

const int frameRegister::reg(const int& pos, const int& prev_reg, const DiffMethod&
	me)throw(ErrMsg){
   if(pos >= nfr1)return -1;
   Method=me;
   int startpos = pos>range?pos-range:0, endpos = pos+range<nfr2?pos+range:nfr2-1;
   if(inc && startpos<=prev_reg)startpos=prev_reg+1;
   if(endpos+range<nfr2 && startpos>endpos){
	sprintf(msg,"frameRegister::reg: prev_reg starts from %d too large for searching"
		" %d-th frame. Search range=[%d %d].\n Try increase search range (%d) .",
		prev_reg, pos, startpos, endpos, range); throw ErrMsg(msg);
   }
   if(dropArray){
	for(int cnt=0; cnt<startpos; ++cnt)if(dropArray[cnt])--startpos, --endpos;
	for(int cnt=startpos; cnt<endpos; ++cnt)if(dropArray[cnt])--endpos;
   }
   count = endpos-startpos+1;
   memset(diffVal, 0, sizeof(float)*(range*2+1));
   memset(diffPos, 0, sizeof(int)*(range*2+1));
   int src_startpos =static_cast<int>(cvGetCaptureProperty(src_cap, CV_CAP_PROP_POS_FRAMES));
   cvSetCaptureProperty(src_cap, CV_CAP_PROP_POS_FRAMES, pos); frame1=cvQueryFrame(src_cap);
   cvSetCaptureProperty(dest_cap, CV_CAP_PROP_POS_FRAMES, startpos); frame2=cvQueryFrame(dest_cap);
   for(int cpos=startpos; cpos<=endpos; ++cpos) { /* frame-wise differences */
	// short-cut only when 'inc' constraint not imposed
	if(0==(diffVal[cpos-startpos] = calcDiff()) && !inc)return cpos+1;
	diffPos[cpos-startpos] = cpos;
	frame2=cvQueryFrame(dest_cap);
   }
   prev_srcPos = pos;
   float diffSort[count];
   memcpy(diffSort, diffVal, count*sizeof(float));
   std::nth_element(diffSort, diffSort+static_cast<int>(heuristicSearchBound*count),
	   diffSort+count);
   float threshold = diffSort[static_cast<int>(heuristicSearchBound*count)];
   if(!inc){	// both constraints use heuristic search bound
	bool nextLeft=true; int lpos=pos, rpos=pos;
	for(int cpos=pos; lpos>=startpos && rpos<=endpos; nextLeft=!nextLeft){
	   if(diffVal[cpos-startpos]<=threshold){
		regpos=cpos;break;
	   }
	   if(nextLeft && lpos>startpos)cpos=--lpos;
	   else if(rpos<endpos)cpos=++rpos;
	}
   }else{	// search for leftmost occurrence of p-percential position
	regpos=endpos;
	for(int regtmp=startpos; regtmp<=endpos; ++regtmp)
	   if(diffVal[regtmp-startpos]<=threshold && regtmp<regpos){
		regpos=regtmp; break;
	   }
   }
   if(::verbose)dump();
   return regpos;
}

void frameRegister::dump()const {
   puts("++++++++frameRegister++++++++");
   for(int cnt=0; cnt<count; ++cnt)
	printf("[%3d %3d]%c->%f\n", prev_srcPos, diffPos[cnt],
		regpos==diffPos[cnt]?'*':' ', diffVal[cnt]);
   puts("++++++++frameRegister++++++++");
}

const float frameRegister::calcDiff()throw(ErrMsg) {
   if(!(frame1 && frame2))
	throw ErrMsg("frameRegister::calcDiff: frame ptr Null.");
   fse->update();
   IplImage ipImg; cv::Mat ipMat;
   if(roi) {
	ipImg = *(roi->getPolyMat()), ipMat = cv::Mat(&ipImg);
	if(Method==HistDiff) hist->setMask(&ipMat);
   }
   IplImage *frm1 = const_cast<IplImage*>(fse->get(true)),
		*frm2 = const_cast<IplImage*>(fse->get(false));
   if(Method==HistDiff) {  // 0 gives perfect match
	if(!hist)	hist = new Hist(fse, nbins, diffParam, false);
	else		hist->update(frm1, frm2);
	arrayDiff<float, std::vector> adiff(hist->get(true), hist->get(false),
		diffParam, norm1);
	return static_cast<float>(adiff.diff());
   }else if(Method==DtDiff){
	if(!dft1) {
	   dft1 = new VideoDFT(frame1, tr, nbins, true);
	   delete dft2; dft2 = new VideoDFT(frame2, tr, nbins, true);
	} else dft2->update(), dft1->update();
	arrayDiff<double, std::vector> adiff(dft1->getEnergyDist(),
		dft2->getEnergyDist(), diffParam, norm2);
	return static_cast<float>(adiff.diff());
   }
   return calcImgDiff(frm1, frm2, roi?&ipImg:0, diffNorm);
}

// ++++++++++++++++++++++++++++++++++++++++
bool frameUpdater::rmAdjEq;

frameUpdater::frameUpdater(IplImage* pfrm[2], frameSizeEq& fse, frameBuffer* fb,
	simDropFrame* drop, VideoProp* pvp[2], ARMA_Array<float>* ma_vals[3], const
	unsigned& norm):normDiff(norm),frame1(pfrm[0]),frame2(pfrm[1]),
   mask(frameBuffer::getMask()),ma_val(ma_vals),fse(fse),fb(fb),capEOF(false),
   vp1(pvp[0]),vp2(pvp[1]),ndrop1(0),ndrop2(0),ndrop2Sim(0),
   fc1(vp1->prop.posFrame),fc2(vp2->prop.posFrame){
   if(!fb)throw ErrMsg("frameUpdater::ctor: frameBuffer NULL.\n");
   int size = std::min<int>(vp1->prop.fcount, vp2->prop.fcount);
   drop1.assign(size, true); drop2.assign(size, true);
   if(drop){
	const bool* array = drop->dropArray();
	for(int indx=0; indx<size; ++indx)simdrop.push_back(array[indx]);
   }
}

void frameUpdater::update(){
   do{
	do{
	   if(!(frame1=cvQueryFrame(vp1->cap)))break;
	   fb->update(true); fse.update(drop1[fc1++]=++ndrop1);
	}while(rmAdjEq && !calcImgDiff(fb->get(0,true),fb->get(1,true),
		   mask,-2));
	do{	// NOTE: had to add fc2>2 constraint Y?
	   if(!(frame2=cvQueryFrame(vp2->cap)))break;
	   fb->update(false); fse.update(!(drop2[fc2++]=++ndrop2));
	}while(rmAdjEq && !calcImgDiff(fb->get(0,false),
		   fb->get(1,false),mask,-2));
	--ndrop1, --ndrop2;
	// simDropFrame affects only the secondary video
	while(!simdrop.empty() && simdrop[fc2]){
	   if(!(frame2=cvQueryFrame(vp2->cap)))break;
	   fb->update(true); fse.update(!(drop2[fc2++]=++ndrop2Sim));
	}
   }while(frame1 && frame2 && rmAdjEq &&
	   !calcImgDiff(fse.get(true),fb->get(1,true),mask,-2));
   capEOF=!(frame1&&frame2);
   ma_val[0]->append(calcImgDiff(fse.get(true),fb->get(1,true),
		frameBuffer::getMask(), normDiff));
   ma_val[1]->append(calcImgDiff(fse.get(false),fb->get(1,false),
		frameBuffer::getMask(), normDiff));
   ma_val[2]->append(calcImgDiff(fse.get(true), fse.get(false),
		frameBuffer::getMask(), normDiff));
}

void frameUpdater::dump()const {
   puts("\n========frameUpdater::dump========\nVideo #1:");
   printf("==%d/%d==\n", fc1, fc2);
   for(int indx=0; indx<fc1; ++indx)
	printf("%c ", drop1[indx]?'T':'F');
   puts("\nVideo #2:");
   for(int indx=0; indx<fc2; ++indx)
	printf("%c ", drop2[indx]?'T':'F');
   printf("ndrop1=%d, ndrop2=%d, ndrop2Sim=%d\n", ndrop1, ndrop2, ndrop2Sim);
   puts("\n====================");
}

//++++++++++++++++++++++++++++++++++++++++
/* */
Logger::Logger(const char* log, VideoProp*vp[2], ARMA_Array<float>* ma[3], Hist&
	hist, VideoDFT* vd[2], const bool nv[2])throw(ErrMsg):file(log),vp1(vp[0]),
    vp2(vp[1]),hist(hist),dft1(vd[0]),dft2(vd[1]),dyn1(ma[0]),dyn2(ma[1]),diff(ma[2]),
    histDiff(hist.get(true), hist.get(false),Default, normVec[0]=nv[0]),
   bufRec(0){
	if(!vp1 || !vp2)  throw ErrMsg("Logger::ctor: VideoProp set NULL.");
	if(!dyn1 || !dyn2 || !diff)throw ErrMsg("Logger::ctor: Dynamic/Diff set NULL.");
	if(!dft1 || !dft2)throw ErrMsg("Logger::ctor: VideoDFT set NULL.");
	dftDiff = std::unique_ptr<arrayDiff<double> >(new
		arrayDiff<double>(dft1->getEnergyDist(), dft2->getEnergyDist(),
		Default, normVec[1]=nv[1]));
   fputs("Index1\tIndex2\tDyn1\tDyn2\tDiff\tHist_Corr\tHist_Chisq\t"
	   "Hist_Inter\tHist_Bhatta\tDT_Corr\tDT_Inter\tDT_Bhatta\n",file.fd);
   vdyn1.reserve(vp[0]->prop.fcount); vdyn2.reserve(vp[0]->prop.fcount);
   vdiff.reserve(vp[0]->prop.fcount);
   vhist_diff1.reserve(vp[0]->prop.fcount); vhist_diff2.reserve(vp[0]->prop.fcount); vhist_diff3.reserve(vp[0]->prop.fcount); vhist_diff4.reserve(vp[0]->prop.fcount);
   vdft_diff1.reserve(vp[0]->prop.fcount); vdft_diff3.reserve(vp[0]->prop.fcount); vdft_diff4.reserve(vp[0]->prop.fcount);
   update();
}

void Logger::update(){
   fprintf(file.fd,"%d\t%d\t", vp1->prop.posFrame, vp2->prop.posFrame);   // frame pos
   double ff[]={dyn1->get_val(), dyn2->get_val(), diff->get_val(), histDiff.diff(Correlation),
	histDiff.diff(Chi_square), histDiff.diff(Intersection), histDiff.diff(Bhattacharyya),
   dftDiff->diff(Correlation), dftDiff->diff(Intersection), dftDiff->diff(Bhattacharyya)};
   // NUMERICAL problem with f7: non-zero when den==num
   for(int indx=0; indx<3; ++indx)fprintf(file.fd,"%.2f\t",ff[indx]);
   for(int indx=3; indx<10;++indx)fprintf(file.fd,"%.4g\t",ff[indx]);
   fputc('\n',file.fd);
   if(++bufRec>=bufRecCap && !(bufRec=0)) fflush(file.fd);
   // Always set both vectors with same mean when copying to object histDiff/dftDiff
   histDiff.update(hist.get(true), hist.get(false), normVec[0]);
//    Normalized histogram/DFT difference
   dftDiff->update(dft1->getEnergyDist(), dft2->getEnergyDist(), normVec[1]);
   vdyn1.push_back(ff[0]); vdyn2.push_back(ff[1]); vdiff.push_back(ff[2]);
   vhist_diff1.push_back(ff[3]); vhist_diff2.push_back(ff[4]);
   vhist_diff3.push_back(ff[5]); vhist_diff4.push_back(ff[6]);
   vdft_diff1.push_back(ff[7]); vdft_diff3.push_back(ff[8]); vdft_diff4.push_back(ff[9]);
}

const std::vector<float>& Logger::get(const int& id)const{
   switch(id){
	case 0: return vdyn1;
	case 1: return vdyn2;
	case 2: return vdiff;
	case 3: return vhist_diff1;
	case 4: return vhist_diff2;
	case 5: return vhist_diff3;
	case 6: return vhist_diff4;
	case 7: return vdft_diff1;
	case 8: return vdft_diff3;	// skipping DT Chi-square
	default: return vdft_diff4;
   }
}

//++++++++++++++++++++++++++++++++++++++++

std::string Updater::fn;
std::fstream* Updater::pfs;
double Updater::tickFreq = cv::getTickFrequency();
bool Updater::firstCall=true;

Updater::Updater(const short& cnt, VideoProp** _vp, VideoDFT** _vd, myROI* _roi,
	Hist* _hist, frameUpdater* _fu, Logger& _log, const bool& _showBar,
	const bool& _showDft):logs(_log),frameNum(2),roi(_roi),
   showBar(_showBar),showDft(_showDft),hist(_hist),fu(_fu),
   curTick(cv::getTickCount()),prevTick(curTick),vp_null(!(_vp && _vp[0])),dftBar(0),
   vd_null(!(_vd && _vd[0])) {
   if(!vp_null) for(int indx=0; indx<frameNum; ++indx){
	vp_vec.push_back(_vp[indx]);
   }
   if(!vd_null){
	for(int indx=0; indx<frameNum; ++indx)vd_vec.push_back(_vd[indx]);
	if(showBar) dftBar = new drawBars<std::vector<double> >(cvSize(380, 120));
   }
   std::fstream fs("na", std::ios_base::in);// yes, I know it fails and 
   pfs = &fs;	// goes out of scope; just to point to sth
   if(pfs->is_open())pfs->close();
}

Updater::~Updater() {
   // use fn as fclose condition checker
   if(!fn.empty())pfs->close();
   if(dftBar)delete dftBar;
}

void Updater::setFileName(const char* str) {
   fn=*str;
   if(pfs->is_open())pfs->close();
   pfs->open(fn.c_str(), std::ios_base::in|std::ios_base::out);
   // sets stream exception triggering
   pfs->exceptions(std::ios_base::eofbit|std::ios_base::failbit
	   |std::ios_base::badbit);   
}

void Updater::setFileName(const std::string& str) {
   fn=str;
   if(pfs->is_open())pfs->close();
   pfs->open(fn.c_str(), std::ios_base::in|std::ios_base::out);
   pfs->exceptions(std::ios_base::eofbit|std::ios_base::failbit
	   | std::ios_base::badbit);
}

void Updater::read(){	// call setFileName before read/write
   if(!pfs || !*pfs) {
	fprintf(stderr, "Update::read: pfs=NULL or not opened\n"); return;
   }
   pfs->seekp(std::ios_base::beg);	// header checker
   time_t cur_time; char delim;
   (*pfs)>>cur_time>>delim;
   if(delim!=0x0) {
	sprintf(msg, "Updater::read::header: tm=%ju <%s>, delim=%d!=0.\n",
		static_cast<uintmax_t>(cur_time), asctime(localtime(&cur_time)),
		static_cast<int>(delim));
	throw ErrMsg(msg);
   }else printf("File created on %s\n\n", asctime(localtime(&cur_time)));
   if(!vd_null)BOOST_FOREACH(VideoDFT* i,vd_vec)i->read(pfs);
   if(roi)	roi->read(pfs);
   pfs->flush();
}

void Updater::write() {
   if(!pfs || !*pfs) {
	fprintf(stderr, "Update::read: pfs=NULL or not opened\n"); return;
   }
   pfs->seekp(std::ios_base::beg);
   time_t curtime = time(NULL);
   (*pfs)<<curtime<<static_cast<char>(0x0);
   if(!vd_null)BOOST_FOREACH(VideoDFT* i,vd_vec)i->write(pfs);
   if(roi) roi->write(pfs);
   pfs->flush();
}

void Updater::update(const bool& locked, const bool& log) {
   if(roi)roi->update();
   if(!locked) {
	if(!vp_null)BOOST_FOREACH(VideoProp* i, vp_vec)i->update();
	if(fu)fu->update();
	if(log && !firstCall)logs.update();
	if(hist)hist->update();
	if(!vd_null){
	   BOOST_FOREACH(VideoDFT* i, vd_vec)i->update();
	   if(showBar)cv::imshow("DftBars", dftBar->draw(vd_vec[0]->getEnergyDist(),
			vd_vec[1]->getEnergyDist()));
	   if(showDft){
		cv::imshow("Dft1", vd_vec[0]->getDftMag());
		cv::imshow("Dft2", vd_vec[1]->getDftMag());
	   }
	}
   }
   prevTick = curTick; curTick = cv::getTickCount(); firstCall=false;
   if(verbose)dump();
}

void Updater::dump()const {
   if(!vd_null)BOOST_FOREACH(VideoDFT* i, vd_vec)i->dump();
   if(roi)roi->dump();	if(hist)hist->dump();
}

const long Updater::tm()const{	// in unit of ms
   return static_cast<long>(1000*(curTick-prevTick)/tickFreq);
}

//++++++++++++++++++++++++++++++++++++++++

char VideoRegister::Codec[]="DIV3";
const float VideoRegister::pi_deg = M_PI/180;

VideoRegister::VideoRegister(VideoProp& vp, char*const fname, const char& bright)
   throw(ErrMsg):vp(vp),fname(fname),bright(bright),suf(0),appName(0),tolMean(10),
   tolSd(5),minFrameVideo(20),badStop(false) {
   if(!fname){
	sprintf(msg,"VideoRegister::ctor: file name NULL.\n"); throw ErrMsg(msg);
   }
}

VideoRegister::~VideoRegister(){delete[] appName;}

void VideoRegister::prepend(char*const suf, const int& np, const int& noiseType,
	const float* noiseLevel, const simDropFrame* df, const int& dropMethod)
   throw(ErrMsg,cv::Exception){
   if(!suf){
	sprintf(msg,"VideoRegister::prepend(): file name NULL.\n"); throw ErrMsg(msg);
   }
   this->suf=suf; strAppend();  // prepares appName
   cvSetCaptureProperty(vp.cap,CV_CAP_PROP_POS_FRAMES,0);
   int icodec = cvGetCaptureProperty(vp.cap, CV_CAP_PROP_FOURCC); CvVideoWriter* write;
   try{
	write = cvCreateVideoWriter(appName, icodec, vp.prop.fps, cvSize(vp.prop.width,
		   vp.prop.height), vp.prop.chan);
   }catch(const cv::Exception& ex){
	char *p=reinterpret_cast<char*>(&icodec), codec[]={*p, *(p+1), *(p+2), *(p+3), 0};
	printf("Warning: VideoRegister::save: original codec format \"%s\" not supported."
		" Fall back to \"%s\" codec option.\n", codec, Codec);
	write = cvCreateVideoWriter(appName, CV_FOURCC(Codec[0],Codec[1],Codec[2],Codec[3]),
		vp.prop.fps, cvSize(vp.prop.width, vp.prop.height), vp.prop.chan);
   }
   IplImage* frame=np>0&&noiseType>0? cvCreateImage(cvSize(vp.prop.width,
		vp.prop.height), vp.prop.depth, vp.prop.chan):0,
	*framePrev=df>0 ? cvCreateImage(cvSize(vp.prop.width,
		vp.prop.height), vp.prop.depth, vp.prop.chan):0; 
   bool frameCap=false;
   simDropFrame* drop2 = df && dropMethod==3 ? new simDropFrame(*df):0;
   const bool *dropSeq=df&&dropMethod?df->dropArray():0,
	   *dropSeq2=drop2?drop2->dropArray():0;
   if(np>0){
	frame = framePrev;   // sharing
	memset(frame->imageData, bright, frame->imageSize);
	for(int indx=0; indx<np; ++indx)cvWriteFrame(write, frame);
   }else if(np<0)	// skip first -np frames
	for(int cnt=0; cnt>np; --cnt)
	   if(!(frame=cvQueryFrame(vp.cap)))break;
   for(int cnt= np>=0?0:-np; cnt<vp.prop.fcount; ++cnt)
	if((frame=cvQueryFrame(vp.cap)) && !(dropSeq&&*(dropSeq+cnt))){
	   switch(noiseType){
		case 1:	saltPepper(frame,*noiseLevel);   break;
		case 2:	normalNoise(frame,*noiseLevel);  break;
		case 3:	solidMoveAdd(frame, noiseLevel);
		default:;
	   }
	   cvWriteFrame(write, frame); frameCap=true;
	   if(dropMethod>1)memcpy(framePrev->imageData, frame->imageData,
		   frame->imageSize);
	}else if(df){
	   if(dropMethod==2 || dropSeq2 && *(dropSeq2+cnt)){	// freezing
		if(!frameCap){
		   frameCap=(frame=cvQueryFrame(vp.cap));
		   switch(noiseType){
			case 1:	saltPepper(frame,*noiseLevel);   break;
			case 2:	normalNoise(frame,*noiseLevel);  break;
			case 3:	solidMoveAdd(frame, noiseLevel);
			default:;
		   }
		   if(dropMethod>1)memcpy(framePrev->imageData, frame->imageData,
			   frame->imageSize);
		}
		if(frame)cvWriteFrame(write, framePrev);
		else break;
	   }
	}else{
	   fprintf(stderr,"VideoRegister::prepend(): cannot query %d-th frame.", cnt);
	   if(badStop){
		fprintf(stderr," %d-%d frames discarded.\n", cnt, vp.prop.fcount);
		break;
	   }else fputc('\n',stderr);
	}
   cvReleaseVideoWriter(&write);
   cvSetCaptureProperty(vp.cap,CV_CAP_PROP_POS_FRAMES, vp.prop.posFrame);
   if(drop2)delete drop2;
   if(np>0&&noiseType>0)cvReleaseImage(&frame);
   if(framePrev)cvReleaseImage(&framePrev);
   if(::verbose && df)df->dump();
}

void VideoRegister::save(char*const suf, const cv::Size& sz)throw(ErrMsg,cv::Exception) {
   if(!suf)throw ErrMsg("VideoRegister::save(): file name NULL.\n");
   this->suf=suf; strAppend();
   reg(sz);
   cvSetCaptureProperty(vp.cap,CV_CAP_PROP_POS_FRAMES,startFrame);
   const int width2=end_x-start_x+1, height2=end_y-start_y+1;
   int icodec = cvGetCaptureProperty(vp.cap, CV_CAP_PROP_FOURCC); CvVideoWriter* write;
   try{
	write = cvCreateVideoWriter(appName, icodec, vp.prop.fps, cvSize(width2,height2),
		vp.prop.chan);
   }catch(const cv::Exception& ex){
	char *p=reinterpret_cast<char*>(&icodec), codec[]={*p, *(p+1), *(p+2), *(p+3), 0};
	printf("Warning: VideoRegister::save: original codec format \"%s\" not supported."
		" Fall back to \"%s\" codec option.\n", codec, Codec);
	write = cvCreateVideoWriter(appName, CV_FOURCC(Codec[0],Codec[1],Codec[2],Codec[3]),
		vp.prop.fps, cvSize(width2,height2), vp.prop.chan);
   }
   IplImage *cropped=cvCreateImage(cvSize(width2, height2), vp.prop.depth,
	   vp.prop.chan), *ip;
   cvQueryFrame(vp.cap);	// skip added frame
   for(int cnt=startFrame; cnt<endFrame; ++cnt){
	if(ip=cvQueryFrame(vp.cap)){
	   crop(cropped, ip); cvWriteFrame(write,cropped);
	} else{
	   fprintf(stderr,"VideoRegister::save(): cannot query %d-th frame.",cnt);
	   if(badStop){
		fprintf(stderr, " %d-%d frames discarded.\n", cnt, vp.prop.fcount);
		break;
	   }else fputc('\n',stderr);
	}
   }
   cvReleaseVideoWriter(&write); cvReleaseImage(&cropped);
   cvSetCaptureProperty(vp.cap,CV_CAP_PROP_POS_FRAMES, vp.prop.posFrame);
}

int VideoRegister::reg(const cv::Size& dimHint)throw(ErrMsg){
   const int &width=vp.prop.width, &height=vp.prop.height;
   cvSetCaptureProperty(vp.cap,CV_CAP_PROP_POS_FRAMES,0); 
   int nMatched=0, matches[]={0,0}, indx=0;
   double mean, sd; bool verbose=false;
   // frame indices matching
   for(int indx=0; indx<vp.prop.fcount && nMatched<2;
	   ++indx, cvMean_StdDev(cvQueryFrame(vp.cap), &mean, &sd, 0)){
	if(verbose && sd<20)printf("%d %F %F\n", indx, mean, sd);
	if(sd<tolSd && mean>bright-tolMean && mean<bright+tolMean)
	   if(!nMatched && (nMatched=1) || indx-matches[0]<minFrameVideo)
		matches[0]=indx-1;
	   else if(nMatched && indx-matches[0]>=minFrameVideo){
		matches[1]=indx-1; break;
	   }
   }
   if(!nMatched){
	sprintf(msg, "VideoRegister::reg(): No matched frame found from %d frames.\n"
		"Try loosening mean/sd tolerance?\n",
		vp.prop.fcount); throw(ErrMsg(msg));
   }
   startFrame=matches[0]; endFrame=matches[1]?matches[1]:vp.prop.fcount-1;
   // pixel position matching
   cvSetCaptureProperty(vp.cap,CV_CAP_PROP_POS_FRAMES,startFrame);
   IplImage* frm = cvQueryFrame(vp.cap);
   nMatched = nMatched==1?9:0;
   const int accFactor=3;	// for traversing pixels in image
   long double ld_mean;
   int val, x,y;
   float tolMean2 = tolMean*.8;	   // tighten
   if(dimHint.width && width<=dimHint.width){
	start_x=0; end_x=width-1; ++nMatched;
   }else{
	unsigned char*const px0=reinterpret_cast<unsigned char*>(frm->imageData), *px=px0;
	for(x=0; x<width; ++x, px+=vp.prop.chan){ // 1st element of BGR
	   for(y=0,ld_mean=*px; y<height; ld_mean+=*(px+(y+=accFactor)*frm->widthStep+1));
	   val=static_cast<int>(ld_mean/(height/accFactor));
	   if(val>bright-tolMean2 && val<bright+tolMean2)break;
	}
	start_x = x; start_x=0;
	for(px=px0+(x=width-1)*vp.prop.chan; x>start_x; --x,px-=vp.prop.chan){
	   for(y=0,ld_mean=*px; y<height; ld_mean+=*(px+(y+=accFactor)*frm->widthStep));
	   val=static_cast<int>(ld_mean/height*accFactor);
	   if(val>bright-tolMean2 && val<bright+tolMean2)break;
	}
	end_x = x;
	if(dimHint.width && end_x-start_x+1!=dimHint.width){
	   if(end_x-start_x+1!=dimHint.width && (nMatched+=2)){
		fprintf(stderr,"Warning: VideoRegister::reg(): registered width "
			"(%d-%d=%d) != hint width %d\n", end_x,start_x, end_x-start_x+1,
			dimHint.width);
		// centrify width dimensions
		int diff = (dimHint.width-(end_x-start_x+1))/2;
		start_x=start_x-diff; end_x=start_x+dimHint.width-1;
		fprintf(stderr, "\t Width range set to [%d %d]\n", start_x, end_x);
	   }
	}
   }
   if(dimHint.height && height<=dimHint.height){
	start_y=0; end_y=height-1; nMatched+=3;
   }else{
	for(y=0; y<height; ++y){
	   for(x=0,ld_mean=static_cast<unsigned char>(frm->imageData[vp.prop.chan*x]);
		   x<width; ld_mean+=static_cast<unsigned char>
		   (frm->imageData[frm->widthStep*y+vp.prop.chan*x]), x+=accFactor);
	   val=ld_mean/width*accFactor;
	   if(val>bright-tolMean2 && val<bright+tolMean2)break;
	}
	start_y = y;
	for(y=height-1; y>start_y; --y){
	   for(x=0,ld_mean=static_cast<unsigned char>(frm->imageData[vp.prop.chan*x]);
		   x<width; ld_mean+=static_cast<unsigned char>
		   (frm->imageData[frm->widthStep*y+vp.prop.chan*x]), x+=accFactor);
	   val=ld_mean/width*accFactor;
	   if(val>bright-tolMean2 && val<bright+tolMean2)break;
	}
	end_y = y;
	if(dimHint.height && end_y-start_y+1!=dimHint.height){
	   if(end_y-start_y+1 != dimHint.height && (nMatched+=6)){
		fprintf(stderr,"Warning: VideoRegister::reg(): registered height "
			"(%d-%d=%d) != hint height %d\n", end_y, start_y, end_y-start_y+1,
			dimHint.height);
		int diff = (dimHint.height-(end_y-start_y+1))/2;
		start_y=start_y-diff; end_y=start_y+dimHint.height-1;
		fprintf(stderr, "\t Height range set to [%d %d]\n", start_y, end_y);
	   }
	}
   }
   cvSetCaptureProperty(vp.cap,CV_CAP_PROP_FRAME_COUNT, vp.prop.posFrame);
   return nMatched;
}

void VideoRegister::strAppend(){
   char *p=fname;
   delete[] appName; appName=new char[strlen(fname)+strlen(suf)+1];
   char *dest=appName; size_t sz=0;
   while(*p && *(p++)!='.')++sz;
   strncpy(dest,fname,sz); strcpy(dest+sz, suf);
   strncpy(dest+sz+strlen(suf), --p, strlen(fname)-sz+1);
}

void VideoRegister::crop(IplImage* dest, IplImage*const src){
   assert(end_x-start_x+1==dest->width && end_y-start_y+1==dest->height &&
	   src->nChannels==dest->nChannels);
   const int copy_width=end_x-start_x+1;
   char *psrc, *pdest; int y;
   for(y=start_y, psrc=src->imageData+y*src->widthStep+start_x*src->nChannels,
	   pdest=dest->imageData; y<=end_y;
	   ++y, psrc+=src->widthStep, pdest+=dest->widthStep)
	memcpy(pdest, psrc, copy_width*src->nChannels);
}

void VideoRegister::saltPepper(IplImage* src, const float& SNR) {
   const float prob = pow10f(-SNR/10);
   int chan=src->nChannels; float p;
   for(int y=0; y<src->height; ++y)
	for(int x=0; x<src->width; ++x)
	   if((p=static_cast<float>(rand())/RAND_MAX)<prob/2)
		memset(src->imageData+y*src->widthStep+x*src->nChannels, 0, src->nChannels);
	   else if(p<prob)
		memset(src->imageData+y*src->widthStep+x*src->nChannels,0xff, src->nChannels);

}

void VideoRegister::normalNoise(IplImage* src, const float& sigma) {
   cv::Mat Mnoise(src,true), Msrc(src);
   cv::randn(Mnoise, 0, sigma);
   if(src->nChannels>1)	   // noise in gray-scale only
	for(int y=0; y<src->height; ++y)
	   for(int x=0; x<src->width; ++x)
		memset(Mnoise.data+Mnoise.step*y+x*Mnoise.elemSize()+1,
			*(Mnoise.data+Mnoise.step*y+x*Mnoise.elemSize()), Mnoise.elemSize()-1);
   cv::add(Msrc, Mnoise, Msrc);
}

void VideoRegister::solidMoveAdd(IplImage* src, const float*param){
   const int width=src->width, height=src->height;
   int offx = static_cast<int>(*param*cos(*(param+1)*pi_deg))%width,
	   offy = static_cast<int>(*param*sin(*(param+1)*pi_deg))%height;
   offx = offx<0? src->width+offx : offx;
   offy = offy<0? src->height+offy : offy;
   IplImage* added=cvCreateImage(cvSize(width,height), src->depth, src->nChannels);
   moveRectRegion(src,added, cv::Size(width-offx, height-offy),
	   cv::Point(0, 0), cv::Point(offx, offy));
   moveRectRegion(src,added, cv::Size(offx, height-offy),
	   cv::Point(width-offx-1, 0), cv::Point(0, offy));
   moveRectRegion(src,added, cv::Size(width-offx, offy),
	   cv::Point(0, height-offy-1), cv::Point(offx, 0));
   moveRectRegion(src,added, cv::Size(offx, offy),
	   cv::Point(width-offx-1, height-offy-1), cv::Point(0, 0));
   cvAddWeighted(src,1-*(param+2), added,*(param+2), 0.,src);
   cvReleaseImage(&added);
}

void VideoRegister::moveRectRegion(const IplImage* src, IplImage* dest, const
	cv::Size& rectSz, const cv::Point& srcPt, const cv::Point& destPt)throw(ErrMsg) {
/* "Internal" function for fast implementation of large
 * blockwise translation used by solidMove */
   if(!rectSz.width && !rectSz.height || srcPt == destPt) {
	memcpy(dest->imageData, src->imageData, src->imageSize); return;
   }
   const int wid=src->widthStep, h=rectSz.height, w=rectSz.width*src->nChannels;
   char *pdest=dest->imageData+destPt.y*wid+destPt.x*dest->nChannels,
		*psrc=src->imageData+srcPt.y*wid+srcPt.x*src->nChannels;
   for(int y=0; y<rectSz.height; ++y, pdest+=wid, psrc+=wid) // row-wise copy
	memcpy(pdest, psrc, w);
}
