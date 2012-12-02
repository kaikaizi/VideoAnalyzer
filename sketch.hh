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
#include <cv.h>
#include <highgui.h>
#include <deque>
#include <map>
#include "hist.hh"

class myROI;
/**
* @brief Add/delete a point to a myROI object according to
* mouse event
* @relates myROI @relates VideoCtrlStream
* @param params 0: quit selection mode;
* 1: append a point (x = params[1], y = params [2] is location);
* 2: remove a point; 3: pass ROI to Hist and clear;
* @param roi myROI object ref
*/
void roiMouseCallBack(const int* params, myROI& roi);

/**
* @brief Simple exception class, encapsulating text message.
*/
class ErrMsg{
public:
   ErrMsg(const char*, const int& =0);
   ErrMsg(const std::string&, const int& =0);
   const char* what()const noexcept{return msg;}
   const int errnos()const noexcept{return Errno;}
private:
   char msg[256];
   const int Errno;
};

/**
* @brief pure virtual base class.
* bufferedArray, myROI and VideoDFT are its derivatives.
*/
class readWrite {
public:
/**
* @name Ctor/dtor
* @{
* @brief Ctor: create a local copy of image when specified,
* on which to draw stuffs
* @param mat Source image; make a local copy if specified
* @param _objID Class ID, used to identify derived classes
* @param lc Line color (brightness) for dash/solid line
* drawing
* @param alloc should a local copy of "mat" be made?
* @throw ErrMsg issues error when source image NULL or
* cannot make local copy when specified.
*/
   readWrite(cv::Mat* mat, const char _objID, const cv::Scalar&
	   lc=170, const bool& alloc=false)throw(ErrMsg);
   readWrite(IplImage* ipl, const char _objID, const cv::Scalar&
	   lc=170, const bool& alloc=false)throw(ErrMsg);
   ~readWrite();
/**  @} */
/**
* @name 
* @{ */
/**
* @brief File interface for derived object details
* @param std::fstream
*/
   virtual void write(std::fstream*)=0;
   virtual void read(std::fstream*)=0;
/**  @} */
/**
* @brief Force update processing
*/
   virtual void update()=0;
/**
* @brief Print diagnostic information
*/
   virtual void dump()const=0;
protected:
   cv::Mat* region;
   const cv::Scalar lineColor;
   static const char obj_delim, field_delim, *err_msg_io, *err_msg_format,
		    *err_msg_unknown;
   const char objID;
   inline const bool ioChecker(std::fstream*)const;	// finalized
   inline const bool seekp(std::fstream*, const char&)const;	// ofstream
   inline const bool seekg(std::fstream*, const char&)const;	// ifstream
   template<typename T>			// T can be vector/deque/list/etc.
	void writeArray(std::fstream*, const typename T::iterator&,
		const typename T::iterator&);
   template<typename T1, typename T2=float>
	void readArray(std::fstream*, T1&, const typename T1::iterator&,
		const typename T1::iterator&)throw(ErrMsg);
private:
   const bool self_alloc;	// set if ctor 'new''s region ptr
   const bool seek(std::fstream*, const char&)const;
};

/**
* @brief Storage for vertices of polygon ROI. Currently not
* in use
*/
class bufferedArray: public readWrite {
public:
/**
* @{ */
   bufferedArray(const float* arr, const unsigned& arr_sz, cv::Mat*
	   pcanvas, const float& rat, const cv::Scalar& lc);
   bufferedArray(const float* arr, const unsigned& arr_sz,
	   IplImage*pcanvas, const float& rat, const cv::Scalar& lc);
/**  @} */
   void addElem(const float&);
   void append(const float*, const unsigned&);
   inline const unsigned& getSize()const;
   inline const std::deque<float>& getArray()const;
   void update();   // force re-calc & re-drawing all data points
   void dump()const;
   void write(std::fstream*);
   void read(std::fstream*);
private:
   std::deque<float> data_array;
   std::deque<cv::Point> pt_array;
   const int canvas_width, canvas_height;
   unsigned drawFront;		// front-end index of data for curve drawing
   const float disp_ratio;
   static const char *exRscope, *exWscope;
   void init(const float*);
};

