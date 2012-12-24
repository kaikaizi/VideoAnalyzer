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
#include "cv_templates.hh"
#include "sketch.hh"
#include "hist.hh"
#include "conf.hh"
#include <iterator>
#include <boost/foreach.hpp>
#define For BOOST_FOREACH

void dashLine(cv::Mat& mat, const cv::Point& from, const cv::Point& to) {
   cv::LineIterator iter(mat, from, to);
   const int dashLen=10, dashGap=3, dashTotal=dashLen+dashGap;
   const int chan = mat.channels(), count = iter.count;
   if(chan==1) {
	for(int index=0; index<count; ++index, ++iter)
	   if(index%dashTotal < dashLen)
		**iter = 255-**iter;		// opposite color
   }else if(chan==3) {
	for(int index=0; index<count; ++index, ++iter)
	   if(index%dashTotal < dashLen) {
		cv::Point pt = iter.pos();
		cv::Vec3b rgb = mat.at<cv::Vec3b>(pt);
		mat.at<cv::Vec3b>(pt)[0] = cv::saturate_cast<uchar>(255-rgb[0]);
		mat.at<cv::Vec3b>(pt)[1] = cv::saturate_cast<uchar>(255-rgb[1]);
		mat.at<cv::Vec3b>(pt)[2] = cv::saturate_cast<uchar>(255-rgb[2]);
	   }
   }else {
	sprintf(msg, "dashLine(): unable to handle %d-channel matrix.\n", chan);
	throw ErrMsg(msg);
   }
}

void roiMouseCallBack(const int* params, myROI& roi) {
// params[0]==0: quit selection mode
// 1: append a point (x = params[1], y = params [2] is location);
// 2: remove a point; 3: pass ROI to Hist and clear;
   if(*params==1) {
	roi.pushPoint(cv::Point(*(params+1), *(params+2)));
	fprintf(stderr,"[%d, %d] added.\n", *(params+1), *(params+2));
	fflush(stderr);
   }else if(*params==2) {
	const cv::Point &last=roi.popPoint();
	if(last.x>=0)fprintf(stderr, "[%d %d] removed.\n",
		last.x, last.y);
   }else if(*params==3) {
	if(roi.pt_poly.size()<2){  // also check for other conds?
	   fputs("Warning: roiMouseCB::invalid polygon\n", stderr);
	   fflush(stderr); return;
	}
	if(!myROI::chkConvexPolygon(roi.pt_poly))
	   fputs("\nWarning: roiMouseCB:: concave polygon\n", stderr);
	Hist::setMask(roi.getPolyMat());
	IplImage mask = *(roi.getPolyMat());
	frameBuffer::setMask(&mask);
	fputs("File name to save image mask: ", stderr);
	char fname[64]; int len;
	if(!fgets(fname, 32, stdin) || 1==(len=strlen(fname))){
	   fputs("\nMask NOT saved.",stderr); fflush(stderr);
	   roi.clear(); return;	// making mask-selection an no-op
	}
	fname[strlen(fname)-1]=0;
	if(strcmp(fname+strlen(fname)-4,".msk")) strcat(fname,".msk");
	FILE* fp;
	if((fp=fopen(fname,"r"))){
	   fprintf(stderr,"Warning: %s already exists.\n", fname);
	   fflush(stderr); fclose(fp);
	}
	if(!(fp=fopen(fname, "w"))){
	   sprintf(msg, "roiMouseCallBack: cannot write to file %s\n", fname);
	   throw ErrMsg(msg);
	}
	roi.ready=true; roi.write(fp); roi.clear();
   }else roi.getROI();
}

inline ErrMsg::ErrMsg(const char* _msg, const int& id):Errno(id){
   memset(msg,0,256); strcpy(msg, _msg);
   sprintf(msg+strlen(_msg), ":0x%x",id);
}

inline ErrMsg::ErrMsg(const std::string& _msg, const int& id):Errno(id){
   memset(msg,0,256); strcpy(msg, _msg.c_str());
   sprintf(msg+_msg.size(), ":0x%x",id);
}

// ++++++++++++++++++++++++++++++++++++++++
// readWrite
const char readWrite::delim[2]="\1";
readWrite::readWrite(cv::Mat* mat, const bool& alloc)throw(ErrMsg):
   self_alloc(alloc),region(mat&&self_alloc?(new cv::Mat(*mat)):mat) {
   if(!region)throw ErrMsg("readWrite::ctor: Mat data cannot be NULL.\n");
}

readWrite::readWrite(IplImage* ipl, const bool& alloc)throw(ErrMsg)
   :self_alloc(alloc){
   if(self_alloc) region = new cv::Mat(ipl);
   else region = new cv::Mat(ipl);	// TODO: ???
   if(!region) throw ErrMsg("readWrite::ctor: Mat data cannot be NULL.\n");
}

// ++++++++++++++++++++++++++++++++++++++++
// myROI
// NOTE: 1st arg(pmat) points to an empty canvass; src/src2
// points to actual frames
myROI::myROI(cv::Mat* pmat, cv::Mat* src[2], const bool& enable_redraw):
   readWrite::readWrite(pmat,true),redrawable(enable_redraw),pPoly_updated(true),pt_array(0),psrc(src[0]),
	psrc2(src[1]),imgSz(cvGetSize(src[1])),flash(off),self_alloc(false),ready(false),
	gray(src[0]->channels()==1),wn1(0),wn2(0) {}

myROI::myROI(IplImage* pmat, IplImage* src[2], const char* const wns[2], const bool&
	enable_redraw):readWrite::readWrite(pmat,true),redrawable(enable_redraw),
	pPoly_updated(true),pt_array(0),flash(off),self_alloc(true),ready(false),
	gray(src[0]->nChannels==1),imgSz(cv::Size(src[0]->width,src[0]->height)) {
   psrc = new cv::Mat(src[0]); psrc2 =new cv::Mat(src[1]);
   wn1 = new char[strlen(wns[0])+1]; wn2 = new char[strlen(wns[1])+1];
   strcpy(wn1, wns[0]); strcpy(wn2, wns[1]);
}

myROI::~myROI() {
   if(self_alloc) {
	delete psrc; delete psrc2;
	delete[] wn1; delete[] wn2;
   }
   if(pt_array) delete[] pt_array;
}

void myROI::update() {
   if(flash!=off) {
	if(flash==state2)cvWaitKey(80);// flash
	cv::swap(*psrc, *region);
	if(self_alloc) {
	   cv::imshow(wn1, *psrc); cv::imshow(wn2, *psrc2);
	}
	flash = flash==state1? state2 : off;
   }
}

void myROI::clear() {
   if(!pt_poly.empty()) {
	pt_poly.clear();
	if(redrawable) {
	   lines_gray1.clear(); lines_gray2.clear();
	   lines_color1.clear(); lines_color2.clear();
	}
	if(pt_array) delete[] pt_array;
	*region = cv::Scalar(0);
   }
   ready=false;
}

void myROI::pushLine(const bool tail) {
   int beg = pt_poly.size()-(tail?1:2), end = tail?0:pt_poly.size()-1;
   cv::LineIterator iter1(*psrc, pt_poly.at(beg), pt_poly.at(end)),
	iter2(*psrc2, pt_poly.at(beg), pt_poly.at(end));
   if(gray) {  // for undraw restoration of pixel values
	std::vector<unsigned char> lin1(iter1.count), lin2(iter2.count);
	for(int index=0; index<iter1.count; ++index, ++iter1)lin1[index] = **iter1;
	lines_gray1.push_back(lin1);
	for(int index=0; index<iter2.count; ++index, ++iter2)lin2[index] = **iter2;
	lines_gray2.push_back(lin2);
   }else{
	std::vector<cv::Vec3b> lin1(iter1.count), lin2(iter2.count);
	for(int index=0; index<iter1.count; ++index, ++iter1)
	   lin1[index] = psrc->at<cv::Vec3b>(iter1.pos());
	lines_color1.push_back(lin1);
	for(int index=0; index<iter2.count; ++index, ++iter2)
	   lin2[index] = psrc->at<cv::Vec3b>(iter2.pos());
	lines_color2.push_back(lin1);
   }
}

void myROI::redrawLine()throw(ErrMsg) {
   int beg = pt_poly.size()-2, end = pt_poly.size()-1;
   cv::LineIterator iter1(*psrc, pt_poly.at(beg), pt_poly.at(end)),
	iter2(*psrc2, pt_poly.at(beg), pt_poly.at(end));
   if(gray) {
	std::vector<unsigned char> lin1=lines_gray1.back(), lin2=lines_gray1.back();
	if(lin1.size()!=iter1.count) {
	   sprintf(msg, "myROI::redrawLine: line pix number %d disagree with prestored"
		  " vector number %d.", iter1.count, lin1.size());
	   throw ErrMsg(msg);
	}
	for(int index=0; index<iter1.count; ++index, ++iter1)
	   psrc->at<unsigned char>(iter1.pos()) = lin1[index];
	lines_gray1.pop_back();
	for(int index=0; index<iter2.count; ++index, ++iter2)
	   psrc2->at<unsigned char>(iter2.pos()) = lin2[index];
	lines_gray2.pop_back();
   }else{
	std::vector<cv::Vec3b> lin1=lines_color1.back(), lin2=lines_color2.back();
	if(lin1.size()!=iter1.count) {
	   sprintf(msg, "myROI::redrawLine: line pix number %d disagree with prestored"
		   " vector number %d.", iter1.count, lin1.size());
	   throw ErrMsg(msg);
	}
	for(int index=0; index<iter1.count; ++index, ++iter1)
	   psrc->at<cv::Vec3b>(iter1.pos()) = lin1[index];
	lines_color1.pop_back();
	for(int index=0; index<iter2.count; ++index, ++iter2)
	   psrc2->at<cv::Vec3b>(iter2.pos()) = lin2[index];
	lines_color2.pop_back();
   }
   if(self_alloc) {
	cv::imshow(wn1, *psrc); cv::imshow(wn2, *psrc2);
   }
}

