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
#include <bitset>
#include "dynan.hh"

/**
* @brief loads/generates configuration file. File format is:
* name=val. Comments start with '#' sign.
*/
class loadConf{
public:
/**
* @brief 
* @name ctor loads configuration options either from an
* existing conf file or from newly-generated default conf
* file.
* @use_default whether to generate default conf file and use
* its settings?
* @throw ErrMsg cannot open conf file for reading (or
* writing)
*/
   explicit loadConf(const char* name, const bool& use_default)throw(ErrMsg);
   void dump()const;

   static const size_t BoolCap=13, IntCap=19, StrCap=6, FloatCap=20, CritCap=1;
/**
* @name gets conf variables all-at-once
*/
   void get(bool bools[BoolCap], int ints[IntCap], char&, char* str[StrCap],
	   float floats[FloatCap], Criterion crits[CritCap], frameRegister::DiffMethod&,
	   VideoDFT::Transform&, createAnimation::Bgvt&, CvScalar&)const;
   static const char* defaultConfName;
protected:
/**
* @brief customizable options, from conf file
*/
   struct confData{
	std::bitset<BoolCap> conf_bools;
// 	Show_Videos, Show_DT, Show_DtBar, Show_Hist, Constraint_DtLogScale, /* 0-4 */
// 	Constraint_RetrieveGrayFB, Constraint_FRused, Constraint_FRInc, /* 5-7 */
// 	Behavior_StopConvBadFrame, Norm_Hist, Norm_DT, Method_DFT_Ring, Behavior_FR_rmAdjacentEq,	/* 8-12 */
	int Shape_Hist_Gap, Shape_Hist_BarWidth, Shape_Hist_Height, Num_Bin_Hist, Num_Bin_FR, /* 0-4 */
	    Num_FB, Num_DT, Num_FRSearch, Num_MinFrameVideo, Num_PrependFrame, /*5-9*/
	    Num_MA, Num_VideoObj, Norm_Diff, Bright_Tol_Mean, Bright_Tol_Sd, /* 10-14 */
	    Show_FPS, Noise_Type, Video_FPS, Behavior_Prepend_DropMethod; /* 15-18 */
	static const int strCap=128;
	char Bright_PrepFrame, File_Suffix_prep[strCap], File_Suffix_reg[strCap],
	     File_Log[strCap], File_VideoCodec[5], File_BgImage[strCap],
	     File_VideoExtension[strCap];
	float Prob_FrameDrop, Prob_SuccessiveDrop, Noise_Level[3], Video_Duration, Pattern_BgVideo_Blur, /* 0-6 */
		Pattern_BgVideo_Elevator[2], Pattern_BgVideo_Cappuccino[6], Pattern_BgVideo_Vapor[4];	/* 7-19 */
	Criterion CmpCrit_FR;
	frameRegister::DiffMethod Method_Cmp_FR;
	VideoDFT::Transform Method_DT;
	createAnimation::Bgvt Pattern_BgVideo;
	CvScalar Bright_BgMono;
	confData();
   }confdata;
   size_t lineNumber;
   static size_t lineCap;
   const char* fname;
   char* line;
/**
* @brief generates default conf file
* @throw ErrMsg if failed to open file for writing
*/
   void generate()throw(ErrMsg);
   int parseline(FILE*);
   const static char OptionDelim=':';
/**
* @brief Parse 'OptionDelim' (set to ':')-delimited options
* in values
* @param str value string
* @param nOption #of options to be scanned
* @param vals place holder to store scanned values
*/
   static void parseOption(char*const  str, const int& nOption, float* vals);
};

class fsToggle{
public:
   explicit fsToggle(const cv::Mat&);
   explicit fsToggle(const IplImage*);
   void setFs(const cv::Mat&);
   void setFs(const IplImage*);
   const cv::Mat& toggle(){return (fs=!fs)?fs_Mat:orig_Mat;}
   const IplImage* toggle(int){return (fs=!fs)?&fs_img:&orig_img;}
   static const int& width(){if(!Width)getScreenSize(); return Width;}
   static const int& height(){if(!Height)getScreenSize(); return Height;}
   void update();
protected:
   static void getScreenSize()throw(ErrMsg);
   static int Width, Height;
   bool fs;
   cv::Mat orig_Mat, fs_Mat;
   IplImage orig_img, fs_img;
};

/**
* @brief extracts options/arguments from main() and sets
* status
* @param argv passed from main
* @param argc passed from main
* @param optargs string arrays for optarg and main-argument
* @param status bit-wise set from options
* @return 0 on failure
*/
int cv_getopt(int argv, char* argc[], char* optargs[5], int16_t& status);

/**
* @brief displays usage info
* @param progname name of current program. Passed from
* argv[0] of main().
*/
void help(const char* progname);

/**
* @brief core wrapper for functionalities used in main()
* @param optargs set by cv_getopt()
* @param status set by cv_getopt()
* @throw ErrMsg Logic/runtime errors from home-brewed objects
* @throw cv::Exception runtime errors from OpenCV library
* @throw std::exception runtime errors from STL
*/
void cv_procOpt(char*const * optargs, const int16_t& status)throw(ErrMsg,
	cv::Exception,std::exception);
/**
* @relates Logger
* @brief plots summary curves for RMSE, dynamics, difference
* in the end
* @param x_max max range of x-coordinate
* @param log Logger object to retrieve history data
* @param Plot whether to display curve plotting.
*/
void summaryPlot(const int& x_max, const Logger& log, const bool Plot);