/**
* @brief Manager for ROI selection. Used by other processing
* classes.
*/
class myROI: public readWrite {
public:
/** @{
* @brief Assign pre-allocated new image, set states
* @param pmat pre-allocated image resource
* @param src Two images: current frame and a blank (all-0)
* for flashing polygon ROI after selection.
* When IplImage* type (in 2nd ctor) is passed, local copies
* of thw two images are created.
* @param enable_redraw Do I keep the dashed lines on current
* frame after ROI has been selected?
*/
   myROI(cv::Mat* pmat, cv::Mat* src[2], const bool&
	   enable_redraw=true);
   myROI(IplImage* pmat, IplImage* src[2], const char*const wns[2],
	   const bool& enable_redraw=true);
   ~myROI();
   const cv::Mat* getPolyMat()const{return region;}
/**
* @brief Closes polygon and inits a flash (via changing
* interal state to notify member function update())
*/
   void getROI();
/**
* @brief Gives a brief flash (swap of frame and filled
* polygon and back) when ROI selection completed
*/
   void update();
/**
* @brief Prints polygon vertices
*/
   void dump()const;
/**
* @name File IO for polygon state. Not used.
* @{ */
   void write(std::fstream*);
   void read(std::fstream*);
/**  @} */
protected:
   friend void roiMouseCallBack(const int*, myROI&);
   cv::Mat *psrc, *psrc2;
   const cv::Size imgSz;
   std::deque<cv::Point> pt_poly;
   std::deque<std::vector<unsigned char> > lines_gray1, lines_gray2;
   std::deque<std::vector<cv::Vec3b> > lines_color1, lines_color2;
   bool pPoly_updated, redrawable;
   const bool self_alloc, gray;
   cv::Point* pt_array;
   char *wn1, *wn2;
   typedef enum{off, state1, state2} FlashState;
   FlashState flash;
   static const char *exRscope, *exWscope;
   void pushLine(const bool);
   void redrawLine()throw(ErrMsg);
/**  @} */
/**
* @name Add/remove a point of ROI polygon vertex
* @{ */
   void pushPoint(const cv::Point&);
   const cv::Point popPoint();
/**  @} */
/**
* @brief Resets polygon ROI selection
*/
   void clear();
};

/**
* @brief See http://en.wikipedia.org/wiki/Window_function
* for definitions
*/
class Filter1D{
public:
/**
* @brief FIR window functions
*/
   typedef enum{Rect, Hann, Hamming, Tukey, Cosine, Lanczos, Triangular,
	Bartlett, Gaussian, Bartlett_Hann, Blackman, Kaiser}FilterType;
/**
* @name set filter type, FIR order and additional params
* Note: shallow copy on params.
* @{ */
   Filter1D(const FilterType& ft, const int& n, float**const params, bool
	   norm=false)throw(ErrMsg);
   void update(const FilterType&, const int&, float**const)throw(ErrMsg);
/**  @} */
   const std::vector<float>& getCoefs()const{return coefs;}
   void dump()const;
protected:
   int order; bool norm;
   FilterType curFT;
   float** param;
   std::vector<float> coefs;
   void set()throw(ErrMsg);
   static const float sinc(const float&);
};

/**
* @brief Calcs frequency energy distribution and draws 2D-DFT (or DCT/DWT)
*/
class VideoDFT {
public:
   typedef enum{DFT,DCT,DWT} Transform;
/**
* @brief
* @param _frame Source image from which to perform DFT
* @param bins # uniform bins to divide 2D-DFT by distance
* from DC component, which gives a vector of size `bins` of
* mean energy of DFT in that region
* @param log Is log-transform used in 2D-DFT energy?
*/
   VideoDFT(IplImage* _frame, const Transform& tr, const int& bins,
	   const bool& log=true);
/**
* @name update
* @{
* @brief Notifies that source image has been updated.
* Recalculates 2D-DFT and energy distribution
*/
   void update();
   void update(IplImage*);
/**  @} */
   const std::vector<double>& getEnergyDist()const{return energyDist;}
/**
* @brief Prints energy distribution in each bins
*/
   void dump()const;
/**
* @brief returns transformed 2D-DFT of source image
*/
   const cv::Mat& getDftMag()const{return dftMag;}
/**
* @name File IO for states. Not in use.
* @{ */
   void read(std::fstream*){};
   void write(std::fstream*){};
/**  @} */
   static enum DftPartition{Ring, Fan}DftPartitionMethod;
protected:
   IplImage *frame;
   const Transform tr;
   cv::Mat dftMag;
   const cv::Size dftSz;
   const bool log_energy, padding;
   const int nBin, xCent, yCent, circRadius;
   const float ring_width;
   std::vector<long> binCnt;
   std::vector<double> energyDist;
   std::vector<int> index;
   static const char *exRscope, *exWscope;
   void init();
};