void myROI::pushPoint(const cv::Point& pt){
   if(pPoly_updated){
	for(int indx=0; indx<pt_poly.size(); ++indx)popPoint();
	clear();
   }
   pt_poly.push_back(pt);
   if(pt_poly.size()>1) {
	if(redrawable) pushLine(false);
	dashLine(*psrc, pt_poly[pt_poly.size()-2], pt);
	dashLine(*psrc2, pt_poly[pt_poly.size()-2], pt);
	// use cv::line using same params to draw solid line
	if(!gray) {
	   cv::imshow(wn1, *psrc); cv::imshow(wn2, *psrc2);
	}
   }
   pPoly_updated = false;
}

const cv::Point myROI::popPoint() {
   if(!pt_poly.empty()) {
	cv::Point unget = pt_poly.back();
	if(pt_poly.size()>1) {
	   dashLine(*psrc, unget, pt_poly[pt_poly.size()-2]);
	   dashLine(*psrc2, unget, pt_poly[pt_poly.size()-2]);
	   if(redrawable) redrawLine();
	   if(self_alloc) {
		cv::imshow(wn1, *psrc); cv::imshow(wn2, *psrc2);
	   }
	}
	cv::Point last=pt_poly.back();
	pt_poly.pop_back();
	return last;
   }
   return cv::Point(-1,-1);
}

void myROI::getROI() {
   const int npts = pt_poly.size();
   if(npts<2)return;
   if(redrawable)pushLine(true);
   dashLine(*psrc, pt_poly.back(), pt_poly.front());
   dashLine(*psrc2, pt_poly.back(), pt_poly.front());
   if(!pPoly_updated) {
	delete[] pt_array;
	pt_array = new cv::Point[npts];
	for(int index=0; index<npts; ++index)pt_array[index] = pt_poly[index];
	pPoly_updated = true;
   }
   const cv::Point* ppts[]={pt_array};
   cv::fillPoly(*region, ppts, &npts, 1, cv::Scalar(180, 180, 180));
   flash = state1;
   if(self_alloc) {
	cv::imshow(wn1, *psrc); cv::imshow(wn2, *psrc2);
   }
}

void myROI::dump()const {
   puts("==========ROI Data:==========");
   printf("Ptr array %s.\n", pPoly_updated?"clean":"dirty");
   For(cv::Point iter, pt_poly)printf("[%d, %d] ", iter.x,iter.y);
   puts("\n====================");
}

void myROI::write(FILE* fs)throw(ErrMsg){
   if(!ready){
	fputs("myROI::write: state not ready\n",stderr);
	fclose(fs); return;
   }
   fprintf(fs,"%d%s%d%s%d\n", psrc->rows, delim, psrc->cols, delim, pt_poly.size());
   For(cv::Point iter, pt_poly)
	fprintf(fs,"[%d%s%d]\n", iter.x, delim, iter.y);
   fclose(fs);
}

void myROI::read(FILE* fs)throw(ErrMsg){
   if(ready) fputs("Warning: myROI::read: state ready to be saved, but "
	   "loading ROI requested.\nSelected ROI abandoned.\n",
	   stderr);
   size_t lineCap=128; char lineC[lineCap+1], *line=lineC;
   if(getline(&line, &lineCap, fs)==-1){
	fclose(fs); throw ErrMsg("myROI::read: cannot read 1st line.");
   }
   char* vals[]={strtok(line,delim), strtok(0,delim), strtok(0,"\n")},
	*tail1, *tail2, *tail3,**tailptr[3]={&tail1, &tail2, &tail3};
   long width=strtol(vals[1],tailptr[0],10), height=strtol(vals[0],tailptr[1],10),
	  nRec=strtol(vals[2],tailptr[2],10);
   if(!(*tailptr[0]&& *tailptr[1]&& *tailptr[2])){
	fclose(fs); throw ErrMsg("myROI::read: data corrupted on line #1.\n");
   }else if(width!=psrc->cols || height!=psrc->rows){
	sprintf(msg, "myROI::read: dimension disagreement: [%ld %ld]!=[%d %d] "
		"on line #1.\n", width, height, psrc->cols, psrc->rows);
	fclose(fs); throw ErrMsg(msg);
   }
   std::vector<cv::Point> tmp(nRec);
   for(int indx=0; indx<nRec; ++indx)
	if(getline(&line, &lineCap, fs)==-1){
	   sprintf(msg,"myROI::read: data corrupted on line #%d: EOF\n", indx+2);
	   fclose(fs); throw ErrMsg(msg);
	}else{
	   vals[0]=strtok(line,delim); vals[1]=strtok(0,"\n");
	   if(*vals[0]!='[' || *(vals[1]+strlen(vals[1])-1)!=']'){
		sprintf(msg,"myROI::read: data corrupted on line #%d: "
			"missing bracket.\n", indx+2);
		fclose(fs); throw ErrMsg(msg);
	   }
	   *(vals[1]+strlen(vals[1])-1)=0;
	   tmp[indx].x = strtol(vals[0]+1,tailptr[0],10);
	   tmp[indx].y = strtol(vals[1],tailptr[1],10);
	   if(!(*tailptr[0]&&*tailptr[1])){
		sprintf(msg,"myROI::read: data corrupted on line #%d: "
			"invalid number\n", indx+2);
		fclose(fs); throw ErrMsg(msg);
	   }
	}
   fclose(fs); clear();
   For(cv::Point iter, tmp)pushPoint(iter);
   getROI();
   if(::verbose)dump();
}

bool myROI::chkConvexPolygon(std::vector<cv::Point> v){
   if(v.size()<3)return false;   // Jarvis
   if(v.size()==3)return (v[0].y-v[1].y)*(v[1].x-v[2].x) !=
	(v[0].x-v[1].x)*(v[1].y-v[2].y);
   for(int indx=1; indx<v.size(); ++indx)
	if((v[indx-1].y-v[indx].y)*(v[indx].x-v[(indx+1)%v.size()].x) ==
		(v[indx-1].x-v[indx].x)*(v[indx].y-v[(indx+1)%v.size()].y))
	   v.erase(v.begin()+indx);
   bool max = deg(v[0],v[1],v[2])>deg(v[0],v[1],v[3]);
   for(int indx=1; indx<v.size(); ++indx){
	double m=deg(v[indx-1],v[indx],v[indx+1]), cur;
	for(int indx2=indx+2; indx2<v.size(); ++indx2)
	   if(max && (cur=deg(v[indx-1],v[indx],v[indx2]))>m ||
		   !max && cur<m) return false;
   }
   return true;
}

inline double myROI::deg(const cv::Point& prev, const cv::Point& from,
	const cv::Point& to){
   double a1 = atan2(to.y-from.y,to.x-from.x),
   a0 = atan2(from.y-prev.y,from.x-prev.x),
   dd=(a0<0?2*M_PI-a0:a0) - (a1<0?2*M_PI-a1:a1);
   while(dd<0)dd+=2*M_PI;
   while(dd>=2*M_PI)dd-=2*M_PI;
   return dd;
}

//++++++++++++++++++++++++++++++++++++++++
// Filter1D

inline const float Filter1D::sinc(const float& val){
   return val==0 ? 1:sin(val)/val;
}

Filter1D::Filter1D(const FilterType& ft, const int& n, float**const params,
	bool norm)throw(ErrMsg):curFT(ft),order(n),param(params),norm(norm){
   if(!param && (curFT==Tukey || curFT==Gaussian)){
	sprintf(msg, "Filter1D::ctor: param NULL"); throw(msg);
   }
   coefs.reserve(order); set();
}

void Filter1D::update(const FilterType& ft, const int& n, float**const params)
   throw(ErrMsg){
   if(!param && (curFT==Tukey || curFT==Gaussian)){
	sprintf(msg, "Filter1D::ctor: param NULL"); throw(msg);
   }
   coefs.clear();
   curFT=ft; order=n; param=params;
   set();
}

