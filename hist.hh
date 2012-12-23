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
#include <list>
#include <memory>
#include <cv.h>
#include <highgui.h>
#include <boost/tuple/tuple.hpp>
#include "sketch.hh"

/**
 * @relates Hist
 * @brief Compare similarites of two vectors. Used by
 * histogram comparison and frame-registration. Note that
 * Chi-square and Intersection differences are sensitive to
 * scaling of one vector, while the other two not.
*/
typedef enum{Default=-1, Correlation=CV_COMP_CORREL, Chi_square=CV_COMP_CHISQR,
   Intersection=CV_COMP_INTERSECT, Bhattacharyya=CV_COMP_BHATTACHARYYA} Criterion;
/**
 * @relates Hist
 * @brief sets an IplImage object with homogeneous
 * brightness
 * @param IplImage image to be set
 * @param brightness brightness (0-255) to be set. Defaults
 * to 0.
 */
void setIplImage(IplImage*, const char& brightness=0);

/**
* @relates Hist
* @name calculates normed difference between two images
* @{
* @param IplImage 1st image
* @param IplImage 2nd image. These two images must be of
* same width/height/channel
* @param mask if>0, gives the mask on which the norm is
* calculated.
* @param norm If norm<-1, check for exact duplicate;
* norm=-1 gives the PSNR of RMSE; 0 gives max|f1(x,y)-f2(x,y)|, etc.
*
* @return difference between two images
*/
const float calcImgDiff(const IplImage*, const IplImage*, const IplImage* mask=0,
	const int&norm =1); 
const float calcImgDiff(const cv::Mat*, const cv::Mat*, const cv::Mat* =0,
	const int& =1); 
/**  @} */

/**
 * @relates VideoCtrlStream
 * @brief Used as parameter to member func of VideoCtrlStream.
 * Ctl+R_mouse to enter/leave ROI selection mode (wo. adding pt);
 * L_mouse to add current cursor point to myROI object (defined in sketch.hh);
 * R_mouse to remove previous added pt in ROI;
 * @param event CV mouse events
 * @param x	cursor position
 * @param y
 * @param flags Modifier.
 * @param param Not used.
 */
void MouseCallback(int event, int x, int y, int flags, void* param);

/** @brief Purpose: takes a colored image, convert to Gray scale,
 * and calculates the gray-scale histogram.
 */
class frameSizeEq;

/**
 * @brief Calculates (and draws, if desired) gray-scale histogram of *A PAIR OF * current frames.
 */
class Hist {
public:
/**
 * @name Ctor/Dtors
 * @{
 * @brief 
 * @param src1 first frame to calc histogram
 * @param src2	second frame. Must be same size
 * In the second form, accepts fse object ptr (defined in dynan.hh) in place of src1/src2
 * @param nbins # of uniform bins of historgram
 * @param drawable whether to show the histogram in a new
 * window?
 * @param gap width between adjacent bars in histogram
 * @param _mask Image mask used for calculating
 * histogram. Must be either NULL (whole image used) or
 * of equal size as src1/src2
 * @param scale_sz width = bar_width; height=canvas.height
 */
   explicit Hist(IplImage* src1, IplImage* src2, const unsigned& nbins =30, 
	   const bool& drawable=false, const unsigned& gap=3, const cv::Mat*
	   _mask=0, const cv::Size& scale_sz=cv::Size(3,140));
   explicit Hist(const frameSizeEq* fse, const unsigned& nbins=30, const bool&
	   drawable =true, const unsigned& gap=3, const cv::Mat* _mask=0,
	   const cv::Size& scale_sz=cv::Size(3,140));
   ~Hist();
/**  @} */

/**
 * @name update
 * @brief notify that frame pts are updated; histogram
 * need recalculation. May re-install frame pts.
 * @{ */
   void update();
   void update(IplImage*, IplImage*);
/**  @} */

/**
 * @brief Get histogram vector
 * @param first first frame?
 */
   const std::vector<float>& get(const bool& first)const;

/**
 * @brief Updates mask used. Performs a deep copy
 * @param mask new image mask to install
 */
   static void setMask(const cv::Mat* mask);

/** @brief Prints two histogram vectors. */
   void dump()const;
/** @brief sets foreground colors of histogram bars */
   static void setFC(const int& f1, const int& f2){fc1=f1, fc2=f2;}

/**
 * @name read/write
 * @brief Not used
 * @{ */
   const bool read(std::fstream&);
   const bool write(std::fstream&)const;
/**  @} */
protected:
   const unsigned long nbins, gapx;
   const frameSizeEq* fse;
   IplImage *src1, *src2;
   const cv::Size scale;
   static int fc1, fc2;// foreground colors for 2 drawable histograms
   const bool drawable;
   std::vector<float> bin1, bin2;
   cv::Mat canvas, hist1, hist2;
   static unsigned long histMax;
   static cv::Mat *mask;
   void init(const cv::Mat*);
   void calc(const bool=true);
   void draw();
};

/**
 * @brief Simulates frame drop according to simple
 * probablistic model
 */
