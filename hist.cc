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
#include "hist.hh"
#include "dynan.hh"
#include "conf.hh"

void setIplImage(IplImage*im, const char& brightness){
   if(im)memset(im->imageData, brightness, im->imageSize*sizeof(char));
}

const float calcImgDiff(const IplImage* img1, const IplImage* img2, const
	IplImage* mask, const int& norm) { /* same image sizes assumed. */
   if(!img1 || !img2) return 0;
   if(mask && mask->nChannels>1) {
	sprintf(msg,"calcImgDiff: mask channel %d>1", mask->nChannels);
	throw ErrMsg(msg);
   }
   const int width=img1->width, height=img1->height, chan1 = img1->nChannels,
	   chan2 = img2->nChannels, img_sz=width*height;
   if(img2->width!=width || img2->height!=height || mask && (mask->width!=width
		|| mask->height!=height)) {
	mask? sprintf(msg,"calcImgDiff: mask size disagree with image: "
		"[%d %d]/[%d %d]/[%d %d]", width,height, img2->width,img2->height,
		mask->width,mask->height) :
	   sprintf(msg, "calcImgDiff: 2 image sizes disagree: [%d %d]/[%d %d]",
		   width, height, img2->width, img2->height);
	throw ErrMsg(msg);
   }
   const char *mat1=img1->imageData, *mat2=img2->imageData, *matMask=mask?mask->imageData:0;
   float mav=0; size_t src_index, dest_index;
   if(norm < -1) {
	if(!matMask && chan1==chan2)return memcmp(mat1,mat2,img1->imageSize);
	else if(!matMask){
	   for(int y=src_index=dest_index=0; y<height; ++y)
		for(int x=0; x<width; ++x, src_index+=chan1, dest_index+=chan2)
		   if(mat1[src_index] != mat2[dest_index])
			return mat1[src_index] - mat2[dest_index];
	}else{
	   for(int y=src_index=dest_index=0; y<height; ++y)
		for(int x=0; x<width; ++x, src_index+=chan1, dest_index+=chan2)
		   if(matMask[src_index] && mat1[src_index] != mat2[dest_index])
			return mat1[src_index] - mat2[dest_index];
	}
	return 0;
   }else if(norm>0 || norm==-1){
	float mav_acc=0; int norms = norm==-1?2:norm;
	for(int y=src_index=dest_index=0; y<height; ++y){	// L(p) norm
	   for(int x=0; x<width; ++x, src_index+=chan1, dest_index+=chan2)
		if(!matMask||matMask[src_index]){
		   float tmp=fabs(mat1[src_index]-mat2[dest_index]), tmp1=tmp;
		   for(int n=1; n<norms; ++n, tmp1*=tmp);
		   mav_acc += tmp1;
		}
	   mav+=mav_acc/img_sz; mav_acc=0;
	}
	/* NOTE: p-norm definition: |sum(|x_i-y_i|^p)^(1/p) substituted by
 	 * sum/|x_i-y_i|^p\^1
	 *    |-----------| -
	 *    \    N      / p */
	return norm==-1?10*(2*log10f(255)-log10f(mav)):pow(mav,1./norm);
	// PSNR when norm=-1
   }else if(norm==0){	   // actual inf-norm
	float max=0;
	for(int y=src_index=dest_index=0; y<height; ++y)
	   for(int x=0; x<width; ++x, src_index+=chan1, dest_index+=chan2)
		if(!matMask || matMask[src_index])
		   max=std::max<float>(fabs(mat1[src_index]-mat2[dest_index]), max);
	return max;
   }
}

const float calcImgDiff(const cv::Mat* img1, const cv::Mat* img2,
	const cv::Mat* mask, const int& norm){
   if(!img1 || !img2)return 0;
   IplImage ipImg1 = *img1, ipImg2 = *img2, ipMask;
   if(mask)ipMask = *mask;	// simple conversion from cv::Mat to IplImage
   return calcImgDiff(&ipImg1, &ipImg2, &ipMask, norm);
}

// ----------Mouse Handlings----------