void Filter1D::set()throw(ErrMsg){
   if(order<1) throw ErrMsg("Filter1D::set: FIR order<1\n");
   int indx; float tmp, a0, a1, a2, tmp1, tmp2;
   switch(curFT){	   // un-normalized coefs
	case Rect:
	   for(indx=0; indx<order; ++indx)coefs.push_back(1.);break;
	case Hann:
	   for(indx=0; indx<order; ++indx)
		coefs.push_back(.5-.5*cos(2*M_PI*indx/(order-1)));
	   break;
	case Hamming:
	   for(indx=0; indx<order; ++indx)
		coefs.push_back(.54-.46*cos(2*M_PI*indx/(order-1)));
	   break;
	case Tukey:
	   if(*param[0]<0 || *param[0]>1){
		sprintf(msg, "Filter1D::set Tukey: invalid param alpha=%f\n\t"
			"Supposedly in [0, 1]", *param); throw(msg);
	   }
	   for(indx=0; indx<=*param[0]*(order-1)/2; ++indx)
		coefs.push_back(.5+.5*cos(M_PI*(2*indx/(*param[0]*(order-1))-1)));
	   for(indx=*param[0]*(order-1)/2; indx<=(order-1)*(1-*param[0]/2); ++indx)
		coefs.push_back(1);
	   for(indx=(order-1)*(1-*param[0]/2); indx<order; ++indx)
		coefs.push_back(.5+.5*cos(M_PI*(2*indx/(*param[0]*(order-1))-
				2/(*param[0])+1)));
	   break;
	case Cosine:
	   for(indx=0; indx<order; ++indx)coefs.push_back(sin(M_PI*indx/(order-1)));
	   break;
	case Lanczos:	// sinc(2*n/(N-1)-1)
	   for(indx=0; indx<order; ++indx)coefs.push_back(sinc(2*indx/(order-1)-1));
	   break;
	case Triangular:
	   tmp=(order-1.)/(order+1.);
	   for(indx=0; indx<(order-1)/2; ++indx)
		coefs.push_back(1+2*indx/(order+1)-tmp);
	   for(indx=(order-1)/2; indx<order; ++indx)
		coefs.push_back(1-2*indx/(order+1)+tmp);
	   break;
	case Bartlett:
	   for(indx=0; indx<(order-1)/2; ++indx)coefs.push_back(2*indx/(order-1));
	   for(indx=(order-1)/2; indx<order; ++indx)
		coefs.push_back(2-2*indx/(order-1));
	   break;
	case Gaussian:
	   tmp=*param[0]*(order-1)/2;
	   for(indx=0; indx<order; ++indx){
		float x = (indx-(order-1.)/2)/tmp; coefs.push_back(exp(-x*x));
	   }
	   break;
	case Bartlett_Hann:
	   a0=.62, a1=.48, a2=.38, tmp=2.*M_PI/(order-1);
	   for(indx=0; indx<order; ++indx)
		coefs.push_back(a0-a1*fabs(indx/(order-1)-.5)-a2*cos(indx*tmp));
	   break;
	case Blackman:	// 'Exact'-Blackman
	   a0=.42659, a1=.49656, a2=.076849, tmp1=2*M_PI/(order-1), tmp2=2*tmp1;
	   for(indx=0; indx<order; ++indx)
		coefs.push_back(a0-a1*cos(tmp1*indx)+a2*cos(tmp2*indx));
	   break;
	case Kaiser:	// Bessel function with \alpha and M as 2 params
	   tmp = M_PI**param[0], tmp1 = j0f(tmp);
	   for(indx=0; indx<=*param[1]; ++indx)
		coefs.push_back(j0f(tmp*sqrt(1-(2*indx/order-1)*(2*indx/order-1)))/tmp1);
	   for(indx=1+*param[1]; indx<order; ++indx)coefs.push_back(0);
	default:;
   }
   if(norm){
	float sum = std::accumulate(coefs.begin(), coefs.end(), 0.);
	if(sum>0)std::transform(coefs.begin(), coefs.end(), coefs.begin(), Divides<float>(sum));
   }
}

void Filter1D::dump()const{
   puts("==========Filter1D Coefs:==========");
   std::copy(coefs.begin(), coefs.end(), std::ostream_iterator<float>(std::cout,
		" "));
   puts("\n====================");
}

//++++++++++++++++++++++++++++++++++++++++
// VideoDFT	   (objID=0x9)

VideoDFT::DftPartition VideoDFT::DftPartitionMethod=Ring;

VideoDFT::VideoDFT(IplImage* _frame, const Transform& tr, const int& bins,
	const bool& log):frame(_frame),tr(tr),nBin(bins),log_energy(log),
   dftSz(cv::getOptimalDFTSize(frame->width),cv::getOptimalDFTSize(frame->height)),
   xCent(dftSz.width/2-1),yCent(dftSz.height/2-1),circRadius(tr==DFT||true?
	   std::min(xCent,yCent)+1 : sqrt((dftSz.width-1)*(dftSz.width-1)+
		(dftSz.height-1)*(dftSz.height-1))+1),ring_width(circRadius/nBin),
   padding(dftSz.width==frame->width&&dftSz.height==frame->height)
{init();}

void VideoDFT::init() {
   int sz=dftSz.width*dftSz.height;
   index.assign(sz,0); energyDist.assign(nBin,0.); binCnt.assign(nBin,0); 
   int indx=-1, x=0, y=0; bool rise=true; float inc=(sz+0.)/nBin;
   if(tr==DFT){
	if(DftPartitionMethod==Ring){
	   for(int y=0, indx=0; y<dftSz.height; ++y)
		for(int x=0; x<dftSz.width; ++x, ++indx)  // Euclidean distance
		   if((index[indx] = static_cast<int>(sqrt((y-yCent)*(y-yCent)+(x-xCent)
					*(x-xCent))/ring_width)) < nBin)
			++binCnt[index[indx]];	// discard outside of outmost ring
	}else{
	   std::vector<double> tang(nBin); tang[0]=-(tang[nBin-1]=DBL_MAX);
	   for(int indx=1; indx<nBin-1; ++indx)
		tang[indx]=tan(M_PI*(indx/(nBin-1.)-1)+M_PI/2);
	   for(int y=0, indx=0; y<dftSz.height; ++y)
		for(int x=0; x<dftSz.width; ++x, ++indx)
		   if(x!=xCent)
			++binCnt[index[indx] = std::distance(tang.begin(),
				std::find_if(tang.begin(), tang.end(), std::bind2nd(
					std::greater_equal<double>(),
					(0.+y-yCent)/(x-xCent))))];
		   else if(y>yCent)++binCnt[index[indx]=nBin-1];
		   else ++binCnt[index[indx]=0];
	}
   }else while(x<dftSz.width && y<dftSz.height){   // DCT: run-length zig-zag coding
	++binCnt[index[y*dftSz.width+x] = static_cast<int>(++indx/inc)];
	if(y==0){
	   if(rise){
		++x; rise=false;
	   }else{
		--x; ++y;
	   }
	}else if(y==dftSz.height-1){
	   if(!rise){
		++x; rise=true;
	   }else{
		--y; ++x;
	   }
	}else if(x==0){
	   if(!rise){
		++y; rise=true;
	   }else{
		--y; ++x;
	   }
	}else if(x==dftSz.width-1){
	   if(rise){
		++y; rise=false;
	   }else{
		++y; --x;
	   }
	}else if(rise){
	   ++x; --y;
	}else{
	   --x; ++y;
	}
   }
   update();
}

void VideoDFT::update() {
   cv::Mat padded;
   if(padding){	// TODO
	cv::Mat paddedc;
	if(frame->nChannels!=1)	   // deal with BGR image before padding
	   cvtColor(cv::Mat(frame), paddedc, CV_BGR2GRAY);
	else paddedc = cv::Mat(frame);
	copyMakeBorder(paddedc, padded, 0, dftSz.height-frame->height, 0,
		dftSz.width-frame->width, cv::BORDER_CONSTANT, cv::Scalar::all(0));
   } else {
	if(frame->nChannels!=1) cvtColor(cv::Mat(frame), padded, CV_BGR2GRAY);
	else padded = cv::Mat(frame);
   }
   cv::Mat planes[]={cv::Mat_<float>(padded),cv::Mat::zeros(padded.size(),CV_32FC1)},
	complexImg;
   float f; int indx;
   switch(tr){
	case DWT:	// TODO
	case DFT:
	   cv::merge(planes, 2, complexImg);
	   cv::dft(complexImg, complexImg);	   // calc DFT
	   cv::split(complexImg, planes);
	   cv::magnitude(planes[0], planes[1], planes[0]);
	   dftMag = planes[0];
	   break;
	case DCT:
	   padded.convertTo(padded, CV_32F);
	   cv::dct(padded, dftMag);	   // calc DFT
	default:;
   }
   if(log_energy){	   // log(1+abs(x))
	(dftMag += cv::Scalar::all(1)).convertTo(dftMag,CV_32F);
	cv::log(dftMag, dftMag);
   }
   if(tr==DFT){
	if(padding) dftMag=dftMag(cv::Rect(0, 0, dftMag.cols&-2, dftMag.rows&-2));
	// rearrange the quadrants of Fourier image to center the origin
	const int cx=dftMag.cols>>1, cy=dftMag.rows>>1;
	cv::Mat tmp, q0(dftMag, cv::Rect(0, 0, cx, cy)),
	   q1(dftMag, cv::Rect(cx, 0, cx, cy)), q2(dftMag, cv::Rect(0, cy, cx, cy)),
	   q3(dftMag, cv::Rect(cx, cy, cx, cy));
	q0.copyTo(tmp); q3.copyTo(q0); tmp.copyTo(q3);
	q1.copyTo(tmp); q2.copyTo(q1); tmp.copyTo(q2);
   }
   cv::normalize(dftMag, dftMag, 0, 1, cv::NORM_MINMAX);
   energyDist.assign(nBin, 0.);
   for(int y=indx=0; y<dftSz.width; ++y)
	for(int x=0; x<dftSz.height; ++x, ++indx)
	   if(index[indx]<nBin && (f=dftMag.at<float>(x,y))<=1 && f>0)
		energyDist[index[indx]] += dftMag.at<float>(x,y);
   for(indx=0; indx<nBin; ++indx)
	energyDist[indx]/=binCnt[indx];
}

void VideoDFT::dump()const {
   puts("==========VideoDFT Data:==========");
   std::cout<<std::setprecision(4);
   std::copy(energyDist.begin(), energyDist.end(),
	   std::ostream_iterator<double>(std::cout," "));
   putchar('\n');
}

