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
#include "sketch.hh"
#include "cv_templates.hh"
#pragma once

extern char msg[256];
extern bool verbose;
/**
* @brief Interpolate frame(s) among a set of frames. Used
* for frame recovery when frame-drop is detected.
*/
class dynan{	   // interpolate gray-scale images
public:
   typedef enum{
	cDefault,	   // defaults to Pix
	Pix,
	Lucas_Kanades,
	Pyr_LK,
	Horn_Schunck,
	Block_Matching
   }InterpMetric;

   typedef enum{
	dDefault,	   // defaults to Linear
	ZeroHold,
	Linear,
	Quad,
	ConstraintQuad,
   }InterpMethod;

   dynan(const IplImage**, IplImage**, const unsigned&, const unsigned&,
	   const unsigned&, const InterpMetric& =Pix, const InterpMethod& =Linear,
	   const short& =15)throw(ErrMsg);
   void setIntMethod(const InterpMetric&, const InterpMethod&);
   void interpolate();
protected:
   std::vector<const IplImage*> vSrcImg;
   InterpMethod ipd;
   InterpMetric ipc;
   const int apertureSz, width, height, imageSize;
   const unsigned iLoc, iFrameNum, oFrameNum;
   IplImage** pDestImg;

   template<typename T>void interp(const T*, T*)const;
   void regInterp(const IplImage*const, const IplImage*const)const;

   void medianFill(const IplImage* const, unsigned char*, const int=1)const;
	// qsort fptr by medianFill
   template<typename T>static int typeCmp(const void*, const void*);
};

/**
 * @brief Integrates/manages video properties */
class VideoProp {
protected:
   IplImage* ip;
public:
/**
 * @brief contents of video information book-keeping */
   struct Props {
	cv::Size size;
	int fps, fcount, codec, posFrame, depth, chan;
	float posMsec, posRatio;
	// prevents partial init state
	Props(const cv::Size&, const int&, const int&, const int&,
		const float&, const float&, const int&, const int&,
		const int&);
   }prop;
   CvCapture* cap;

   explicit VideoProp(CvCapture*);
/**
* @brief refreshes video information
* @param lazy: lazy update, assume that cap has not changed.
* Defaults true
*/
   void update(const bool& lazy=true)throw(ErrMsg);
};

void swap(VideoProp&, VideoProp&);
/**
* @brief Frame size equalizer. Manages a pair of images of
* same aspect ratio, in addition to convert to gray-scale.
*/
class frameSizeEq {
public:
/**
* @name ctor/dtor
* @brief initializes with a pair of frames. Shallow copy
* when resizing not needed.
* @{ */
   explicit frameSizeEq(IplImage* pfrm[2]);
   ~frameSizeEq(){
	if(img1Created)cvReleaseImage(&oframe1);
	if(img2Created)cvReleaseImage(&oframe2);
   }
/**  @} */
/**
* @name update
* @brief refreshes internally kept frames
* @{ */
   void update(const bool& first);  // invoked by class frameUpdater
   void update() {update(true);update(false);}
/**  @} */
   const IplImage* get(const bool& first)const {return first?oframe1:oframe2;}
/** @brief ratio of widths of two frames */
   const float& getR()const {return ratio;}
protected:
   // alternatives: CV_INTER_NN, CV_INTER_LINEAR, CV_INTER_CUBIC,
   // CV_INTER_AREA, CV_INTER_LANCZOS4
   const static int InterpMethod=CV_INTER_CUBIC;  
   IplImage *iframe1, *iframe2, *oframe1, *oframe2;
   const float ratio;
   const bool img1Cvt, img2Cvt, img1Created, img2Created;
   frameSizeEq(const frameSizeEq&);
   const static bool cvtGrayFast=true;	// replace B-element to gray level,
   static void cvt2Gray(IplImage*);
};

class myROI;