void MouseCallback(int event, int x, int y, int flags, void* param){
// param used to identify window id
// --Ctl+R_mouse to enter/leave ROI selection mode (wo. adding pt);
// --L_mouse to add pt to myROI (in ROI selection mode);
// --R_mouse to remove previous added pt in ROI;
   switch(event){
	case CV_EVENT_LBUTTONDOWN:
	   if(VideoCtrlStream::roi && VideoCtrlStream::roiState){
		int tmp[] = {1,x,y};
		roiMouseCallBack(tmp, *(VideoCtrlStream::roi));
	   }
	   break;
	case CV_EVENT_RBUTTONDOWN: int i_n;
	   if(VideoCtrlStream::roi){
		if(VideoCtrlStream::roiState && !(flags & CV_EVENT_FLAG_CTRLKEY)){
		   roiMouseCallBack(&(i_n=2), *(VideoCtrlStream::roi));
		} else if(flags & CV_EVENT_FLAG_CTRLKEY){	// toggles myROI mode
		   fprintf(stderr, "%s ROI selection mode: ",
			   VideoCtrlStream::roiState?"Out of":"In");
		   if(false==(VideoCtrlStream::roiState=!VideoCtrlStream::roiState)){   // TODO
			// quit selection
			roiMouseCallBack(&(i_n=0), *(VideoCtrlStream::roi));
			// pass ROI to histogram
			roiMouseCallBack(&(i_n=3), *(VideoCtrlStream::roi));
		   }
		}
	   }
	default:;
   }
   // focused window ID
   VideoCtrlStream::wid = *reinterpret_cast<int*>(param);
}


// ++++++++++++++++++++++++++++++++++++++++
// Hist

cv::Mat* Hist::mask = 0;
unsigned long Hist::histMax = 0;
int Hist::fc1=255, Hist::fc2=170;

Hist::Hist(IplImage* src1, IplImage* src2, const unsigned& nbins, const bool&
	drawable, const unsigned& gap, const cv::Mat* _mask, const cv::Size&
	scale_sz):drawable(drawable),scale(scale_sz),gapx(gap),src1(src1),src2(src2),
   nbins(nbins),fse(0),canvas(drawable?cv::Mat::zeros(scale.height,
		   2*nbins*(scale.width+gapx),CV_8UC1) : cv::Mat()){init(_mask);}

Hist::Hist(const frameSizeEq* fse, const unsigned& nbins, const bool& drawable,
	const unsigned& gap, const cv::Mat* _mask, const cv::Size& scale_sz):
   drawable(drawable),scale(scale_sz),gapx(gap),nbins(nbins),fse(fse),src1(fse?
	   const_cast<IplImage*>(fse->get(true)):0),src2(fse?
	   const_cast<IplImage*>(fse->get(false)):0),canvas(drawable?
	   cv::Mat::zeros(scale.height,2*nbins*(scale.width+gapx), CV_8UC1):
	   cv::Mat()){init(_mask);}

void Hist::init(const cv::Mat* _mask) {
   if(_mask) {
	mask = new cv::Mat(*_mask);
	*mask = _mask->clone();	   // deep-copy needed
	histMax = static_cast<unsigned long>(cv::countNonZero(*mask));
	if(histMax==0) histMax=1;
   }
   else histMax = src1->width*src1->height;
   bin1.reserve(nbins), bin2.reserve(nbins);
   bin1.assign(nbins,0), bin2.assign(nbins,0);
}

void Hist::setMask(const cv::Mat* mat) {
   if(mat) {
	delete mask; mask = new cv::Mat(*mat);
	*mask = mat->clone();
	histMax = static_cast<unsigned long>(cv::countNonZero(*mask));
	if(histMax==0) histMax=1;
   }
}

void Hist::calc(const bool first) {
   cv::Mat gray, hist;
   const int bins_p[]={static_cast<int>(nbins)}; 
   const float hranges[]={0, 255}, *ranges[]={hranges};
   cvtColor(cv::Mat(first?src1:src2), gray, CV_BGR2GRAY);
   cv::calcHist(&gray, 1, 0, mask? *mask : cv::Mat(),
	   first?hist1:hist2, 1, bins_p, ranges, true, false);
   if(first)for(int h=0; h<nbins; ++h) bin1[h]=hist1.at<float>(h);
   else	for(int h=0; h<nbins; ++h) bin2[h]=hist2.at<float>(h);
}