// test DT transforms on certain patterns
void DtTest(){
   /* Testing DCT for known patterns */
   const int dim=512, ringWid=32;
   IplImage* img=cvCreateImage(cvSize(dim,dim), 8, CV_8UC1),
	*img2=cvCreateImage(cvSize(dim,dim), 8, CV_8UC1),
	*img3=cvCreateImage(cvSize(dim,dim), 8, CV_8UC1);
   memset(img->imageData, 0, img->imageSize);
   memset(img2->imageData, 0, img2->imageSize);
   memset(img3->imageData, 0, img3->imageSize);
   const int d=dim/3;
   for(int y=0; y<dim; ++y)
	for(int x=0; x<dim; ++x){
	   if(static_cast<int>(sqrt((x-(dim>>1))*(x-(dim>>1))+
			   (y-(dim>>1))*(y-(dim>>1))))/ringWid%2)
		*(img->imageData+y*img->widthStep+x)=255;   // rings
	   if(x/ringWid%2 && y/ringWid%2)
		*(img2->imageData+y*img2->widthStep+x)=255;  // checkboard
	   if(static_cast<int>(sqrt((x-x/d*d-(d>>1))*(x-x/d*d-(d>>1))+
			   (y-y/d*d-(d>>1))*(y-y/d*d-(d>>1))))/ringWid%2)
		*(img3->imageData+y*img3->widthStep+x)=255;  // multi-rings
	}
   VideoDFT dct1(img, VideoDFT::DCT, 20), dft1(img, VideoDFT::DFT, 20);
   cvShowImage("w1",img);  cv::imshow("w1t1",dct1.getDftMag()); cv::imshow("w1t2",dft1.getDftMag());
   VideoDFT dct2(img2, VideoDFT::DCT, 20), dft2(img2, VideoDFT::DFT, 20);
   cvShowImage("w2",img2); cv::imshow("w2t1",dct2.getDftMag()); cv::imshow("w2t2",dft2.getDftMag());
   VideoDFT dct3(img3, VideoDFT::DCT, 20), dft3(img3, VideoDFT::DFT, 20);
   cvShowImage("w3",img3); cv::imshow("w3t1",dct3.getDftMag()); cv::imshow("w3t2",dft3.getDftMag());
   while('q'!=cvWaitKey(0));
}

//++++++++++++++++++++++++++++++++++++++++
// Plot2D

struct Plot2D::Less1st{
   const bool operator()(const cv::Point2f& first, const cv::Point2f& second)
   {return first.x < second.x;}
};

struct Plot2D::STATE{
   // state: 0--dash; 1--gap1st; 2--dot; 3--gap2nd (used only for dashdot type)
   int state, prog;
   const int dashlen, dotlen, gaplen;
   STATE(const int& dash, const int& dot):state(0),prog(0),dashlen(dash),
   dotlen(dot),gaplen(dotlen?dotlen:dashlen){}
   void set(const int& s, const int&p){state=s; prog=p;}
   int rem()const{
	return (!state ? dashlen-prog : (state==1 || state==3)? gaplen-prog :
		dotlen-prog)-1;
   }
   void nextState(){
	prog=0;
	switch(state){
	   case 0: state=1; break;
	   case 1: state = dotlen?2:0; break;
	   case 2: state=3; break;
	   default: state=0; break;
	}
   }
};

const float Plot2D::xr_margin=.05, Plot2D::yr_margin=.05, Plot2D::dashR=5e-3,
	Plot2D::dotR=2e-3;

Plot2D::Plot2D(const cv::Size& sz, const float dim[4], const cv::Scalar& bgc,
	const cv::Scalar& fgc)throw(ErrMsg):size(sz),max_id(0),lines(),fgc(fgc),
   x_margin(size.width*xr_margin),y_margin(size.height*yr_margin),x_start(dim[0]),
   x_end(dim[1]),y_start(dim[2]),y_end(dim[3]),xlab(),ylab(),bgc(bgc),
   canvass(sz.height,sz.width,CV_8UC3,bgc),
   x_rat((sz.width-x_margin)/(x_end-x_start)),
   y_rat((sz.height-y_margin)/(y_end-y_start)),
   fontScale(std::min(x_margin,y_margin)/20){
   if(x_start>=x_end || y_start>=y_end){
	sprintf(msg, "Plot2D::ctor: X range [%f %f], Y range [%f %f] not in increasing"
		" order!", x_start, x_end, y_start, y_end); throw ErrMsg(msg);
   }
   drawAxes();
}

// cv::LineIterator should really overload operator+=, and end()
cv::LineIterator& Plot2D::selfInc(cv::LineIterator& iter, const int& inc,
	STATE& state, int& cnt){
   int indx=0;
   while(indx<inc && cnt<iter.count){
	for(; state.rem() && indx<inc && cnt<iter.count;
		++state.prog, ++iter, ++cnt, ++indx);
	if(!state.rem())state.nextState();
   }
   return iter;
}

void Plot2D::drawLine(const Line& line){
   const std::vector<cv::Point> &pos = line.get<1>();
   if(!pos.size())return;
   if(pos.size()==1)canvass.at<cv::Scalar>(pos[0])=line.get<2>();
   int dash_len;
   if(line.get<4>()!=Solid){ // sets dash_len, dot_len according to current line
	float len=0;
	for(int seg=1; seg<pos.size(); ++seg)
	   len += fabs(pos[seg].y-pos[seg-1].y) + fabs(pos[seg].x-pos[seg-1].x);
	dash_len=std::max<int>(3,len/(3*(pos.size()-1)));
   }
   switch(line.get<4>()){
	case Solid:
	   for(int seg=1; seg<pos.size(); ++seg)
		cv::line(canvass, pos[seg-1], pos[seg], line.get<2>(), line.get<3>());
	   break;
	case Dot: dash_len = std::max<int>(dash_len/2, 2);
	case Dash: {
		 STATE state(dash_len,0);
		 for(int seg=1; seg<pos.size(); ++seg){
		    cv::LineIterator iter(canvass, pos[seg-1], pos[seg]);
		    int cnt=0;
		    do{
			 cv::Point pp=iter.pos();
			 cv::line(canvass, pp, selfInc(iter,dash_len,state, cnt).pos(),
				 line.get<2>(), line.get<3>());
			 if(cnt<iter.count)selfInc(iter,dash_len,state, cnt);
		    }while(cnt<iter.count);
		 }
	    }
	   break;
	case DashDot: {
		STATE state(dash_len, dash_len/3);
		for(int seg=1; seg<pos.size(); ++seg){
		   cv::LineIterator iter(canvass, pos[seg-1], pos[seg]);
		   int cnt=0;
		    do{
			 cv::Point pp=iter.pos();
			 cv::line(canvass, pp, selfInc(iter,dash_len,state, cnt).pos(),
				 line.get<2>(), line.get<3>());
			 if(cnt<iter.count)selfInc(iter,dash_len,state, cnt);
			 if(cnt<iter.count)
			    cv::line(canvass, pp, selfInc(iter,dash_len/3,state, cnt).pos(),
				 line.get<2>(), line.get<3>());
			 if(cnt<iter.count)selfInc(iter,dash_len,state, cnt);
		    }while(cnt<iter.count);
		}
	   }
	default:;
   }
}

void Plot2D::redraw(){
   canvass.setTo(bgc); drawAxes();
   LinesIter iter=lines.find(0);
   for(int id=0; id<max_id; iter=lines.find(++id))
	if(iter!=lines.end()) drawLine(iter->second);
}

void Plot2D::dump(){
   puts("---------------Plot2D::dump()---------------");
   LinesIter iter=lines.find(0);
   for(int id=0; id<max_id; iter=lines.find(++id))
	if(iter!=lines.end()){
	   const cv::Scalar& sc = iter->second.get<2>();
	   const LineType& lt=iter->second.get<4>();
	   printf("ID=%d: Thickness=%d, Line-type=%s Color=[%d %d %d], "
		   "#pt=%d\n\tdata={", id, iter->second.get<3>(), lt==Solid?"solid":(
			lt==Dot?"dotted": (lt==Dash?"dash":"dotted-dash")),
		   static_cast<int>(sc.val[0]), static_cast<int>(sc.val[1]),
		   static_cast<int>(sc.val[2]), iter->second.get<0>().size());
	   const std::vector<cv::Point2f> &data=iter->second.get<0>();
	   for(int indx=0; indx<data.size(); ++indx)
		printf("(%f, %f), ", data[indx].x, data[indx].y);
	   fputs("},\n\tPoints={", stdout);
	   const std::vector<cv::Point> &pt = iter->second.get<1>();
	   for(int indx=0; indx<pt.size(); ++indx)
		printf("(%d, %d), ", pt[indx].x, pt[indx].y);
	   fputs("}\n\n", stdout);
	}
   puts("------------------------------");
}

void Plot2D::drawAxes(){
   cv::line(canvass, cv::Point(x_margin-1,0), cv::Point(x_margin-1,size.height-1),
	   fgc, fontScale);
   cv::line(canvass, cv::Point(0,size.height-y_margin),
	   cv::Point(size.width-1,size.height-y_margin), fgc, fontScale);
   labelAxes();
}