/**
* @brief Find best-match frame of video #2 for a given frame
* of video #1 within a search range. Affected by static
* member function that sets ROI (defined in sketch.hh)
*/
class frameRegister {
public:
// defaults to FrameDiff
   typedef enum{DefaultDiff, FrameDiff, HistDiff, DtDiff}DiffMethod;
/**
* @brief Initializes videos to manage, search range,
* frame-wise comparison criterion, etc.
* @param cap_str[2] capture name from which to
* create/maintain local copies of captures
* @param prange searches maximally [-prange, +prange] around
* specified frame of target video
* @param me frame-wise matching criterion. Given in DiffMethod enum.
* @param tr transform method if DiffMethod==DtDiff. Required
* for local object construction.
* @param parm Criterion to compare two vectors.
* @param nbin used when DiffMethod is "HistDiff"
* @param drop If not NULL, used when frame-drop simulation
* is in effect
* @param normVec normalize histogram/DFT differences (with
* same mean value) before calculating differences?
* @param inc Impose constraint that registered number should
* be strictly increasing?
* @param norm Norm used to calculate pixel-wise difference
* between two frames.
* @throw ErrMsg panicks when aspect ratio unequal, or failed
* to create local captures
*/
   frameRegister(const char* cap_str[2], const int& prange, const DiffMethod&
	   me, const VideoDFT::Transform& tr, const Criterion& parm, const int&
	   nbin, const bool* drop, const bool normVec[2], const bool& inc=true,
	   const int& norm=1)throw(ErrMsg);
   ~frameRegister();
/**
* @brief Performs frame registration for the pos-th frame.
* Search in the second video capture from [pos-range, pos+range]
* for a best match
* @param pos frame position in source video
* @param prev_reg registered frame position of previous
* frame. Current frame size must be larger than this number.
* @param me matching criterion
* @return frame position in target video
* @throw ErrMsg when prev searched result is too large.
* Maybe increase search range?
*/
   const int reg(const int& pos, const int& prev_reg, const DiffMethod&
	   me=DefaultDiff)throw(ErrMsg);
/**
* @brief Prints frame-wise difference in the previous
* matching process
*/
   void dump()const;
/**
* @brief Installs ROI based on which to perform frame-wise match
* @param myROI  ROI object
*/
   static void setROI(myROI* _roi){roi=_roi;}
   static void setHsb(const float& f){
	if(f>0 && f<1)heuristicSearchBound=f;
   }
protected:
   CvCapture *src_cap, *dest_cap;
   IplImage *frame1, *frame2;
   frameSizeEq* fse;
   Hist* hist;
   VideoDFT *dft1, *dft2;
   unsigned nfr1, nfr2, regpos, count;
   int nbins, range, prev_srcPos, *diffPos;
   float *diffVal;
   const bool* dropArray, norm1, norm2, inc;
   static myROI* roi;
   const int diffNorm;
   DiffMethod Method;
   const VideoDFT::Transform tr;
   const Criterion diffParam;
   frameRegister(const frameRegister&);
   const float calcDiff()throw(ErrMsg);
   static float heuristicSearchBound;
};

/** @brief Resource manager (broker) for video pairs, so
 * that top-level object (Updater) do not need to worry
 * about underlying details */
class frameUpdater{
public:
/**
* @brief 
* @param pfrm[2] Frame pointers
* @param fse Frame size equalizer. REQUIRES that pfrm param
* is identical to frame pointers passed to frameSizeEq ctor.
* @param fb for delegation
* @param drop for delegation. Only affects second capture.
* @param pvp[2] for delegation
* @param ma_vals[3] dynamics (frame difference of adjacent frames) of capture 1/2
* and paired-difference (same frame id of two captures)
* @param norm n-Norm difference of image difference
*/
   frameUpdater(IplImage* pfrm[2], frameSizeEq& fse, frameBuffer* fb, simDropFrame*
	   drop, VideoProp* pvp[2], ARMA_Array<float>* ma_vals[3], const unsigned&
	   norm=2);
/**
* @brief Updates fse, fb, drop-array; skips any frame
* replica (same frame content in adjacent frames) and
* dropped frames, then updates frame differences
*/
   void update();
/**
* @brief Prints up to current frame which frames are dropped?
*/
   void dump()const;
/**
* @brief retrieve currently detected frame replication
* number of two videos or simDrop dropped 2nd video frames
* @param id 0: simDrop of 2nd video frames dropped so far;
* 1: replicated frames of 1st video; 2: #replica of 2nd.
*/
   const int& getNdrop(const int& id){return id==0?ndrop2Sim:(id==1?ndrop1:ndrop2);}
   const bool& eof()const{return capEOF;}
   static bool rmAdjEq;
protected:
   const IplImage *frame1, *frame2, *mask;
   frameSizeEq& fse;
   VideoProp *vp1, *vp2;
   frameBuffer *fb;
   ARMA_Array<float>** ma_val;
   int ndrop1, ndrop2, ndrop2Sim, &fc1, &fc2;
   bool capEOF;
   const unsigned normDiff;
   std::deque<bool> drop1, drop2, simdrop;
   frameUpdater(const frameUpdater&);
};