/**
* @brief plot line segments on a canvass
*/
class Plot2D{
public:
/**
* @brief allocates canvass resource and sets coordinate
* dimensions, colors
* @param sz size of canvass (width, height)
* @param dim[4] x_start, x_end, y_start, y_end
* @param bgc background color (BGR)
* @param fgc foreground color, used for axes plotting
* @throw ErrMsg Panick when dim not in increasing order
*/
   Plot2D(const cv::Size& sz, const float dim[4], const cv::Scalar& bgc, const
	   cv::Scalar& fgc)throw(ErrMsg);
   typedef enum{Solid, Dot, Dash, DashDot}LineType;
/**
* @name set coordinates
* @{ */
/**
* @brief sets a whole line (-segments)
* @param x X-Coordinates
* @param y Y-Coordinates
* @param len #pts
* @param fgc foreground color
* @param thick line thickness in pixels
* @param sort Sort x-coordinates before plotting?
* @return Line-ID (starting from 0)
* @throw ErrMsg
*/
   const int set(const float* x, const float* y, const int& len, const cv::Scalar&
	   fgc, const int& thick=1, const LineType& lt=Solid)throw(ErrMsg);
   const int set(const std::vector<float>& x, const std::vector<float>& y, const
	   cv::Scalar& fgc, const int& thick=1, const LineType& lt=Solid)throw(ErrMsg);
/**  @} */
/**
* @brief erase a whole line
* @param id Line-ID to be erased
*/
   void reset(const int& id);
/**
* @name updates dataset for one curve, point-wise or in batch
* @{
* @brief update (insert/remove/update) a single point. When
* inserting, ASSUMES that the original x-coordinates are
* @param id ID of the line to be updated
* @param x coordinate X
* @param y coordinate Y
* @param rm remove this point?
*/
   void update(const int& id, const float& x, const float& y, const bool& rm=false);
   void update(const int& id, const float* x, const float* y, const int& len, const
	   bool& rm=false);
   void update(const int& id, const std::vector<float>& x, const std::vector<float>&
	   y, const bool& rm=false);
   void update(const int& id, const std::vector<cv::Point2f>& pts, const bool&
	   rm=false);
/**  @} */
/**
* @brief Resize canvass
* @param sz new size
*/
   void resize(const cv::Size& sz)throw(ErrMsg);
   void labelAxes(const std::vector<float>&, const std::vector<float>&);
   void labelAxesAuto();
   void redraw();
   void dump();
   const cv::Mat& get()const{return canvass;}
protected:
/**
* @brief x/y-coordinates, x/y-positions on canvass, line-color, thickness, line-type
*/
   typedef boost::tuple<std::vector<cv::Point2f>, std::vector<cv::Point>, cv::Scalar,
	     int, LineType> Line;
   typedef std::map<int, Line> Lines;
   typedef Lines::const_iterator LinesIter;
   typedef std::vector<cv::Point2f> VPF;
   typedef std::vector<cv::Point> VP;
   Lines lines;
   cv::Size size;
   cv::Mat canvass;
   int max_id, x_margin, y_margin;
   float x_start, x_end, y_start, y_end, x_rat, y_rat, fontScale;
   cv::Scalar bgc, fgc;
   static const float xr_margin, yr_margin, dotR, dashR;
   std::vector<float> xlab, ylab;
   void drawAxes();
   void drawLine(const Line& line);
   void labelAxes();
   const cv::Point pf2pi(const cv::Point2f& pf)const{return
	cv::Point(x_margin+(pf.x-x_start)*x_rat, size.height-y_margin-(pf.y-y_start)*y_rat);}
   struct Less1st; struct STATE;
   static cv::LineIterator& selfInc(cv::LineIterator&, const int&, STATE&, int&);
   static std::vector<float> findAxis(const float&, const float&);
};