const int Plot2D::set(const float* x, const float* y, const int& len, const
	cv::Scalar& fgc, const int& thick, const LineType& lt)throw(ErrMsg){
   std::vector<float> vx(len), vy(len);
   for(int indx=0; indx<len; ++indx){
	vx.push_back(x[indx]); vy.push_back(y[indx]);
   }
   set(vx, vy, fgc, thick, lt);
}

const int Plot2D::set(const std::vector<float>& x, const std::vector<float>& y,
	const cv::Scalar& fgc, const int& thick, const LineType& lt)
   throw(ErrMsg){
   const int len=x.size();
   std::vector<cv::Point2f> Ptf; Ptf.reserve(len);
   for(int indx=0; indx<len; ++indx)
	Ptf.push_back(cv::Point2f(x[indx], y[indx]));
   std::sort(Ptf.begin(), Ptf.end(), Less1st());
   Ptf.erase(std::unique(Ptf.begin(), Ptf.end()), Ptf.end());
   std::vector<cv::Point> Pt; Pt.reserve(len);
   for(int indx=0; indx<len; ++indx)Pt.push_back(pf2pi(Ptf[indx]));
   Line line = boost::make_tuple(Ptf, Pt, fgc, thick, lt);
   lines.insert(std::make_pair(max_id++, line));
   drawLine(line);
   return max_id-1;
}

inline void Plot2D::reset(const int& id){
   LinesIter iter;
   if((iter=lines.find(id)) == lines.end())return;
   lines.erase(iter); redraw();
}

void Plot2D::update(const int& id, const float& x, const float& y, const bool& rm){
   LinesIter iter;
   if((iter=lines.find(id)) == lines.end())return;
   const cv::Point2f Ptf_cmp(x, y);
   VPF Ptf(iter->second.get<0>()); VP Pt(iter->second.get<1>());
   VPF::iterator iter_pf = Ptf.begin();
   VP::iterator iter_p = Pt.begin();
   int indx = 0;
   if(rm){
	for(; indx<Ptf.size() && Ptf[indx] != Ptf_cmp; ++indx, ++iter_pf, ++iter_p);
	Ptf.erase(iter_pf); Pt.erase(iter_p);
	Line line = boost::make_tuple(Ptf, Pt, iter->second.get<2>(),
		iter->second.get<3>(), iter->second.get<4>());
	lines[id] = line; redraw(); return;
   }
   const cv::Point Pt_add(x_margin+(x-x_start)*x_rat,
	   size.height-y_margin-(y-y_start)*y_rat);
   if(!Ptf.size()){
	Ptf.insert(iter_pf, Ptf_cmp); Pt.insert(iter_p, Pt_add);
	Line line = boost::make_tuple(Ptf, Pt, iter->second.get<2>(),
		iter->second.get<3>(), iter->second.get<4>());
	lines[id] = line; return;	// no drawing needed for a single pt
   }
   for(; indx<Ptf.size() && Ptf[indx].x < Ptf_cmp.x; ++indx, ++iter_pf, ++iter_p);
   Ptf.insert(iter_pf, Ptf_cmp); Pt.insert(iter_p, Pt_add);
   Line line = boost::make_tuple(Ptf, Pt, iter->second.get<2>(),
	   iter->second.get<3>(), iter->second.get<4>());
   lines[id] = line; redraw();
}

void Plot2D::update(const int& id, const float* x, const float* y, const int& len,
	const bool& rm){
   if(!len || !x || !y)return;
   std::vector<float> vx(len), vy(len);
   for(int indx=0; indx<len; ++indx){
	vx[indx]=x[indx]; vy[indx]=y[indx];
   }
   update(id, vx, vy, rm);
}

void Plot2D::update(const int& id, const std::vector<float>& x, const std::vector
	<float>& y, const bool& rm){
   LinesIter iter;
   if((iter=lines.find(id)) == lines.end())return;
   const int len = std::min(x.size(), y.size());
   if(!len)return;
   int indx=0;
   VPF Ptf(iter->second.get<0>()), Ptf_src; Ptf_src.reserve(len);
   VP Pt(iter->second.get<1>());
   VPF::iterator iter_pf=Ptf.begin();
   VP::iterator iter_p=Pt.begin();
   for(indx=0; indx<len; ++indx) Ptf_src.push_back(cv::Point2f(x[indx], y[indx]));
   std::sort(Ptf_src.begin(), Ptf_src.end(), Less1st());
   Ptf.erase(std::unique(Ptf_src.begin(), Ptf_src.end()), Ptf_src.end());
   indx=0;
   if(rm) for(;indx<len && iter_pf!=Ptf.end();)
	if(*iter_pf == Ptf_src[indx]){
	   iter_pf=Ptf.erase(iter_pf); iter_p=Pt.erase(iter_p); ++indx;
	}else{
	   ++iter_pf; ++iter_p;
	   while(indx<len && Less1st()(Ptf_src[indx], *iter_pf))++indx;
	}
   else{
	VPF::const_iterator iter_pfPrev;
	for(iter_pfPrev = iter_pf++, ++iter_p; indx<len && iter_pf!=Ptf.end();)
	if(Less1st()(*iter_pfPrev, Ptf_src[indx]) &&
		Less1st()(Ptf_src[indx], *iter_pf)){
	   iter_pfPrev = (iter_pf=Ptf.insert(iter_pf, Ptf_src[indx]))-1;
	   iter_p = Pt.insert(iter_p, pf2pi(Ptf_src[indx++]));
	}else{
	   iter_pfPrev = iter_pf++; ++iter_p;
	}
   }
   Line line = boost::make_tuple(Ptf, Pt, iter->second.get<2>(),
	   iter->second.get<3>(), iter->second.get<4>());
   lines[id] = line; redraw();
}

void Plot2D::update(const int& id, const std::vector<cv::Point2f>& pts, const bool&
	   rm){
   const int len=pts.size();
   std::vector<float> vx(len), vy(len);
   for(int indx=0; indx<len; ++indx){
	vx[indx] = pts[indx].x; vy[indx]=pts[indx].y;
   }
   update(id, vx, vy, rm);
}

void Plot2D::resize(const cv::Size& sz)throw(ErrMsg){
   if(sz==size)return;
   float rat = (size.width+0.)/sz.width; size=sz; 
   x_margin = xr_margin*size.width; y_margin = yr_margin*size.height;
   x_rat = (size.width-x_margin+0.)/(x_end-x_start);
   y_rat = (size.height-y_margin+0.)/(y_end-y_start);
   fontScale = std::min(x_margin,y_margin)/20;
   LinesIter iter;
   for(iter=lines.begin(); iter!=lines.end(); ++iter){
	std::vector<cv::Point2f> Ptf(iter->second.get<0>());
	std::vector<cv::Point> Pt(iter->second.get<1>());
	std::vector<cv::Point2f>::const_iterator iter_ptf = Ptf.begin();
	std::vector<cv::Point>::iterator iter_pt = Pt.begin();
	for(; iter_pt!=Pt.end(); ++iter_pt, ++iter_ptf)
	   *iter_pt = pf2pi(*iter_ptf);
	Line line = boost::make_tuple(Ptf, Pt, iter->second.get<2>(),
		iter->second.get<3>(), iter->second.get<4>());
	lines[iter->first] = line;
   }
   cv::resize(canvass, canvass, size); redraw();
}

void Plot2D::labelAxes(const std::vector<float>& x, const std::vector<float>& y){
   xlab.clear(); xlab.assign(x.begin(), x.end());
   ylab.clear(); ylab.assign(y.begin(), y.end());
   std::sort(xlab.begin(), xlab.end()); std::sort(ylab.begin(), ylab.end());
   xlab.erase(std::unique(xlab.begin(), xlab.end()), xlab.end());
   ylab.erase(std::unique(ylab.begin(), ylab.end()), ylab.end());
   labelAxes();
}

void Plot2D::labelAxes(){
   if(xlab.empty() && ylab.empty())return;
   char num[32];
   for(int indx=0; indx<xlab.size(); ++indx){
	if(xlab[indx]<x_start || xlab[indx]>x_end)continue;
	sprintf(num, "%.1f", xlab[indx]);
	cv::putText(canvass, num, cv::Point(x_margin+(xlab[indx]-x_start)*x_rat,
		   size.height-2), 1, fontScale, fgc, fontScale);
	cv::rectangle(canvass, cv::Point(x_margin+(xlab[indx]-x_start)*x_rat-fontScale,
		   size.height-y_margin-fontScale),
		cv::Point(x_margin+(xlab[indx]-x_start)*x_rat+fontScale,
			size.height-y_margin+fontScale), fgc);
   }
   for(int indx=0; indx<ylab.size(); ++indx){
	if(ylab[indx]<y_start || ylab[indx]>y_end)continue;
	sprintf(num, "%.1f", ylab[indx]);
	cv::putText(canvass, num, cv::Point(2, size.height-y_margin-(ylab[indx]-y_start)
		   *y_rat), 1, fontScale, fgc, fontScale);
	cv::rectangle(canvass, cv::Point(x_margin-1-fontScale,
		   size.height-y_margin-fontScale-y_rat*(ylab[indx]-y_start)),
		cv::Point(x_margin+fontScale,
		   size.height-y_margin-(ylab[indx]-y_start)*y_rat+fontScale), fgc);
   }
}