/**
* @brief printes information about video quality evaluation
*/
class Logger{
public:
   Logger(const char* log, VideoProp*vp[2], ARMA_Array<float>* ma[3], Hist& hist,
	   VideoDFT* vd[2], const bool normVec[2])throw(ErrMsg);
   Logger(const Logger&)=delete;
   Logger& operator=(const Logger&)=delete;
   void update();
   /**
    * @brief retrieves history data of dynamics(ID=0-1), RMSE (2)
    * histogram difference (ID=3-6), DFT difference (ID=7-9, no Chi-square).
    * @return retrieved vector stored so far
    */
   const std::vector<float>& get(const int& id)const{
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
protected:
   bool normVec[2];
   struct File{
	File(const char* nm)throw(ErrMsg){
	   if(!nm)throw ErrMsg("Logger::File::ctor: log file set NULL.");
	   if(!strcmp("stdout",nm))fd=stdout;
	   else if(!(fd=fopen(nm,"w"))){
		sprintf(msg,"Logger:File::ctor: Cannot open file \"%s\" for writing.", nm);
		throw ErrMsg(msg);
	   }
	}
	~File(){fclose(fd);}
	FILE* fd;
   }file;
   friend void summaryPlot(const int&, const Logger&, const bool);
   VideoProp *vp1, *vp2;
   VideoDFT *dft1, *dft2;
   Hist& hist;
   ARMA_Array<float> *dyn1, *dyn2, *diff;
   arrayDiff<float> histDiff;
   std::unique_ptr<arrayDiff<double> > dftDiff;
   std::vector<float> vdyn1, vdyn2, vdiff, vhist_diff1, vhist_diff2, vhist_diff3,
	vhist_diff4, vdft_diff1,  vdft_diff3,  vdft_diff4;
   /**
    * @name Logger file output buffer size, in unit of record/line
    * @{ */
   const static size_t bufRecCap=16;
   size_t bufRec;
   /**  @} */
};

template<typename T>class drawBars;

/**
* @brief Top-level video control/processing unit. Adds
* management of histogram, video property and DFT, and
* profiles program performance
*/
class Updater{	// wraps obj. updates and time consumptions
public:
/**
* @name ctor/dtor
* @{
* @param cnt current frame positional index
* @param _vp delegates video property
* @param _vd delegates DFT
* @param _roi delegates ROI
* @param _hist delegates histogram
* @param _fu wraps lower-level manager
* @param _log Logger
* @param _showBar display DFT distribution?
* @param _showDft display DFT magnitude?
*/
   Updater(const short& cnt, VideoProp** _vp, VideoDFT** _vd, myROI* _roi, Hist*
	   _hist, frameUpdater* _fu, Logger& _log, const bool& _showBar=false,
	   const bool& _showDft=false);
   ~Updater(){if(dftBar)delete dftBar;}
/**  @} */
/**
* @brief updates video property, DFT, histogram,
* frameUpdater, etc. if not locked (video paused)
* @param locked Is video playing paused?
* @param log Append new logging data? controlled by
* VideoCtrlStream prev/nextFrame
*/
   void update(const bool& locked, const bool& log=true);
/**
* @brief Gets the time elapsed (in ms) between adjacent
* video frame processing
* @return time in ms elapsed from last frame to current
* frame. In case of paused video, the time of pause is *NOT*
* subtracted.
*/
   const long tm()const{
	return static_cast<long>(1000*(curTick-prevTick)/tickFreq);
   }
/**  @} */
/**
* @brief Prints information of DFT, ROI, bufferedArray and
* histogram if they are initialized.
*/
   void dump()const;
protected:
   static double	tickFreq;
   static bool firstCall;
   long curTick, prevTick;
   const short frameNum;
   const bool  vp_null, vd_null,showBar, showDft;
   std::vector<VideoProp*> vp_vec;
   std::vector<VideoDFT*> vd_vec;
   myROI* roi;
   Hist* hist;
   frameUpdater* fu;
   Logger& logs;
   drawBars<std::vector<double> >* dftBar;
};

/**
* @brief Prepare video by prepending a frame of equal
* brightness and save it to a video clip; register the
* transmitted-recorded video by extracting matched prepended
* frame and register for frame width/height, and save
* clipped/cropped video.
*/
class VideoRegister{
public:
/**
* @brief ctor
* @param vp video capture and essential properties for frame
* repositioning
* @param fname file name of the original video (un-appended
* or un-registered)
* @param bright brighteness of the frame to append or to
* match.
* @throw ErrMsg fname string NULL
*/
   VideoRegister(VideoProp& vp, char*const fname, const char& bright=0xff)throw(ErrMsg);
   ~VideoRegister(){delete[] appName;}
/**
* @name prepender/reg-saver
* @brief prepends/reg-saves given VideoProp to a new file.
* No checking for file name clashes performed.
* @param suf suffix appended to base name.
* @param np # consecutive prepended frames
* @param noiseType: 0(none), 1 (salt-pepper), 2(normal-distribution),
* 3(double-image)
* @param df frame drop sequence
* @param dropMethod 1: drop only; 2: freeze dropped frames; 3:
* freeze portion of dropped frames
* @throw ErrMsg suf string NULL
* @throw cv::Exception video writer failed (unsupported
* codec), or when writing to video
* @{ */
   void prepend(char*const suf, const int& np, const int& noiseType, const
	   float* noiseLevel, const simDropFrame* df=0, const int& dropMethod=0)
	throw(ErrMsg,cv::Exception);
/**
* @param sz hint size from reference video
*/
   void save(char*const suf, const cv::Size& sz)throw(ErrMsg,cv::Exception);
/**  @} */
   void setTol(const int tols[3], const bool& bc=false){
	tolMean=tols[0]; tolSd=tols[1]; minFrameVideo=tols[2];
	badStop=bc;
   }
   friend class loadConf;	// allow for modification of Codec
protected:
   VideoProp& vp;
   bool badStop;
   char *const fname, *suf, *appName;
   unsigned char bright;
   int startFrame, endFrame, start_x, end_x, start_y, end_y;
/**
* @brief configurable tolerances using conf file. Matched
* frame is not considered if it appears 'minFrameVideo'
* framesearlier than previous matched frame
*/
   int tolMean, tolSd, minFrameVideo;
/* Available codecs: PIM1 -- MPEG-1; MJPG -- motion-jpeg;
 * MP42 -- MPEG4.2; DIV3 -- MPEG4.3; U263 -- H263; I263 -- H263I;
 * FLV1 -- FLV1; IYUV -- default (large file size) */
   static char Codec[5];
   const static float pi_deg;
/**
* @brief core function for registering inserted frame(s) and
* cropping positions. Register at most 2 frame indices from
* specified frame to be the 1st frame, and register for
* starting/end pixels of the video USING THIS FRAME.
* @param dimHint what width/height are the cropped video
* supposed to be? If the found size is larger, then *FORCE*
* (broaden) dimensions to be the central portions; else the
* found size being smaller indicates over-cropping in
* recording video.
* @return working status:
* 0 found 2 matched frames AND the cropped dimension = dimHint;
* 1 hint width > vp width;
* 2 hint width != found width by column-wise scanning;
* 3 hint height> vp height;
* 4 hint width > vp width && hint height > vp height;
* 5 hint width != found width && hint height != found height
* 6 hint height!= found height
* 7 hint height!= found height && hint width > vp width
* 8 hint height!= found height && hint width != found width
* +9 if only one matched frame is found.
* @throw ErrMsg if no matched frame is found
*/
   int reg(const cv::Size& dimHint=cv::Size(0,0))throw(ErrMsg);
   void strAppend();
/** @brief workaround for cvCopy with rectangular mask (doesn't seem to
 * work?) to crop using start_x, end_x, start_y, end_y.
* @param dest Allocated resource to copy cropped portion
* @param src
*/
   void crop(IplImage* dest, IplImage*const src);
/**
* @name Additive adding noise to an image
* @{
* @brief Adds salt-pepper noise to current image with given
* SNR level
* @param src Source image. Noise are added to source image
* @param snr SNR level (SNR=10*log(Pr(noise))) that sets
* probability whether a pixel is to be noised
*/
   static void saltPepper(IplImage* src, const float& snr);
/**
* @brief Adds zero-mean normally-distributed noise to current
* image with given standard deviation
* @param sigma standard deviation of normal noise
*/
   static void normalNoise(IplImage* src, const float& sigma);
/**  @} */
/**
* @brief Splits source image of dimension WxH into four
* rectanglar regions: r1: [0, 0]~[x, y]; r2: [x+1, 0]~[W,
* y]; r3: [0, y+1]~[x, y+1]; r4: [x+1, y+1]~[W, H]; then
* reassembles from [r1, r2; r3, r4] to [r4, r3; r1, r2].
* Offsets x, y are determined using polar representation:
* x=dist*cosine(deg), y=dist*sin(deg). Modular if x/y.
* Then the `moved' image is weight-added to original image
* greater than original dimension.
* @param src Source image
* @param param: [dist deg weight]:
* dist -- Distance in pixels to "rotate"-"reassemble"
* deg -- deg Phase in degree
* weight -- additive weight of shifted image added to
* original image
* @throw ErrMsg invalid arguments
*/
   static void solidMoveAdd(IplImage* src, const float* param);
// phase in degree
   static void moveRectRegion(const IplImage*, IplImage*, const cv::Size&,
	   const cv::Point&, const cv::Point&)throw(ErrMsg);
// mv subimage to new location
   friend class createAnimation;
};