class simDropFrame {
public:
/**
 * @brief Initializes probablitic model
 * @param sz length of array
 * @param prob i.i.d. probability that each frame gets
 * droped
 * @param prob2 Probability of successive frame drop if
 * previous frame gets dropped. Equals prob if prob2<0 or
 * prob2>1
 * @param seed random generator seed. Set to 0 to seed by
 * system time
 */
   simDropFrame(const int& sz, const float& prob, const float&
	   prob2=0, const unsigned& seed=0);
   simDropFrame(const simDropFrame&);
   ~simDropFrame(){delete[] drop_array;}
   const bool drop();
/**
 * @brief Updates probablistic model
 * @param p1 i.i.d probability of each frame
 * @param p2 successive dropping probability
 */
   void dropProb(const float& p1, const float& p2);
/**
 * @brief returns the drop array; re-initialize if desired
*/
   const bool* dropArray()const{return drop_array;}
   void shuffle();
   void dump()const;
protected:
   int size;
   float drop_prob, cond_prob;
   bool *drop_array;
};

class myROI;
class VideoProp;
class fsToggle;

/**
 * @brief Interface with kbd for simple video playing
 * control
 */
class VideoCtrlStream {
public:
   typedef enum {
	NUL=-1, ESC=27, SPACE=32, ToggleFs='f', NextFrame='l', PrevFrame='h',
	Startstop='s', Save='S', Quit='q'
   }keyboard;
   typedef void(*pfMouseCB)(int, int, int, int, void*);
   static bool roiState;
/**
* @brief Sets video call-backs and kbd play-controls
* @param images[2] image pairs
* @param wns[2]	window names
* @param pvp[2]	video property array
* @param paired	accept a pair of video captures?
* @param pm	   mouse call-back
* @param fd	   frame delay in playing (in addition to processing time)
*/
   VideoCtrlStream(IplImage* images[2], const char* wns[2], VideoProp* pvp[2],
	   pfMouseCB* pm, const short& fd=50);
   ~VideoCtrlStream();
/**
* @name getXX
* @brief retrives control states/variables
* @{ */
   const bool& getEsc()const{return Esc;}
   const short& getFrameDelay()const{return curFrameDelay;}
   const short& getConstFrameDelay()const{return frameDelay;}
   const bool getUpdate()const{return histIndex>=history.size();}
/**  @} */
/**
* @brief Keyboard interface
* @param keyboard key events
*/
   void kbdEventHandle(const keyboard&);
   const IplImage* getImg(const bool& first)const{return first?img1:img2;}
   void update(const int& d1, const int& d2){  // advance 1 frame
	if(++histIndex>history.size())history.push_back(std::make_pair(d1,d2));
   }
   static void setROI(myROI* mr);
protected:
   static bool pause, Esc, trackState, stop, rewind;
   static int w1, w2;	// track pixel brightness with right mouse down;
   static int x, y, wid;   // and focused window id
   static myROI* roi;
   static char fname[64], watermark[64];
   friend void MouseCallback(int, int, int, int, void*);
   const char* wn1, *wn2;
   short frameDelay, curFrameDelay, histIndex;
   std::vector<std::pair<int,int> > history;
   const IplImage *img1, *img2, *img10, *img20;
   fsToggle *fs1, *fs2;
   VideoProp *vp1, *vp2;
   void setPause();
   void setEsc(){Esc=true;}
   static void waterMark(cv::Mat&, const char*, const cv::Point&);
};

/**
* @brief buffers adjacent frames (of identical dimension) for two sets of videos.
* Internally keeps two copies per frame: one gray scale
* and the other with possibly depth=3 (BGR).
*/
class frameBuffer {
public:
/**
* @name ctors/dtors
* @brief converts (from BGR) to gray-scale and stores in a deque. Deep copy needed.
* @param sz # frames to buffer
* @param pfrms frames (or fse: frame size equalizer object)
* @{ */
   frameBuffer(const size_t& sz, IplImage* pfrms[2]);
   frameBuffer(const size_t& sz, frameSizeEq* fse);
   ~frameBuffer();
/**  @} */
/**
* @name update
* @brief notify frames update (call fse.get() to FORCE
* object update); then updates frame buffers
* @{ */
   void update(const bool& first);
   void update(){update(true);update(false);}
/**  @} */
/**
* @brief queries #(current-indx) frame
* @param indx history index entry
* @param first first/second frame?
* @param singleChan image depth
*/
   const IplImage* get(const int& indx, const bool& first, const
	   bool& singleChan=true)const;
/**
* @name setMask getMask
* @brief associated mask
* @{ */
   static void setMask(const IplImage*);
   static const IplImage* getMask(){return mask;}
/**  @} */
private:
   template<typename T>
	class IplImageAllocator{
	public:
	   typedef size_t size_type;
	   typedef T value_type;
	   typedef value_type* pointer;
	   typedef const pointer const_pointer;
	   typedef value_type& reference;
	   typedef const T& const_reference;
	   size_type max_size(){return 0x100;}
	   pointer allocate(size_type n){
		return reinterpret_cast<pointer>(malloc(n*sizeof(T)));
	   }
	   void deallocate(pointer p, size_type n){free(p);}
	   void construct(pointer p, const_reference v){*p=v;}
	   void destroy(pointer p){
		IplImage** pp=reinterpret_cast<IplImage**>(p);
		cvReleaseImage(pp+2);	// ?
	   }
	   template<typename U> struct rebind{
		typedef IplImageAllocator<U> other;
	   };
	};
protected:
   const size_t size;
   const IplImage *image1, *image2;
   static IplImage *mask, *copy;
   const bool rgbCvt;

   typedef std::list<IplImage*, IplImageAllocator<IplImage*>
	>listImage;
   typedef listImage::iterator li;
   typedef listImage::reverse_iterator rli;
   listImage buffer1, buffer2, obuffer1, obuffer2;
   int curSize1, curSize2;
   frameSizeEq* fse;
   void init();
};