std::vector<float> Plot2D::findAxis(const float& min, const float& max){
   float diff=max-min;
   const int rmin=4, rmax=10; int state=1;
   float scale=1;
   while(diff<rmin*scale || diff>rmax*scale){
	if(diff>rmin*scale){
	   scale *= (state==1||state==5)? 2 : 2.5;
	   state = (state==1||state==5)?2:5;
	}else{
	   scale /= (state==1||state==2)? 2 : 2.5;
	   state = (state==1||state==2)?2:2.5;
	}
   }
   std::vector<float> ax;
   for(float f=ceilf(min/scale)*scale; f<=floorf(max/scale)*scale; f+=scale)
	ax.push_back(f);
   return ax;
}

//++++++++++++++++++++++++++++++++++++++++
// createAnimation
char createAnimation::Codec[]="MJPG";  // 'MJPG' coding seems lossless with identical frames
createAnimation::Bgvt createAnimation::bgvt=createAnimation::None;
float createAnimation::bparam[5];
bool createAnimation::WarpSimp;
const float createAnimation::golden_ratio=0.618034;
double (*createAnimation::capKernelArray[])(double)={
   &createAnimation::norm0, &createAnimation::norm1,
   &createAnimation::norm2, &createAnimation::norm3
}, (*createAnimation::capKernel)(double) =
createAnimation::capKernelArray[0];
int createAnimation::fadeDuration=1;

void createAnimation::setBgPattern(const Bgvt& b, const float* f){
   switch(bgvt=b){
	case Blur:
	   if(f)memcpy(bparam, f, 5*sizeof(float));
	   else{
		bparam[0]=CV_GAUSSIAN; bparam[1]=bparam[2]=0;
		bparam[3]=11; bparam[4]=20;
	   }
	   break;
	case Elevator:
	   bparam[0]=f?*f:5; bparam[1]=f?*(f+1):0; bparam[2]=1;
	   break;
	case Cappuccino:	// center (x,y), amplitude, normalized span/radius in (0, 1),
	   if(f)memcpy(bparam,f,5*sizeof(float));   // and direction (clock-wise: >=0)
	   else{
		bparam[0]=bparam[1]=-1; bparam[2]=.03/*amplitude*/;
		bparam[3]=1/* span */; bparam[4]=-1/* direction */;
	   }
	case Vapor:  // uniform-dist range, spatial Gaussian size, sigma, temporal Hamming size
	   if(f)memcpy(bparam, f, 4*sizeof(float));
	   else{
		bparam[0]=20; bparam[1]=11; bparam[2]=.7; bparam[3]=16;
	   }
   }
}

createAnimation::createAnimation(const char* fn, const IplImage* _bg, const
	float& duration, const float& fps)throw(ErrMsg):duration(duration),
   fps(fps),orig(_bg),add(0),bg2(0),x_map(0),y_map(0),remap(0),bgImg(true),
   vertices(new CvPoint[10]){
   if(!fn || !strlen(fn) || !_bg)throw
	ErrMsg("createAnimation::ctor: file name or bg image NULL.");
   if(_bg->nChannels!=3){
	sprintf(msg, "createAnimation::ctor: background image has %d!=3 channels ",
		_bg->nChannels); throw ErrMsg(msg);
   }
   int len=strlen(fn); name=new char[len+4]; strcpy(name,fn);
   if(strcmp(fn+len-4,".avi")){
	strcat(name,".avi"); len+=4;
   }
   name[len]=0;
   if(!(bg=cvCloneImage(orig)) || (bgvt==Blur || bgvt==Cappuccino || bgvt==Vapor)
	   && !(bg2=cvCreateImage(cv::Size(orig->width,orig->height), orig->depth,
		   orig->nChannels)))throw
	ErrMsg("createAnimation::ctor: local image copies allocation failed.");
   if(bgvt==Blur){
	cvSmooth(orig, bg2, static_cast<int>(bparam[0]), static_cast<int>(bparam[1]),
		static_cast<int>(bparam[2]), bparam[3], bparam[4]);
   }else if(bgvt==Cappuccino){
	/* Note: one disadvantage of "fast" remapping is the
 	 * presence of small block centered in [xc, yc], whose
	 * size is determined by rotation speed (span). */
	const int xc = *bparam = static_cast<int>(orig->width*(*bparam<0?.5:*bparam)-1),
		yc = *(bparam+1)=static_cast<int>(orig->height*(*(bparam+1)<0?.5:*(bparam+1))-1),
		size = orig->width*orig->height, sgn=*(bparam+4)=*(bparam+4)>=0?1:-1;
	const float &amp=*(bparam+2), radius = std::max(std::max(sqrt(xc*xc+yc*yc),
			sqrt((orig->width-xc-1)*(orig->width-xc-1)+yc*yc)),
		std::max(sqrt(xc*xc+(orig->height-yc-1)*(orig->height-yc-1)),
		   sqrt((orig->width-xc-1)*(orig->width-xc-1)+
			(orig->height-yc-1)*(orig->height-yc-1)))),
		span=*(bparam+3)=radius*(*(bparam+3)>=0?*(bparam+3):1);
	if(WarpSimp){
	   x_map=new int[size]; y_map=new int[size]; ptrdiff_t acc2=0;
	   for(ptrdiff_t indx=0; indx<orig->height; ++indx, acc2 += orig->width)
		for(ptrdiff_t indx2=0; indx2<orig->width;	// identity map
			*(y_map+acc2+indx2)=indx, *(x_map+acc2+indx2)=indx2++);
	   int *px=x_map, *py=y_map, x_start=std::max<int>(0,xc-span),
		 x_end=std::min<int>(orig->width,xc+span), y_start=std::max<int>(0,yc-span),
		 y_end=std::min<int>(orig->height,yc+span);
	   float r;
	   for(int y=y_start; y<=y_end; ++y)
		for(int x=x_start; x<=x_end; ++x)
		   if((r=sqrt((x-xc)*(x-xc)+(y-yc)*(y-yc))/span)<=1){
			float alpha=sgn*amp*capKernel(r), ca=cos(alpha), sa=sin(alpha);
			ptrdiff_t offset=y*orig->width+x;
			*(x_map+offset) = static_cast<int>(round(
				   (x-xc)*ca-(y-yc)*sa+xc+orig->width))%orig->width;
			*(y_map+offset) = static_cast<int>(round(
				   (y-yc)*ca+(x-xc)*sa+yc+orig->height))%orig->height;
		   }
	}
   }else if(bgvt==Vapor){
	if(verbose)puts("Space allocation...");
	if(!(remap = new(std::nothrow)float*[static_cast<int>(fps*duration)]))
	   throw ErrMsg("createAnimation::ctor: heap allocation failed.");
	int alloc_tries=0;
	for(int indx=0; indx<fps*duration; ++indx)
	   if(!(remap[indx]=new(std::nothrow)float[2*bg->width*bg->height])){
		while(alloc_tries++<5){
		   printf("Heap allocation failed %d. Retrying..."); sleep(2);
		   if((remap[indx]=new(std::nothrow)float[2*bg->width*bg->height]))
			break;
		}
		if(alloc_tries<5)continue;
		for(int j=0; j<indx; ++j)delete[]remap[j]; delete[]remap;
		throw ErrMsg("createAnimation::ctor: heap allocation failed.");
	   }
	srand(time(0));
	for(int indx=0; indx<fps*duration; ++indx)
	   for(int pos=0; pos<2*bg->width*bg->height; ++pos)
		remap[indx][pos] = 2*bparam[0]*(static_cast<float>(rand())/RAND_MAX-.5);
	// Gaussian kernel separatable in 1D
	if(verbose)puts("Spatial filtering...");
	float *sigma=bparam+2;
	Filter1D filterG(Filter1D::Gaussian, static_cast<int>(bparam[1]), &sigma, true);
	ARMA_Array<float> filtS1(bparam[1], &*(filterG.getCoefs().begin()), 0,0),
	   filtS2(filtS1, false);
	float tmp[static_cast<int>(fps*duration)];
	for(int indx=0; indx<fps*duration; ++indx){   // spatial filtering separated
	   for(int c=0; c<bg->height; ++c){
		for(int r=0; r<bg->width; ++r)
		   access(indx,c,r,false)=filtS1.append(access(indx,c,r,false)),
			access(indx,c,r,true) =filtS2.append(access(indx,c,r,true));
		filtS1.clear(); filtS2.clear();
	   }
	   for(int r=0; r<bg->width; ++r){
		for(int c=0; c<bg->height; ++c)
		   access(indx,c,r,false)=filtS1.append(access(indx,c,r,false)),
			access(indx,c,r,true) =filtS2.append(access(indx,c,r,true));
		filtS1.clear(); filtS2.clear();
	   }
	}
	if(verbose)puts("Temporal filtering...");
	Filter1D filter(Filter1D::Hamming, bparam[3], 0);
	ARMA_Array<float> filtT1(bparam[3], &*(filter.getCoefs().begin()), 0,0),
	filtT2(filtT1,false);
	for(int c=0; c<bg->height; ++c)	// temporal filtering
	   for(int r=0; r<bg->width; ++r){
		for(int indx=0; indx<fps*duration; ++indx)
		   access(indx,c,r,false)+=filtT1.append(access(indx,c,r,false)),
		   access(indx,c,r,true)+=filtT2.append(access(indx,c,r,true));
		filtT1.clear(); filtT2.clear();
	   }
	for(int indx=0; indx<fps*duration; ++indx)
	   for(int c=0; c<bg->height; ++c)
		for(int r=0; r<bg->width; ++r){	// float modulo
		   access(indx,c,r,true)+=r; access(indx,c,r,false)+=c;
		   while(access(indx,c,r,true)<0)access(indx,c,r,true)+=bg->width;
		   while(access(indx,c,r,true)>=bg->width)access(indx,c,r,true)-=bg->width;
		   while(access(indx,c,r,false)<0)access(indx,c,r,false)+=bg->height;
		   while(access(indx,c,r,false)>=bg->height)access(indx,c,r,false)-=bg->height;
		   assert(access(indx,c,r,true)>=0 && access(indx,c,r,true)<bg->width &&
			  access(indx,c,r,false)>=0 && access(indx,c,r,false)<bg->height);
		}
	if(verbose)puts("Frame calculating...");
   }
   memcpy(bg->imageData, orig->imageData, orig->imageSize);
}