void Hist::dump()const
{
   std::cout<<std::setprecision(4); 
   puts("==========Hist Data:==========");
   if(src1)std::transform(bin1.begin(), bin1.end(), std::ostream_iterator<float>(std::cout," "),
		std::bind1st(std::multiplies<float>(),(0.+scale.height)/histMax*6));
   if(src2) {
	putchar('\n');
	std::transform(bin2.begin(), bin2.end(), std::ostream_iterator<float>(std::cout," "),
		std::bind2nd(std::multiplies<float>(),(0.+scale.height)/histMax*6));
   }
   putchar('\n');
}

void Hist::draw() {
   if(!drawable || !(src1||src2||fse)) return;
   unsigned long disp_bins[nbins];
   int x_inc=2*(scale.width+gapx), x_indx;
   canvas = cv::Scalar(0);
   if(fse) {
	src1 = const_cast<IplImage*>(fse->get(true));
	src2 = const_cast<IplImage*>(fse->get(false));
   }
   if(src1)	   /* Foreground colors for 2 histograms are 255/170 */
	for(int indx=x_indx=0; indx<nbins; ++indx, x_indx+=x_inc) {
	   long tmp = static_cast<unsigned long>(bin1[indx]*scale.height/histMax*6);
	   disp_bins[indx] = tmp>histMax? 0 : scale.height-tmp;
	   cv::rectangle(canvas, cv::Point(x_indx, scale.height),
		   cv::Point(x_indx+scale.width, tmp>histMax? 0:scale.height-tmp),
		   fc1, CV_FILLED);
	}
   if(src2)
	for(int indx=0, x_indx=scale.width+gapx; indx<nbins; ++indx, x_indx+=x_inc){
	   long tmp = static_cast<unsigned long>(bin2[indx]*scale.height/histMax*6);
	   disp_bins[indx] = tmp>histMax? 0 : scale.height-tmp;
	   cv::rectangle(canvas, cv::Point(x_indx, scale.height),
		   cv::Point(x_indx+scale.width, tmp>histMax? 0:scale.height-tmp),
		   fc2, CV_FILLED);
	}
   cv::imshow("Histogram", canvas);
}
// ++++++++++++++++++++++++++++++++++++++++
// simDropFrame

simDropFrame::simDropFrame(const int& size, const float& prob, const float& prob2,
	const unsigned& seed):size(size),drop_prob(prob<0||prob>=1?.5:prob),
   drop_array(0),cond_prob(prob2<=0||prob2>=1?drop_prob:prob2){
   srand(seed?seed:time(0));
   drop_array = new bool[size]; shuffle();
}

simDropFrame::simDropFrame(const simDropFrame& sd):size(sd.size),drop_prob(sd.drop_prob),
   cond_prob(sd.drop_prob){
   srand(time(0)>>1);	   // force shuffle
   drop_array = new bool[size]; shuffle();
}

void simDropFrame::shuffle(){
   bool last=false;
   for(int index=0; index<size; ++index)
	last=drop_array[index] = last? static_cast<float>(rand())/RAND_MAX < cond_prob :
	   static_cast<float>(rand())/RAND_MAX < drop_prob;
}

void simDropFrame::dump()const {
   if(drop_array) {
	printf("----------DropFrame (%d)----------\n", size);
	for(unsigned index=0; index<size; ++index)
	   if(drop_array[index])printf("%d ", index);
	puts("\n------------DropFrame-------------");
   }
}

// ++++++++++++++++++++++++++++++++++++++++
// VideoCtrlStream
// NOTE: roi object must be assigned before used
// NOTE2: roi is not used within class.

bool VideoCtrlStream::pause, VideoCtrlStream::Esc, VideoCtrlStream::stop=true,
     VideoCtrlStream::trackState, VideoCtrlStream::roiState, VideoCtrlStream::rewind;
myROI* VideoCtrlStream::roi;
int VideoCtrlStream::x, VideoCtrlStream::y, VideoCtrlStream::wid, VideoCtrlStream::w1,
    VideoCtrlStream::w2=1;
char VideoCtrlStream::fname[], VideoCtrlStream::watermark[];