/**
* @brief animates random objects (ball, triangle, etc)
* bouncing around
*/
class createAnimation{
public:
/**
* @brief Ctor using background image (always converted to
* gray-scale)
* @param fn filename for saving (.avi extension unneeded)
* @param bg background image. Must be valid.
* @param duration in seconds
* @param fps frame per second.
* @throw ErrMsg when fn/bg is invalid
*/
   createAnimation(const char* fn, const IplImage* bg, const
	   float& duration=1, const float& fps=15)throw(ErrMsg);
   createAnimation(const char* fn, const CvScalar& bg, const
	   cv::Size&, const float& =1, const float& =15)throw(ErrMsg);
   ~createAnimation();
   typedef enum{Ball=0, Triangle, Square, Pentagon, Star, Crescent} Object;
/**
* @brief object type, position, size(radius), brightness,
* moving speed (pix/sec), rotation speed (rad/sec), added-on-top
*/
   typedef boost::tuple<Object, cv::Point2f/*position*/, float/*radius*/,
	     CvScalar/*3:fgcolor*/, float/*m_spd */, float/*m_angle*/,
	     float/*6:r_spd */, bool/*added*/> ObjProp;
   int addObj(const ObjProp&);
   void rPopulate(const int&);
   void dump()const;
/**
* @brief start animation process
*/
   void factory()throw(ErrMsg);
/**
* @brief background image frame-by-frame variational type.
* None: background image remains same; Blur: slow changing
* filtering effect; Elevator: cyclic moving effect; Cappuccino:
* image warping around center in spiral style
* NOTE: affective only before creating each object
*/
   typedef enum{None,Blur,Elevator,Cappuccino,Vapor} Bgvt;
/**
* @brief sets background image changing pattern
* @param b type of background changing pattern
* @param f pointer to param data passed to cvSmooth when
* b==Blur (smoothType, param1, ..., param4); or (dist, deg
* (in Rad)) when b==Elevator; or (center_x, center_y, amplitude,
* normalized span, direction) when b==Cappuccino. When NULL,
* Filter defaults to cvSmooth(s1, s2, CV_GAUSSIAN, 0,0,11,20),
* Move defaults to (10, 0) and Cappuccino defaults to
* (center_of_image, .05, .75, counter-clockwise)
*/
   static void setBgPattern(const Bgvt& b, const float* f);
   /**
* @name set Cappucchino kernel function
* @brief set kernel function of Cappuccino vortex. 
* @param f function pointer. It's guaranteed that its
* argument lies in [0, 1], and the return value should
* be noralized between [0, 1]
* @param i one of 4 predefined kernel functions
* @{ */
   static void setWarpFunc(double(*f)(double)){if(f)capKernel=f;}
   static void setWarpFunc(int i){
	if(i>=0 && i<4)capKernel=capKernelArray[i];
   }
/**  @} */
/**
* @brief Use static mapping for Cappuccino? If set to be
* simplified, remap is calculated only once and rounded to
* integral grids (not accurate); else it's calculated for
* each frame w. complexity O(n)
*/
   static void setWarpSimp(bool t){WarpSimp=t;}
   static int fadeDuration;
protected:
   const static float golden_ratio;
   static Bgvt bgvt; static float bparam[5];
   static char Codec[5];
   const bool bgImg;
   IplImage *bg, *add, *bg2; const IplImage* orig;
   int *x_map, *y_map, fc /* frame count, used by warp() */;
   char* name; float duration, fps, **remap/* global pixel remapper */;
   std::vector<ObjProp> objects;
   CvPoint *vertices;
   void drawObj(ObjProp&, const int&, IplImage*);
   static void drawCrescent(IplImage*, const ObjProp&, const float&);
   struct equal_to:public std::binary_function<ObjProp,ObjProp,bool>{
	bool operator()(const ObjProp& ob1, const ObjProp& ob2)const{
	   return ob1.get<1>()==ob2.get<1>() && ob1.get<2>()==ob2.get<2>() &&
		ob1.get<4>()==ob2.get<4>() && ob1.get<5>()==ob2.get<5>();
	}
   };
   struct hasAdd{
	bool operator()(const ObjProp& ob)const{return ob.get<7>();}
   };
   struct hasAddMoveable{
	bool operator()(const ObjProp& ob)const{
	   return ob.get<7>() && (ob.get<4>()>0 || ob.get<0>()!=Ball &&
		   ob.get<6>()>0);
	}
   };
   friend class loadConf;  // allow for modification of Codec (same as VideoRegister)
private:
   static bool WarpSimp;
   static double (*capKernelArray[4])(double),
		     (*capKernel)(double);
   static double norm0(double d){return sin(d*M_PI);}
   static double norm1(double d){return 1;}
   static double norm2(double d){return d;}
   static double norm3(double d){return 1-d;}
   void warp();
   float& access(int i,int j,int k, bool x){
	return remap[i][2*(j*bg->width+k)+!x];
   }
};