createAnimation::createAnimation(const char* fn, const CvScalar& _bg, const
	cv::Size& size, const float& duration, const float& fps)throw(ErrMsg):
   bg2(0),orig(0),x_map(0),y_map(0),remap(0),duration(duration),fps(fps),
   add(0),bgImg(true),vertices(new CvPoint[10]){
   if(!fn || !strlen(fn))throw
	ErrMsg("createAnimation::ctor: file name or bg image NULL.");
   int len=strlen(fn); name=new char[len+4]; strcpy(name,fn);
   if(strcmp(fn+len-4,".avi")){
	strcat(name,".avi"); len+=4;
   }
   name[len]=0;
   if(!(bg=cvCreateImage(size, 8, 3)))throw
	ErrMsg("createAnimation::ctor: local image copy allocation failed.");
   char* data=bg->imageData, col[3]={static_cast<char>(_bg.val[0]), static_cast<char>
	(_bg.val[1]), static_cast<char>(_bg.val[2])};
   for(int h=0; h<size.height; ++h)
	for(int w=0; w<size.width; ++w, memcpy(data, col, 3), data+=3);
}

createAnimation::~createAnimation(){
   delete[] name; delete[] vertices;
   cvReleaseImage(&bg);
   if(add)cvReleaseImage(&add);
   if(bg2)cvReleaseImage(&bg2);
   if(x_map)delete[]x_map, delete[]y_map;
   if(remap){
	for(int indx=0; indx<fps*duration; ++indx)delete[]remap[indx];
	delete[]remap;
   }
}

int createAnimation::addObj(const ObjProp& op){
   const CvScalar &s=op.get<3>(); const float &x=op.get<1>().x, &y=op.get<1>().y;
   if((bgImg || s.val[0]==*(bg->imageData) && s.val[1]==*(bg->imageData+1) &&
		s.val[2]==*(bg->imageData+2)) && x>=op.get<2>() &&
	   x<bg->width-op.get<2>() && y>=op.get<2>() && y<bg->height-op.get<2>()
	   && std::find_if(objects.begin(), objects.end(), std::bind2nd(equal_to(),
		   op))==objects.end())
	objects.push_back(op);
   return objects.size();
}

void createAnimation::dump()const{
   int indx=1;
   for(std::vector<ObjProp>::const_iterator iter=objects.begin();
	   iter!=objects.end(); ++iter, ++indx)
	printf("[%d]: %s @(%.2f, %.2f), radius=%.2f, color=[%d %d %d], m_speed=%.2f, m_angle=%.2f, "
	   "r_speed=%.2f, \n", indx, iter->get<0>()==Ball?"Ball":(iter->get<0>()==Triangle?
		"Triag": (iter->get<0>()==Square?"Square":(iter->get<0>()==Pentagon?"Pent":
			(iter->get<0>()==Star?"Star":"Cres")))),
	   iter->get<1>().x, iter->get<1>().y, iter->get<2>(),
	   static_cast<unsigned char>(iter->get<3>().val[0]),
	   static_cast<unsigned char>(iter->get<3>().val[1]),
	   static_cast<unsigned char>(iter->get<3>().val[2]), iter->get<4>(), iter->get<5>(),
	   iter->get<6>());
}

void createAnimation::drawObj(ObjProp& op, const int& fcnt, IplImage* image){
   if(!image)return;
   int npt; const float frac=(fcnt+0.)/fps;
   switch(op.get<0>()){
	case Triangle:
	   vertices[0] = cv::Point(op.get<1>().x+op.get<2>()*cos(op.get<6>()*frac),
		   op.get<1>().y+op.get<2>()*sin(op.get<6>()*frac));
	   vertices[1] = cv::Point(op.get<1>().x+op.get<2>()*cos(op.get<6>()*frac+2*M_PI/3),
		   op.get<1>().y+op.get<2>()*sin(op.get<6>()*frac+2*M_PI/3));
	   vertices[2] = cv::Point(op.get<1>().x+op.get<2>()*cos(op.get<6>()*frac-2*M_PI/3),
		   op.get<1>().y+op.get<2>()*sin(op.get<6>()*frac-2*M_PI/3));
	   cvFillPoly(image, &vertices, &(npt=3), 1, op.get<3>());
	   break;
	case Square:
	   vertices[0] = cv::Point(op.get<1>().x+op.get<2>()*cos(op.get<6>()*frac+M_PI/4),
		   op.get<1>().y+op.get<2>()*sin(op.get<6>()*frac+M_PI/4));
	   vertices[1] = cv::Point(op.get<1>().x+op.get<2>()*cos(op.get<6>()*frac+3*M_PI/4),
		   op.get<1>().y+op.get<2>()*sin(op.get<6>()*frac+3*M_PI/4));
	   vertices[2] = cv::Point(op.get<1>().x+op.get<2>()*cos(op.get<6>()*frac-3*M_PI/4),
		   op.get<1>().y+op.get<2>()*sin(op.get<6>()*frac-3*M_PI/4));
	   vertices[3] = cv::Point(op.get<1>().x+op.get<2>()*cos(op.get<6>()*frac-M_PI/4),
		   op.get<1>().y+op.get<2>()*sin(op.get<6>()*frac-M_PI/4));
	   cvFillPoly(image, &vertices, &(npt=4), 1, op.get<3>());
	   break;
	case Pentagon:
	   vertices[0] = cv::Point(op.get<1>().x+op.get<2>()*cos(op.get<6>()*frac),
		   op.get<1>().y+op.get<2>()*sin(op.get<6>()*frac));
	   vertices[1] = cv::Point(op.get<1>().x+op.get<2>()*cos(op.get<6>()*frac+M_PI*.4),
		   op.get<1>().y+op.get<2>()*sin(op.get<6>()*frac+M_PI*.4));
	   vertices[2] = cv::Point(op.get<1>().x+op.get<2>()*cos(op.get<6>()*frac+M_PI*.8),
		   op.get<1>().y+op.get<2>()*sin(op.get<6>()*frac+M_PI*.8));
	   vertices[3] = cv::Point(op.get<1>().x+op.get<2>()*cos(op.get<6>()*frac+M_PI*1.2),
		   op.get<1>().y+op.get<2>()*sin(op.get<6>()*frac+M_PI*1.2));
	   vertices[4] = cv::Point(op.get<1>().x+op.get<2>()*cos(op.get<6>()*frac+M_PI*1.6),
		   op.get<1>().y+op.get<2>()*sin(op.get<6>()*frac+M_PI*1.6));
	   cvFillPoly(image, &vertices, &(npt=5), 1, op.get<3>());
	   break;
	case Star:
	   vertices[0] = cv::Point(op.get<1>().x+op.get<2>()*cos(op.get<6>()*frac),
		   op.get<1>().y+op.get<2>()*sin(op.get<6>()*frac));
	   vertices[1] = cv::Point(op.get<1>().x+.5*op.get<2>()*cos(op.get<6>()*frac+M_PI*.2),
		   op.get<1>().y+.5*op.get<2>()*sin(op.get<6>()*frac+M_PI*.2));
	   vertices[2] = cv::Point(op.get<1>().x+op.get<2>()*cos(op.get<6>()*frac+M_PI*.4),
		   op.get<1>().y+op.get<2>()*sin(op.get<6>()*frac+M_PI*.4));
	   vertices[3] = cv::Point(op.get<1>().x+.5*op.get<2>()*cos(op.get<6>()*frac+M_PI*.6),
		   op.get<1>().y+.5*op.get<2>()*sin(op.get<6>()*frac+M_PI*.6));
	   vertices[4] = cv::Point(op.get<1>().x+op.get<2>()*cos(op.get<6>()*frac+M_PI*.8),
		   op.get<1>().y+op.get<2>()*sin(op.get<6>()*frac+M_PI*.8));
	   vertices[5] = cv::Point(op.get<1>().x+.5*op.get<2>()*cos(op.get<6>()*frac+M_PI),
		   op.get<1>().y+.5*op.get<2>()*sin(op.get<6>()*frac+M_PI));
	   vertices[6] = cv::Point(op.get<1>().x+op.get<2>()*cos(op.get<6>()*frac+M_PI*1.2),
		   op.get<1>().y+op.get<2>()*sin(op.get<6>()*frac+M_PI*1.2));
	   vertices[7] = cv::Point(op.get<1>().x+.5*op.get<2>()*cos(op.get<6>()*frac+M_PI*1.4),
		   op.get<1>().y+.5*op.get<2>()*sin(op.get<6>()*frac+M_PI*1.4));
	   vertices[8] = cv::Point(op.get<1>().x+op.get<2>()*cos(op.get<6>()*frac+M_PI*1.6),
		   op.get<1>().y+op.get<2>()*sin(op.get<6>()*frac+M_PI*1.6));
	   vertices[9] = cv::Point(op.get<1>().x+.5*op.get<2>()*cos(op.get<6>()*frac+M_PI*1.8),
		   op.get<1>().y+.5*op.get<2>()*sin(op.get<6>()*frac+M_PI*1.8));
	   cvFillPoly(image, &vertices, &(npt=10), 1, op.get<3>());
	   break;
	case Ball:
	   vertices[0]=cv::Point(op.get<1>().x-op.get<2>(), op.get<1>().y-op.get<2>());
	   vertices[1]=cv::Point(op.get<1>().x+op.get<2>(), op.get<1>().y+op.get<2>());
	   cvCircle(image, op.get<1>(), op.get<2>(), op.get<3>(), -1);
	   npt=2; break;
	case Crescent:
	   vertices[0]=cv::Point(op.get<1>().x+op.get<2>()*sin(op.get<6>()*frac),
		   op.get<1>().y-op.get<2>()*cos(op.get<6>()*frac));
	   vertices[1]=cv::Point(op.get<1>().x-op.get<2>()*sin(op.get<6>()*frac),
		   op.get<1>().y+op.get<2>()*cos(op.get<6>()*frac));
	   vertices[2]=cv::Point(op.get<1>().x+op.get<2>()*cos(op.get<6>()*frac-.75*M_PI),
		   op.get<1>().y+op.get<2>()*sin(op.get<6>()*frac-.75*M_PI));
	   vertices[2]=cv::Point(op.get<1>().x+op.get<2>()*cos(op.get<6>()*frac+.75*M_PI),
		   op.get<1>().y+op.get<2>()*sin(op.get<6>()*frac+.75*M_PI));
	   drawCrescent(image, op, frac); npt=4;
   }
   float x_min, x_max, y_min, y_max;
   x_min=x_max=vertices[0].x; y_min=y_max=vertices[0].y;
   for(int indx=1; indx<npt; ++indx){  // prepare for collision test
	if(vertices[indx].x<x_min)x_min=vertices[indx].x;
	else if(vertices[indx].x>x_max)x_max=vertices[indx].x;
	if(vertices[indx].y<y_min)y_min=vertices[indx].y;
	else if(vertices[indx].y>y_max)y_max=vertices[indx].y;
   }
   if(x_min<0){
	op.get<1>().x -= 2*x_min; op.get<5>()=M_PI-op.get<5>();
   }else if(x_max>=image->width){
	op.get<1>().x -= 2*(x_max-image->width+1); op.get<5>()=M_PI-op.get<5>();
   }else op.get<1>().x += op.get<4>()*cos(op.get<5>())/fps;
   if(y_min<0){
	op.get<1>().y -= 2*y_min; op.get<5>()=-op.get<5>();
   }else if(y_max>image->height){
	op.get<1>().y -= 2*(y_max-image->height+1); op.get<5>()=-op.get<5>();
   }else op.get<1>().y += op.get<4>()*sin(op.get<5>())/fps;
}