VideoCtrlStream::VideoCtrlStream(IplImage* images[2], const char* wns[2],
	VideoProp* pvp[2], pfMouseCB* pm, const short& fd):img1(images[0]),img2(images[1]),
   curFrameDelay(0),frameDelay(fd),vp1(pvp[0]),vp2(pvp[1]),fs1(new fsToggle(img1)),
   fs2(new fsToggle(img2)),wn1(wns[0]),wn2(wns[1]),img10(img1),img20(img2),histIndex(0){
   if(img1->width!=img2->width || img1->height!=img2->height) {
	printf("Size=[%d %d]/[%d %d]\n", img1->width, img1->height,
		img2->width, img2->height);
	float ratio=img2->width/img1->width;
	if(img2->width/img1->width != img2->height/img1->height)
	   throw ErrMsg("VideoCtrlStream::ctor: Frame sizes unequal.\n");
   }
   if(pm){		// install MouseCB
	cv::setMouseCallback(wns[0], *pm, &w1);
	cv::setMouseCallback(wns[1], *pm, &w2);
   }
}

VideoCtrlStream::~VideoCtrlStream(){delete fs1; delete fs2;}

void VideoCtrlStream::setPause() {
   if(!stop && !(curFrameDelay=(pause=!pause)?0:frameDelay)){
	cvSetCaptureProperty(vp1->cap, CV_CAP_PROP_POS_FRAMES,
		vp1->prop.posFrame=history[--histIndex].first);
	cvSetCaptureProperty(vp2->cap, CV_CAP_PROP_POS_FRAMES, 
		vp2->prop.posFrame=history[histIndex].second);
   }
}

void VideoCtrlStream::kbdEventHandle(const keyboard& kbd) {
   img1=img10, img2=img20;	// in case fs substitutes img1/img2
   switch(kbd) {
	case NUL: return;		// no key-event intercepted
	case SPACE:
	    setPause(); break;
	case NextFrame:if(rewind)rewind=!(++histIndex); break;
	case PrevFrame:
	   rewind=true;
	   cvSetCaptureProperty(vp1->cap, CV_CAP_PROP_POS_FRAMES, vp1->prop.posFrame
		   =history[--histIndex>0?histIndex:histIndex=0].first);
	   cvSetCaptureProperty(vp2->cap, CV_CAP_PROP_POS_FRAMES, vp2->prop.posFrame
		   =history[histIndex].second);
	   break;
	case Quit:
	case ESC: setEsc(); break;
	case Startstop:
	   if(stop=!stop){
		curFrameDelay = histIndex = 0;
		cvSetCaptureProperty(vp1->cap, CV_CAP_PROP_POS_FRAMES, vp1->prop.posFrame
			=vp1->prop.posRatio=0);
		cvSetCaptureProperty(vp2->cap, CV_CAP_PROP_POS_FRAMES, vp2->prop.posFrame
			=vp2->prop.posRatio=0);
	   }else if(!pause)curFrameDelay=frameDelay;
	   break;
	case ToggleFs:	// call getImg after this in upper-level
	   (wid==w1?fs1:fs2)->update();
	   wid==w1 ? img1=fs1->toggle(0) : img2=fs2->toggle(0);
	   break;
	case Save:
	   if(pause){	// saveable only when paused
		fputs("File name (and extension) to save: ", stderr);
		memset(fname,0,32); memset(watermark,0,64);
		int len;
		if(!fgets(fname, 32, stdin) || 1==(len=strlen(fname))){
		   fputs("\nfgets failed or no input.\n",stderr); break;
		}
		printf("Strlen=%d\n", len);
		if(fname[len-1]=='\n')fname[len-1]=0;
		if(FILE* fp=fopen(fname, "r")){
		   fclose(fp); fprintf(stderr, "\"%s\" exists. Overwritten.\n", fname);
		}
		cv::Mat copy(wid==w1?img10:img20, true);
		// format water mark: frame number min:sec
		VideoProp* vpp = wid==w1?vp1:vp2; float& ms=vpp->prop.posMsec;
		sprintf(watermark,"%d %02d:%02d", vpp->prop.posFrame, static_cast<int>
			(ms/6e4), static_cast<int>(ms*1e-3));
		waterMark(copy, watermark, cv::Point(20,20));
		try{
		   cv::imwrite(fname, copy);
		}catch(const cv::Exception& ex){
		   fputs(ex.what(), stderr);
		   fputs("Most likely file type not supported.\n",stderr);
		}
		fprintf(stderr, "\"%s\" saved.\n", fname);
	   }
   }
   fputs("\b\b\b\b\b\b\b\b\b\b\b\b\b",stdout);	   // messy console control...
   fflush(stdout);
   if((kbd==PrevFrame || kbd==NextFrame) && !getUpdate())
	printf("%d %d   ", history[histIndex].first, history[histIndex].second);
   fflush(stdout);
   if(kbd!=PrevFrame)rewind=false;
}