void createAnimation::drawCrescent(IplImage* image, const ObjProp& op,
	const float& frac){
  const unsigned char col0=static_cast<unsigned char>(op.get<3>().val[0]),
  col1=static_cast<unsigned char>(op.get<3>().val[1]),
  col2=static_cast<unsigned char>(op.get<3>().val[2]);
  const float &r=op.get<2>(), r2=r*r, &x1=op.get<1>().x, &y1=op.get<1>().y,
	  x2=x1+golden_ratio*r*cos(op.get<6>()*frac),
	  y2=y1+golden_ratio*r*sin(op.get<6>()*frac);
  int index;   // point-wise rendering
  for(int x=std::max<int>(0,x1-r); x<std::min<int>(image->width,x1+r+1); ++x)
     for(int y=std::max<int>(0,y1-r); y<std::min<int>(image->height,y1+r+1); ++y)
	  if((x-x1)*(x-x1)+(y-y1)*(y-y1)<=r2 && (x-x2)*(x-x2)+(y-y2)*(y-y2)>r2 &&
		  (index=y*image->widthStep+x*3)){
	     *(image->imageData+index++)=col0; *(image->imageData+index++)=col1;
	     *(image->imageData+index)=col2;
	  }
}

void createAnimation::rPopulate(const int& n){
   objects.clear(); srand(time(0));
   int sz=std::min(bg->width, bg->height)>>2, tries=0;
   while(objects.size()< n && ++tries<n<<2)
	addObj(boost::make_tuple(static_cast<Object>(rand()%6),
		cv::Point2f(rand()%bg->width, rand()%bg->height), rand()%sz,
		cv::Scalar(rand()%256, rand()%256, rand()%256), rand()%(sz),
		rand()%200*M_PI/100, (rand()%400-199)*M_PI/200, rand()%2));
}

void createAnimation::warp(){
   // No checking for legitness of px/py/bg. bg2 constant
   char *src, *dest=bg->imageData;
   if(bgvt==Cappuccino){   // coordinates remapping from orig to bg using x_map, y_map
	if(WarpSimp){
	   memcpy(src=bg2->imageData, dest, orig->imageSize);
	   int *px=x_map, *py=y_map, w=bg->widthStep;
	   for(int y=0; y<orig->height; ++y)
		for(int x=0; x<orig->width; ++x, ++px, ++py, dest+=3)
		   memcpy(dest, src+*py*w+*px*3, 3);
	}else{	// re-caulculate per-frame
	   const int &xc=*bparam, &yc=*(bparam+1), &span=*(bparam+3), &sgn=*(bparam+4),
		   x_start=std::max<int>(0,xc-span), x_end=std::min<int>(orig->width,xc+span),
		   y_start=std::max<int>(0,yc-span), y_end=std::min<int>(orig->height,yc+span);
	   float &amp=*(bparam+2), r;
	   src=orig->imageData;
	   for(int y=y_start; y<=y_end; ++y)
		for(int x=x_start; x<=x_end; ++x)
		   if((r=sqrt((x-xc)*(x-xc)+(y-yc)*(y-yc))/span)<=1){
			float alpha=sgn*fc*amp*capKernel(r), ca=cos(alpha), sa=sin(alpha);
			ptrdiff_t dest_offset=y*orig->widthStep+x*3, src_offset=static_cast<int>
			   (round((y-yc)*ca+(x-xc)*sa+yc+orig->height))%orig->height*orig->widthStep
			   +static_cast<int>(round((x-xc)*ca-(y-yc)*sa+xc+orig->width))%orig->width*3;
			memcpy(dest+dest_offset, src+src_offset, 3);
		   }
	}
   }else{
	src=orig->imageData;
	for(int y=0; y<orig->height; ++y)	   // Vapor
	   for(int x=0; x<orig->width; ++x, src+=3)memcpy(dest+static_cast<int>(access(fc,y,x,false))
		*bg->widthStep+static_cast<int>(access(fc,y,x,true))*3, src, 3);
   }
}

void createAnimation::factory()throw(ErrMsg){
   if(FILE* fp=fopen(name, "r")){
	fprintf(stderr, "createAnimation::factory:\"%s\" exists. Overwritten.\n",
		name); fflush(stderr); fclose(fp);

   }
   CvVideoWriter* write;
   try{
	write = cvCreateVideoWriter(name, CV_FOURCC(Codec[0],Codec[1],Codec[2],Codec[3]),
		fps, cvSize(bg->width,bg->height), bg->nChannels);
   }catch(const cv::Exception& ex){
	sprintf(msg,"%s\ncreateAnimation::factory: failed to initialize video writer"
	     "	\"%s\" for\"%s\" format.\n", ex.what(), name, Codec);
	throw ErrMsg(msg);
   }
   if(std::find_if(objects.begin(), objects.end(), hasAdd())!=objects.end()){
	add=cvCreateImage(cvSize(bg->width,bg->height), bg->depth, bg->nChannels);
	memset(add->imageData,0,add->imageSize);
   }
   const bool addMoveable = std::find_if(objects.begin(), objects.end(),
	   hasAddMoveable())!=objects.end();
   typedef std::vector<ObjProp>::iterator Iter;
   if(add && !addMoveable)	// only static add-able objects
	for(Iter iter=objects.begin(); iter!=objects.end(); ++iter)
	   if(iter->get<7>())drawObj(*iter, 0, add);
   int sz=fps*duration, frp=0, hsec=1; fc=-1;
   IplImage* tmp=cvCreateImage(cvSize(bg->width,bg->height), bg->depth, bg->nChannels);
   while(++fc<sz){
	if(orig)
	   if(bgvt==Blur && bg2){   // interpolate between [orig, bg2] to bg
		float rat=(0.+fc)/(fps*fadeDuration), r=rat-static_cast<int>(rat);
		if(static_cast<int>(rat)%(fadeDuration<<1)%2)
		   cvAddWeighted(orig, r, bg2, 1-r, 0, bg);
		else cvAddWeighted(bg2, r, orig, 1-r, 0, bg);
	   }else if(bgvt==Elevator)VideoRegister::solidMoveAdd(bg, bparam);
	   else if(bgvt==Cappuccino && bg2 || bgvt==Vapor)warp();
	memcpy(tmp->imageData, bg->imageData, bg->imageSize);
	for(Iter iter=objects.begin(); iter!=objects.end(); ++iter)
	   drawObj(*iter, fc, iter->get<7>()?add:tmp);
	if(add){
	   cvAdd(tmp,add,tmp);
	   if(addMoveable)memset(add->imageData,0,add->imageSize);
	}
	cvWriteFrame(write,tmp);
   }
   cvReleaseVideoWriter(&write);
   cvReleaseImage(&tmp);
   if(add)cvReleaseImage(&add);
}