inline void VideoCtrlStream::waterMark(cv::Mat& img, const char* title, const
	cv::Point& start){
   cv::Mat text(img.rows, img.cols, img.type(), cv::Scalar(0));
   cv::putText(text, title, start, 2, 1, cv::Scalar(30, 60, 30), 2);
   cv::add(text,img,img);
}

// ++++++++++++++++++++++++++++++++++++++++

IplImage *frameBuffer::mask=0, *frameBuffer::copy=0;

frameBuffer::frameBuffer(const size_t& sz, IplImage* pfrm[2]):size(sz),
   image1(pfrm[0]),image2(pfrm[1]),rgbCvt(image1->nChannels>1),curSize1(1),
   curSize2(1),fse(0){init();}	// assumes pfrms have same dimension

frameBuffer::frameBuffer(const size_t& sz, frameSizeEq *fse):size(sz),
   image1(fse->get(true)),image2(fse->get(false)),rgbCvt(image1->nChannels>1),
   curSize1(1),curSize2(1),fse(fse){init();}

void frameBuffer::init() {
   if(!(image1&&image2))throw ErrMsg("frameBuffer::ctor: IplImage ptrs NULL.\n");
   copy = cvCreateImage(cvGetSize(image1), image1->depth, CV_8UC1);
   if(rgbCvt) {
	cvCvtColor(image1, copy, CV_BGR2GRAY); buffer1.push_back(copy);
	copy = cvCreateImage(cvGetSize(image2), image2->depth, CV_8UC1);
	cvCvtColor(image2, copy, CV_BGR2GRAY); buffer2.push_back(copy);
   }else{
	memcpy(copy->imageData, image1->imageData, image1->imageSize);
	obuffer1.push_back(copy); // also make a copy of unconverted
	copy = cvCreateImage(cvGetSize(image2), image2->depth, CV_8UC1);
	memcpy(copy->imageData, image2->imageData, image2->imageSize);
	obuffer2.push_back(copy);
   }
}

void frameBuffer::setMask(const IplImage* _mask) {
   if(mask && _mask)cvReleaseImage(&mask);
   mask = cvCreateImage(cvGetSize(_mask),_mask->depth,CV_8UC1);
   if(_mask->nChannels==1)memcpy(mask->imageData,_mask->imageData,_mask->imageSize);
   else cvCvtColor(_mask,mask,CV_BGR2GRAY);
}

void frameBuffer::update(const bool& first) {
   const IplImage* image;
   if(fse && !(image=fse->get(first)))return;
   li beg=(first?buffer1:buffer2).begin();
   int& curSize=first?curSize1:curSize2;
   listImage& buffer=first?buffer1:buffer2;
   if(++curSize>size && (curSize=size)){
	if(rgbCvt)cvCvtColor(image, *buffer.begin(), CV_BGR2GRAY);
	else memcpy((*buffer.begin())->imageData, image->imageData, image->imageSize);
	std::rotate(buffer.begin(), ++beg, buffer.end());
   }else{
	copy = cvCreateImage(cvGetSize(image), image->depth, CV_8UC1);
	if(rgbCvt)cvCvtColor(image, copy, CV_BGR2GRAY);
	else memcpy(image->imageData, copy->imageData, image->imageSize);
	buffer.push_back(copy);
   }
   return;
}

const IplImage* frameBuffer::get(const int& indx, const bool& first, const
	bool& singleChan)const {
   if(indx<0 || first && indx>buffer1.size() || !first && indx>buffer2.size())
	return 0;
   listImage::const_reverse_iterator iter;
   if(first) {
	if(singleChan || !rgbCvt) std::advance(iter=buffer1.rbegin(),indx);
	else std::advance(iter=obuffer1.rbegin(), indx);
   }else if(singleChan || !rgbCvt) std::advance(iter=buffer2.rbegin(), indx);
   else std::advance(iter=obuffer2.rbegin(), indx);
   return *iter;
}

