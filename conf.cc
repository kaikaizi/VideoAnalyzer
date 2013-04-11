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
#include <getopt.h>
#include "sketch.hh"
#include "conf.hh"
#include "encoder.hh"
char msg[256];
bool verbose;

/* Magic-number fraught, for getopt and loadConf. */
static char dirDelim='/';
#if defined(__WIN32)
dirDelim='\\';
#endif

const char* loadConf::defaultConfName="VideoAnalyzer.conf";
size_t loadConf::lineCap=512;

void loadConf::generate()throw(ErrMsg){
   FILE* fd=fopen(fname,"w");
   if(!fd){
	sprintf(msg,"loadConf::generate: cannot open conf file \"%s\" for writing.\n", fname); throw ErrMsg(msg);
   }
   fputs("# Configuration of medical video analysis/comparison software.\n",fd);
   fputs("\n# Draw the two videos being examined? Also determines if DT/histogram are drawn.\n#Show.Videos=false\n",fd);
   fputs("\n# Draw the 2-D Fourier transform of two videos being examined?\n#Show.DT=false\n",fd);
   fputs("#--------------------Histogram Settings--------------------\n",fd);
   fputs("\n# Number of bins used to calculate histogram of brightness\n#Num.Bin.Hist=30\n",fd);
   fputs("\n# Normalize two histogram vectors (s.t. they have same mean) before calculating their difference?\n#Norm.Hist=false\n", fd);
   fputs("\n# Draw two histograms in a separate window? It is shown only if ShowVideos=true.\n#Show.Hist=false\n",fd);
   fputs("\n# Gap in pixels between adjacent bars in drawing brightness histogram bars\n#Shape.Hist.Gap=3\n",fd);
   fputs("\n# Width of histogram bar in pixels\n#Shape.Hist.BarWidth=3\n",fd);
   fputs("\n# Canvass height of histogram window in pixels\n#Shape.Hist.Height=140\n",fd);
   fputs("\n#---------------Frame-drop Simulation Settings---------------\n# Simulates frame drop event to evaluate its effect on video quality degradation\n",fd);
   fputs("\n# Probability of an independent frame drop event\n#Prob.FrameDrop=0.3\n",fd);
   fputs("\n# Probability of a successive frame drop if previous frame is dropped. Set it to 0 to make each frame-drop independent event.\n#Prob.SuccessiveDrop=0\n",fd);
   fputs("\n#--------------------Frame Buffer Settings--------------------\n#container to buffer frames of videos\n",fd);
   fputs("\n# Number of frames in the buffer\n#Num.FB=15\n",fd);
   fputs("\n# Retrieve buffered image as grayscale?\n#Constraint.RetrieveGrayFB=true\n",fd);
   fputs("\n#--------------------Discrete-Transform Settings--------------------\n",fd);
   fputs("\n# Discrete-Transform method used: DFT.Ring/DFT.Fan/DCT/DWT. Also specifies the way to partition 2D-DFT image.\n# (rings of equal widt or fans of equal degree).\n#Method.DT=DFT.Ring\n", fd);
   fputs("\n# Number of bins for 2-D Discrete transform\n#Num.Bin.DT=30\n",fd);
   fputs("\n# show DT energy distribution in similar way as histogram?\n#Show.DtBar=false\n", fd);
   fputs("\n# when calculating energy distribution, take log10(1+x) to absolute scale?\n#Constraint.DtLogScale=true\n",fd);
   fputs("\n# Normalize two energy distributions (same as Norm.Hist) for difference?\n#Norm.DT=true\n", fd);
   fputs("\n#--------------------Frame Register Settings--------------------\n",fd);
   fputs("\n# Use frame-register to perform frame-wise registration/sync between two compared videos?\n#Constraint.FRused=true\n",fd);
   fputs("\n# Remove identical adjacent frames from video (assumes that adjacent frames do not equal)?\n# This option is ALWAYS in effect.\n# Behavior.FR.rmAdjacentEq=true\n",fd);
   fputs("\n# Search range of a best-match frame. For the k-th frame and\n# search range of r, searches from (k-r) to (k+r) -th frame\n#Num.FRSearch=20\n",fd);
   fputs("\n# Criterion used to measure how different two frames are.\n# Options are: FrameDiff, HistDiff and DtDiff\n#Method.Cmp.FR=FrameDiff\n",fd);
   fputs("\n# Comparison criterion used to compare two measures.\n# Options are: correlation, chi-square, intersection, bhattacharyya.\n# Stars indicate perfect match for these criteria:\n# correlation:[-1,1*]; chi-squre:[0*,inf]; intersection:[0,1*];\n# bhattacharyya:[0*,1];\n#CmpCrit.FR=correlation\n",fd);
   fputs("\n# Norm of two image difference (in image domain) when CmpCrit.FR=FrameDiff\n#Norm.Diff=1\n",fd);
   fputs("\n# Number of bins used when HistDiff/DtDiff are chosen as measuring criterion\n#Num.Bin.FR=30\n",fd);
   fputs("\n# Should the registered frame number be strictly increasing?\n#Constraint.FRInc=true\n", fd);
   fputs("\n# Heuristic for searching boundary. With option Constraint.FRInc set, the left-most\n# element that is below p-th percentile of differences in search range is determined\n# as matched frame; with that option unset, the element closest to k-th index frame\n# is decided to be. Lies in [0, 1].\n#Heuristic.FR.bound=.1\n", fd);
   fputs("\n#------------Self-testing Video-extension Settings------------\n",fd);
   fputs("\n# Image source used for video extension. Used as background image. When not set,\n# falls back to mono-chrome background.\n# File.BgImage=\n", fd);
   fputs("\n# Mono-chrome background in RGB values\n# Bright.BgMono=50:50:50\n",fd);
   fputs("\n# File name of output video name to save. No extension needed.\nFile.VideoExtension=animate\n",fd);
   fputs("\n# Output Video duration in seconds\n# Video.Duration=10\n", fd);
   fputs("\n# Output Video fps\n# Video.fps=15\n",fd);
   fputs("\n# Number of moving objects in video\nNum.VideoObj=4\n",fd);
   fputs("\n# Background image variational pattern. Alternatives are: None (static image);\n# Blur (Gaussian blur-in/out); Elevator (cyclic moving); Cappuccino (vortex-\n# deformation); Vapor (random deformation)\nPattern.BgVideo=None\n", fd);
   fputs("\n# Blurring duration in seconds when Patter.BgVideo=Blur\nPattern.BgVideo.Blur=1\n",fd);
   fputs("\n# Elevator cycle\n# (number of seconds it takes for either x- or y-direction to cycle back to\n# origin, whichever comes first), moving direction (counter-clockwise, in unit of degrees).\n# Pattern.BgVideo.Elevator=3:0\n", fd);
   fputs("\n# Cappuccino settings: center-x, center-y (normalized between [0, 1]),\n# rotational speed, normalized span (between [0, 1]), rotational direction\n# (positive:clockwise; negative:counter-clockwise); calculating acceleration\n# (disencouraged)\n#Pattern.BgVideo.Cappuccino=.7:.2:.03:1:-1:0\n", fd);
   fputs("\n# Vapor settings: uniform-random number range, spatial Gaussian kernel size, sigma,\n# temporal Hamming window length\nPattern.BgVideo.Vapor=20:11:.7:16\n", fd);
   fputs("\n#--------------------Miscellaneous Settings--------------------\n",fd);
   fputs("\n# Brightness (0-255) of prepended/detected frame\n#Bright.PrepFrame=255\n",fd);
   fputs("\n# Minimal number of frames allowable in a frame-position registered video output\n#Num.MinFrameVideo=20\n",fd);
   fputs("\n# Tolerance for mean gray-level of a frame to be regarded as a prepended frame\n#Bright.Tol.Mean=5\n",fd);
   fputs("\n# Tolerance for the standard deviation of gray-level of a frame to be regarded as a prepended frame\n#Bright.Tol.Sd=15\n",fd);
   fputs("\n# Stop video conversion on first unsuccessfully grabbed frame?\n#Behavior.StopConvBadFrame=true\n", fd);
   fputs("\n# number of consecutive prepending frame when resaving video. Useful to compare lossy video encoding.\n# When negative, the first '-p' frames of original video are skipped.\n#Num.PrependFrame=15\n", fd);
   fputs("\n# name suffix for prepended video output. e.g. prepended video for\n# \"abc.avi\" becomes \"abc_suffix.avi\", when this option is set to \"_suffix\"\n#File.Suffix.prep=_prep\n", fd);
   fputs("\n# name suffix for registered video output. Same as prepSuffix\n#File.Suffix.reg=_reg\n", fd);
   fputs("\n# file to write comparison results into. Defaults to stdout, as its\n# name suggests. \"%f\" is substituted by the file name of primary video name\n# before '-' and file extension, e.g. \"%f_lan.log\" with abc.avi uses file\n# \"abc_lan.log\" for logging.\n#File.Log=stdout\n", fd);
   fputs("\n# Video codec for AVI container in saving video format. Also used by self-testing video-extension. Alternatives supported by\n# OpenCV are: PIM1 -- MPEG-1; MJPG -- motion-jpeg; MP42 -- MPEG4.2;\n# DIV3 -- MPEG4.3; U263 -- H263; I263 -- H263I; FLV1 -- FLV1; IYUV -- default; XVID; DIVX; H264\n#File.VideoCodec=MJPG\n", fd);
   fputs("\n# Mask file location. Leave empty to avoid loading mask\nFile.Mask=\n", fd);
   fputs("\n# Moving average order of dynamic of two videos under study\n#num.MA=1\n", fd);
   fputs("\n# Frame per sec when playing videos. Due to computational burden, typically actual FPS < half of given number\n#Show.FPS=10\n", fd);
   fputs("\n# Type of additive noise polluted to each frame before prepending frame and saving video to a file.\n# Alternatives are: none, salt-peper, gaussian, duplicate.\n#Noise.Type=none\n", fd);
   fputs("\n# Additive noise level specified as SNR in unit of Decibel for salt-peper, sigma (standard deviation) for gaussian; added weight for duplicate, polluted to each frame before saving to file\n#Noise.Level=0\n", fd);
   fputs("\n# Apply drop sequence when prepending frame to video? 'none' avoids applying\n# drop sequence, 'drop' deliberately drops certain frames, 'freeze' replaces\n# dropped frames with previous ones, 'both' freezes portion of dropped\n# sequence.\n", fd);
   fclose(fd);
}

loadConf::confData::confData():conf_bools(0xff0),Shape_Hist_Gap(3),Shape_Hist_BarWidth(3),
   Shape_Hist_Height(140),Num_Bin_Hist(30),Num_Bin_FR(30),Num_FB(15),Num_DT(30),Num_FRSearch(20),
   Num_MinFrameVideo(20),Num_PrependFrame(0),Num_MA(1),Num_VideoObj(5),Norm_Diff(1),Bright_Tol_Mean(10),
   Bright_Tol_Sd(5),Show_FPS(10),Behavior_Prepend_DropMethod(0),Bright_PrepFrame(0xff),
   Bright_BgMono(cv::Scalar(50,50,50)),Prob_FrameDrop(.3),Prob_SuccessiveDrop(0),Noise_Type(0),
   Video_FPS(15),Pattern_BgVideo_Blur(1),Heuristic_FR_bound(.3),Video_Duration(10),
   CmpCrit_FR(Correlation),Method_DT(VideoDFT::DFT),Method_Cmp_FR(frameRegister::FrameDiff){
   memset(File_Suffix_prep,0,strCap); memset(File_Suffix_reg,0,strCap);
   memset(File_Log,0,strCap); memset(File_VideoCodec,0,5); memset(File_BgImage,0,strCap);
   memset(File_VideoExtension,0,strCap); memset(File_Mask,0,strCap);
   memset(Noise_Level,3,sizeof(float));
   strcpy(File_Suffix_prep,"_prep"); strcpy(File_Suffix_reg,"_reg");
   strcpy(File_Log,"stdout"); strcpy(File_VideoCodec,"DIV3");
   strcpy(File_VideoExtension,"animate");
   Pattern_BgVideo_Elevator[0]=0; Pattern_BgVideo_Elevator[1]=2;
   Pattern_BgVideo_Cappuccino[0]=Pattern_BgVideo_Cappuccino[1]=0; Pattern_BgVideo_Cappuccino[2]=.03;
   Pattern_BgVideo_Cappuccino[3]=1; Pattern_BgVideo_Cappuccino[4]=-1; Pattern_BgVideo_Vapor[0]=20;
   Pattern_BgVideo_Vapor[1]=11; Pattern_BgVideo_Vapor[2]=.7; Pattern_BgVideo_Vapor[3]=16;
}

loadConf::loadConf(const char* name, const bool& use_default)throw(ErrMsg):fname(name),lineNumber(1),
   line(0){
   if(use_default) generate();
   else{
	FILE* fd=fopen(name,"r");
	if(!fd){
	   sprintf(msg,"loadConf::ctor: cannot open conf file  \"%s\" for reading.\n",
		   fname); throw ErrMsg(msg);
	}
	line = new char[lineCap+1];
	while(parseline(fd));
	fclose(fd); delete[]line;
   }
}

int loadConf::parseline(FILE* fd){
   if(getline(&line, &lineCap, fd)==-1)return 0;
   char *pch=strchr(line,'#');
   if(line==pch||line[0]=='\n')return ++lineNumber;   // empty line
   if(pch)*pch=0;
   char* option=strtok(line," =\n\t"), *value=strtok(0," =\n\t");
   if(!value) return fprintf(stderr, "Warning: bad option \"%s\" on line %d\n", option, lineNumber++);
   if(!strcmp("Show.Videos",option))confdata.conf_bools[0]=strcasecmp("false",value);
   else if(!strcmp("Show.DT",option))confdata.conf_bools[1]=strcasecmp("false",value);
   else if(!strcmp("Show.DtBar",option))confdata.conf_bools[2]=strcasecmp("false",value);
   else if(!strcmp("Show.Hist",option))confdata.conf_bools[3]=strcasecmp("false",value);
   else if(!strcmp("Constraint.DtLogScale",option))
	confdata.conf_bools[4]=strcasecmp("false",value);
   else if(!strcmp("Constraint.RetrieveGrayFB",option))
	confdata.conf_bools[5]=strcasecmp("false",value);
   else if(!strcmp("Constraint.FRused",option))confdata.conf_bools[6]=strcasecmp("false",value);
   else if(!strcmp("Constraint.FRInc",option)) confdata.conf_bools[7]=strcasecmp("false",value);
   else if(!strcmp("Behavior.StopConvBadFrame",option))
	confdata.conf_bools[8]=strcasecmp("false",value);
   else if(!strcmp("Norm.Hist",option))confdata.conf_bools[9]=strcasecmp("false",value);
   else if(!strcmp("Norm.DT",option))  confdata.conf_bools[10]=strcasecmp("false",value);
   else if(!strcmp("Behavior.FR.rmAdjacentEq",option)) confdata.conf_bools[12]=strcasecmp("false",value);
   else if(!strcmp("Num.HistBin",option))	confdata.Num_Bin_Hist=atoi(value);
   else if(!strcmp("Shape.Hist.Gap",option))	confdata.Shape_Hist_Gap=atoi(value);
   else if(!strcmp("Shape.Hist.BarWidth",option))confdata.Shape_Hist_BarWidth=atoi(value);
   else if(!strcmp("Shape.Hist.Height",option))	confdata.Shape_Hist_Height=atoi(value);
   else if(!strcmp("Num.FB",option))		confdata.Num_FB=atoi(value);
   else if(!strcmp("Num.Bin.DT",option))	confdata.Num_DT=atoi(value);
   else if(!strcmp("Num.FRSearch",option))confdata.Num_FRSearch=atoi(value);
   else if(!strcmp("Num.Bin.FR",option))	confdata.Num_Bin_FR=atoi(value);
   else if(!strcmp("Norm.Diff",option))	confdata.Norm_Diff=atoi(value);
   else if(!strcmp("Num.MinFrameVideo",option))  confdata.Num_MinFrameVideo=atoi(value);
   else if(!strcmp("Num.PrependFrame",option))confdata.Num_PrependFrame=atoi(value);
   else if(!strcmp("Num.VideoObj",option))confdata.Num_VideoObj=atoi(value);
   else if(!strcmp("Bright.Tol.Mean",option))confdata.Bright_Tol_Mean=atoi(value);
   else if(!strcmp("Bright.Tol.Sd",option))  confdata.Bright_Tol_Sd=atoi(value);
   else if(!strcmp("Num.MA",option))	confdata.Num_MA=atoi(value);
   else if(!strcmp("Show.FPS",option))	confdata.Show_FPS=atoi(value);
   else if(!strcmp("Video.FPS",option))	confdata.Video_FPS=atoi(value);
   else if(!strcmp("Bright.PrepFrame",option))	confdata.Bright_PrepFrame=static_cast<char>(atoi(value));
   else if(!strcmp("Prob.FrameDrop",option)) confdata.Prob_FrameDrop=static_cast<float>(atof(value));
   else if(!strcmp("Prob.SuccessiveDrop",option))confdata.Prob_SuccessiveDrop=static_cast<float>(atof(value));
   else if(!strcmp("Video.Duration",option))	confdata.Video_Duration=static_cast<float>(atof(value));
   else if(!strcmp("Pattern.BgVideo.Blur",option))confdata.Pattern_BgVideo_Blur=static_cast<float>(atof(value));
   else if(!strcmp("Heuristic.FR.bound",option))confdata.Heuristic_FR_bound=static_cast<float>(atof(value));
   else if(!strcmp("Pattern.BgVideo.Elevator",option)) parseOption(value,2,confdata.Pattern_BgVideo_Elevator);
   else if(!strcmp("Pattern.BgVideo.Cappuccino",option))parseOption(value,6,confdata.Pattern_BgVideo_Cappuccino);
   else if(!strcmp("Pattern.BgVideo.Vapor",option))parseOption(value,4,confdata.Pattern_BgVideo_Vapor);
   else if(!strcmp("Behavior.Prepend.DropMethod",option)){
	if(!strcasecmp("none",value))confdata.Behavior_Prepend_DropMethod=0;
	else if(!strcasecmp("drop",value))confdata.Behavior_Prepend_DropMethod=1;
	else if(!strcasecmp("freeze",value))confdata.Behavior_Prepend_DropMethod=2;
	else if(!strcasecmp("both",value))confdata.Behavior_Prepend_DropMethod=3;
	else return fprintf(stderr,"Warning: invalid value \"%s\" for option \"%s\" on"
		" line %d\n", value, option, lineNumber++);
   }
   else if(!strcmp("Pattern.BgVideo",option)){
	if(!strcasecmp("None",value))		confdata.Pattern_BgVideo=createAnimation::None;
	else if(!strcasecmp("Blur",value))	confdata.Pattern_BgVideo=createAnimation::Blur;
	else if(!strcasecmp("Elevator",value)) confdata.Pattern_BgVideo=createAnimation::Elevator;
	else if(!strcasecmp("Cappuccino",value))confdata.Pattern_BgVideo=createAnimation::Cappuccino;
	else if(!strcasecmp("Vapor",value))confdata.Pattern_BgVideo=createAnimation::Vapor;
	else return fprintf(stderr,"Warning: invalid value \"%s\" for option \"%s\" on"
		" line %d\n", value, option, lineNumber++);
   }else if(!strcmp("Noise.Level",option))parseOption(value,3,confdata.Noise_Level);
   else if(!strcmp("Method.DT",option)){
	if(!strcasecmp("DFT",value)||!strcasecmp("DFT.Ring",value))
	   confdata.Method_DT=VideoDFT::DFT, confdata.conf_bools[11]=true;
	else if(!strcasecmp("DFT.Fan",value))confdata.Method_DT=VideoDFT::DFT, confdata.conf_bools[11]=false;
	else if(!strcasecmp("DCT",value))confdata.Method_DT=VideoDFT::DCT;
	else if(!strcasecmp("DWT",value))confdata.Method_DT=VideoDFT::DWT;
	else return fprintf(stderr, "Warning: Unknown transform method \"%s\" for option "
		"\"%s\" on line %d\n", value, option, lineNumber++);
   }else if(!strcmp(option, "CmpCrit.FR")){
	if(!strcasecmp("chi-square",value))confdata.CmpCrit_FR=Chi_square;
	else if(!strcasecmp("intersection",value))confdata.CmpCrit_FR=Intersection;
	else if(!strcasecmp("bhattacharyya",value))confdata.CmpCrit_FR=Bhattacharyya;
	else return fprintf(stderr,"Warning: invalid value \"%s\" for option \"%s\" on"
		" line %d\n", value, option, lineNumber++);
   }else if(!strcmp("Method.Cmp.FR", option)){
	if(!strcasecmp("FrameDiff",value)) confdata.Method_Cmp_FR=frameRegister::FrameDiff;
	else if(!strcasecmp("HistDiff",value)) confdata.Method_Cmp_FR=frameRegister::HistDiff;
	else if(!strcasecmp("DtDiff",value))  confdata.Method_Cmp_FR=frameRegister::DtDiff;
	else return fprintf(stderr,"Warning: invalid value \"%s\" for option \"%s\" on"
		" line %d\n", value, option, lineNumber++);
   }else if(!strcmp("File.Suffix.prep",option)) strcpy(confdata.File_Suffix_prep, value);
   else if(!strcmp("File.Suffix.reg",option))	strcpy(confdata.File_Suffix_reg, value);
   else if(!strcmp("File.Log",option)) strcpy(confdata.File_Log, strcasecmp("stdout",value)?value:"stdout");
   else if(!strcmp("File.VideoExtension",option)) strcpy(confdata.File_VideoExtension, value);
   else if(!strcmp("File.BgImage",option)) strcpy(confdata.File_BgImage, value);
   else if(!strcmp("File.VideoCodec",option)){
	if(!strcasecmp("PIM1",value))	    strcpy(confdata.File_VideoCodec,"PIM1");
	else if(!strcasecmp("MJPG",value))strcpy(confdata.File_VideoCodec,"MJPG");
	else if(!strcasecmp("MP42",value))strcpy(confdata.File_VideoCodec,"MP42");
	else if(!strcasecmp("DIV3",value))strcpy(confdata.File_VideoCodec,"DIV3");
	else if(!strcasecmp("U263",value))strcpy(confdata.File_VideoCodec,"U263");
	else if(!strcasecmp("I263",value))strcpy(confdata.File_VideoCodec,"I263");
	else if(!strcasecmp("FLV1",value))strcpy(confdata.File_VideoCodec,"FLV1");
	else if(!strcasecmp("I263",value))strcpy(confdata.File_VideoCodec,"I263");
	else if(!strcasecmp("IYUV",value))strcpy(confdata.File_VideoCodec,"IYUV");
	else if(!strcasecmp("XVID",value))strcpy(confdata.File_VideoCodec,"XVID");
	else if(!strcasecmp("DIVX",value))strcpy(confdata.File_VideoCodec,"DIVX");
	else if(!strcasecmp("H264",value))strcpy(confdata.File_VideoCodec,"H264");
	else return fprintf(stderr, "Warning: unrecognized codec \"%s\" for option "
		"\"%s\" on line %d\n", value, option, lineNumber++);
	// No need/way to manually set codec option
	strcpy(VideoRegister::Codec, confdata.File_VideoCodec);
	strcpy(createAnimation::Codec, confdata.File_VideoCodec);
	strcpy(VideoCodec::Codec, confdata.File_VideoCodec);
   }else if(!strcmp("File.Mask",option))strcpy(confdata.File_Mask,value);
   else if(!strcmp("Noise.Type",option)){
	if(!strcasecmp("none",value))			confdata.Noise_Type = 0;
	else if(!strcasecmp("salt-pepper",value))	confdata.Noise_Type = 1;
	else if(!strcasecmp("gaussian",value))	confdata.Noise_Type = 2;
	else if(!strcasecmp("duplicate",value))	confdata.Noise_Type = 3;
	else return fprintf(stderr, "Warning: unrecognized noise type \"%s\" for "
		"option \"%s\" on line %d\n", value, option, lineNumber++);
   }
   else return fprintf(stderr, "Warning: unrecognized option \"%s\" on line %d\n",
	     	line, lineNumber++);
   return ++lineNumber;
}

void loadConf::parseOption(char*const  str, const int& nOption, float* vals){
   if(!str || !vals)return;
   char* vals_ch[nOption];
   vals_ch[0]=strtok(str,":");
   if(vals_ch[0])
	for(int indx=1; indx<nOption; ++indx) vals_ch[indx]=strtok(0,":");
   for(int indx=0; indx<nOption && vals_ch[indx]; ++indx)
	vals[indx] = static_cast<float>(atof(vals_ch[indx]));
}

void loadConf::dump()const{
   puts("loadConf::dump():\n");
   printf("\tShow.Videos=%d, Show.DT=%d, Show.DtBar=%d, Show.Hist=%d, Constraint.DtLogScale=%d, "
	   "Constraint.RetrieveGrayFB=%d, Constraint.FRused=%d, Constraint.FRInc=%d, "
	   "Behavior.StopConvBadFrame=%d, Norm.Hist=%d, Norm.DT=%d, Method.DFT.Ring=%d, "
	   "Behavior.FR.rmAdjacentEq=%d\n", confdata.conf_bools[0], confdata.conf_bools[1],
	   confdata.conf_bools[2], confdata.conf_bools[3], confdata.conf_bools[4], confdata.conf_bools[5],
	   confdata.conf_bools[6], confdata.conf_bools[7], confdata.conf_bools[8], confdata.conf_bools[9],
	   confdata.conf_bools[10], confdata.conf_bools[11], confdata.conf_bools[12]);
   printf("\tShape.Hist.Gap=%d, Shape.Hist.BarWidth=%d, Shape.Hist.Height=%d, Num.Bin.Hist=%d, Num.Bin.FR=%d, "
	   "Num.FB=%d, Num.Bin.DT=%d, Num.FRSearch=%d, Num.MinFrameVideo=%d, Num.PrependFrame=%d, Num.MA=%d, "
	   "Num.VideoObj=%d, Norm.Diff=%d, Bright.Tol.Mean=%d, Bright.Tol.Sd=%d, Show.FPS=%d, Video.FPS=%d\n",
	   confdata.Shape_Hist_Gap, confdata.Shape_Hist_BarWidth, confdata.Shape_Hist_Height,
	   confdata.Num_Bin_Hist, confdata.Num_Bin_FR, confdata.Num_FB, confdata.Num_DT, confdata.Num_FRSearch,
	   confdata.Num_MinFrameVideo, confdata.Num_PrependFrame, confdata.Num_MA, confdata.Num_VideoObj,
	   confdata.Norm_Diff, confdata.Bright_Tol_Mean, confdata.Bright_Tol_Sd, confdata.Show_FPS,
	   confdata.Video_FPS);
   printf("\tBright.PrepFrame=%d\n", confdata.Bright_PrepFrame);
   printf("\tProb.FrameDrop=%.1f, Prob.SuccessiveDrop=%.1f, Noise.Level=[%.1f %.1f %.1f], Video.Duration=%.1f, "
	   "Pattern.BgVideo.Blur=%.1f, Pattern.BgVideo.Elevator=[%.1f %.1f], Pattern.BgVideo.Cappuccino=[%.1f "
	   "%.1f %.1f %.1f %.1f %.1f], Pattern.BgVideo.Vapor=[%.1f %.1f %.1f %.1f], Heuristic.FR.bound=%.1f\n",
	   confdata.Prob_FrameDrop, confdata.Prob_SuccessiveDrop, confdata.Noise_Level[0],
	   confdata.Noise_Level[1], confdata.Noise_Level[2], confdata.Video_Duration,
	   confdata.Pattern_BgVideo_Blur, confdata.Pattern_BgVideo_Elevator[0],
	   confdata.Pattern_BgVideo_Elevator[1], confdata.Pattern_BgVideo_Cappuccino[0],
	   confdata.Pattern_BgVideo_Cappuccino[1], confdata.Pattern_BgVideo_Cappuccino[2],
	   confdata.Pattern_BgVideo_Cappuccino[3], confdata.Pattern_BgVideo_Cappuccino[4],
	   confdata.Pattern_BgVideo_Cappuccino[5], confdata.Pattern_BgVideo_Vapor[0],
	   confdata.Pattern_BgVideo_Vapor[1], confdata.Pattern_BgVideo_Vapor[2],
	   confdata.Pattern_BgVideo_Vapor[3], confdata.Heuristic_FR_bound); 
   printf("\tCmpCrit.FR=%s, NoiseType=%s, Behavior.Prepend.DropMethod=%s\n",
	   confdata.CmpCrit_FR==Correlation?"correlation":confdata.CmpCrit_FR==Chi_square?
	   "chi-square":confdata.CmpCrit_FR==Intersection?"intersection":"Bhattacharyya",
	   confdata.Noise_Type?(confdata.Noise_Type==1?"pepper-salt":"Gaussian"):"None",
	   confdata.Behavior_Prepend_DropMethod==0?"none":confdata.Behavior_Prepend_DropMethod==1?
	   "drop":confdata.Behavior_Prepend_DropMethod==2?"freeze":"both");
   printf("\tMethod.Cmp.FR=%s\n", confdata.Method_Cmp_FR==frameRegister::FrameDiff?"FrameDiff":
	   (confdata.Method_Cmp_FR==frameRegister::HistDiff?"HistDiff":"DtDiff"));
   printf("\tTransform method=%s, 2D-DFT partition=%s\n", confdata.Method_DT==
	   VideoDFT::DFT?"DFT":confdata.Method_DT==VideoDFT::DCT?"DCT":"DWT",
	   confdata.conf_bools[11]?"Ring":"Fan");
   printf("\tFile.Suffix.prep=\"%s\", File.Suffix.reg=\"%s\", File.Log=\"%s\", File.VideoCodec=\"%s\""
	   "File.BgImage=\"%s\", File.VideoExtension=\"%s\", File.Mask=\"%s\"\n", confdata.File_Suffix_prep,
	   confdata.File_Suffix_reg, confdata.File_Log, confdata.File_VideoCodec,
	   confdata.File_BgImage, confdata.File_VideoExtension, confdata.File_Mask);
   printf("\tPattern.BgVideo=\"%s\", Bright.Mono=[%.0f %.0f %.0f]\n",
	   confdata.Pattern_BgVideo==createAnimation::None?"None":(confdata.Pattern_BgVideo==createAnimation::Blur?
		"Blur":confdata.Pattern_BgVideo==createAnimation::Elevator?"Elevator":"Cappuccino"),
	   confdata.Bright_BgMono.val[0], confdata.Bright_BgMono.val[1], confdata.Bright_BgMono.val[2]);
}

void loadConf::get(bool bools[BoolCap], int ints[IntCap], char& chars, char*
	str[StrCap], float floats[FloatCap], Criterion crits[CritCap],
   frameRegister::DiffMethod& dm, VideoDFT::Transform& tr, createAnimation::Bgvt&
   bgvt, CvScalar& scal)const{
   for(int indx=0; indx<BoolCap; ++indx)bools[indx]=confdata.conf_bools[indx];
   ints[0]=confdata.Shape_Hist_Gap, ints[1]=confdata.Shape_Hist_BarWidth,
	ints[2]=confdata.Shape_Hist_Height, ints[3]=confdata.Num_Bin_Hist,
	ints[4]=confdata.Num_Bin_FR, ints[5]=confdata.Num_FB,
	ints[6]=confdata.Num_DT, ints[7]=confdata.Num_FRSearch,
	ints[8]=confdata.Num_MinFrameVideo, ints[9]=confdata.Num_PrependFrame,
	ints[10]=confdata.Num_MA, ints[11]=confdata.Num_VideoObj,
	ints[12]=confdata.Norm_Diff, ints[13]=confdata.Bright_Tol_Mean,
	ints[14]=confdata.Bright_Tol_Sd, ints[15]=confdata.Show_FPS,
	ints[16]=confdata.Noise_Type, ints[17]=confdata.Video_FPS,
	ints[18]=confdata.Behavior_Prepend_DropMethod;
   chars=confdata.Bright_PrepFrame;
   str[0]=const_cast<char*>(confdata.File_Suffix_prep),
	str[1]=const_cast<char*>(confdata.File_Suffix_reg),
	str[2]=const_cast<char*>(confdata.File_Log),
	str[3]=const_cast<char*>(confdata.File_VideoCodec),
	str[4]=const_cast<char*>(confdata.File_BgImage),
	str[5]=const_cast<char*>(confdata.File_VideoExtension),
	str[6]=const_cast<char*>(confdata.File_Mask);
   floats[0]=confdata.Prob_FrameDrop, floats[1]=confdata.Prob_SuccessiveDrop,
	memcpy(floats+2, confdata.Noise_Level, 3*sizeof(float)),
	floats[5]=confdata.Video_Duration, floats[6]=confdata.Pattern_BgVideo_Blur,
	memcpy(floats+7, confdata.Pattern_BgVideo_Elevator, 2*sizeof(float)),
	memcpy(floats+9, confdata.Pattern_BgVideo_Cappuccino, 6*sizeof(float)),
	memcpy(floats+15, confdata.Pattern_BgVideo_Vapor, 4*sizeof(float)),
	floats[20]=confdata.Heuristic_FR_bound;
   crits[0]=confdata.CmpCrit_FR;
   dm=confdata.Method_Cmp_FR; tr=confdata.Method_DT;
   bgvt=confdata.Pattern_BgVideo; scal=confdata.Bright_BgMono;
}

void help(const char* progname){
   printf("Usage: %s [-c/--compare primary transmitted] [-C/--conf conf-name] [-d/--diff-mode n]\n"
	   "[-g/--gen-conf] [-G/--gen-video] [-p/--prepend] [-r/--register reference] [-v/--verbose]\n"
	   "[--simulate] [--as-client url ] Video\n", progname);
   puts("-c/--compare transmitted primary: Compares video quality degradation.");
   puts("-C/--conf conf-name: use given name, overriding \"VideoAnalyzer.conf\"");
   puts("-d/--diff-mode n: self-comparison differential mode, treating two frame with "
	   "index difference=n. \"frame-reg\" option ignored.");
   puts("-g/--gen-conf: generate configuration file. Defaults to\"VideoAnalyzer.conf\"");
   puts("-G/--gen-video: generate video clip based on background image/color.");
   puts("-p/--prepend: prepend a frame to video and save "
	   "prepended video to a new file.");
   puts("-r/--register reference: register current video using reference video "
	   "\"reference\" and save registered video to a new file.");
   puts("-v/--verbose: prints more information in processing");
   puts("--simulate: simulates frame dropping as the secondary video.");
   puts("--as-client: work as RTSP client to display/save streamed video.\n"
	   "Use - to suppress video saving.");
}

int cv_getopt(int argc, char* argv[], char* optargs[5], int16_t& status){
   int c, option_index, opt_flag;
   char *confname = optargs[0], *reference = optargs[1], *cmp = optargs[2],
	  *main_arg = optargs[3], *diff_n = optargs[4];
   status=0;
   struct option long_options[]={
	{"simulate", no_argument, &opt_flag,    1 },
	{"conf",	 required_argument,  0,	   'C'},
	{"gen-conf", no_argument,	   0,	   'g'},
	{"gen-video",no_argument,	   0,	   'G'},
	{"prepend",  no_argument,	   0,	   'p'},
	{"register", required_argument,  0,	   'r'},
	{"verbose",	 no_argument,	   0,	   'v'},
	{"compare",	 required_argument,  0,	   'c'},
	{"diff-mode",required_argument,  0,    'd'},
	{"as-client",required_argument,&opt_flag,2},
	{0,		 0,			   0,	    0 }};
   while(true){
	if((c=getopt_long(argc,argv,"c:C:d:gGpr:v",long_options,&option_index))==-1)
	   break;
	switch(c){
	   case 0:
		if(opt_flag=*long_options[option_index].flag && (status|=opt_flag)&2)
		   return rtsp_client(optarg,argv[optind]);
	   case 'C':
		strcpy(confname, optarg); status |= 0x2; break;
	   case 'g': status |= 0x4; break;
	   case 'p': status |= 0x8; break;
	   case 'r':
		strcpy(reference, optarg); status |= 0x10; break;
	   case 'v': verbose=true; break;
	   case 'c':
		strcpy(cmp, optarg); status |= 0x40; break;
	   case 'G': status |= 0x80; break;
	   case 'd':
		strcpy(diff_n, optarg); status |=0x100; break;
	   default: help(argv[0]); return 0;
	}
   }
   if(status&0x100) status&=0xffa6;	// mask-out 'simulate', 'prepend' and 'register'
   if(status&0x4) return status==0x4 ? 1 :
	   !puts("Error: gen-conf option cannot be combined with other options.");
   else if((status&0x80) && argv[optind]) return
	!puts("Error: gen-video mode do not accept additional arguments.\nMixed with"
		"prepend mode?");
   else if(!(status&0x80) && argc-optind!=1){
	puts("Error: Missing main argument.\n");
	help(argv[0]); return 0;
   }else if((status&0x18) == 0x18)	// check for conflicts
	return !puts("Error: prepend/register does not mix.");
   else if((status&0x58) > 0x47) return
	!puts("Error: prepend/register does not mix with comparison");
   else if((status&0x41) == 0x41) return
	!puts("Error: simulation does not mix with comparison");
   else if(status&0x100 && atoi(diff_n)<0) return
	!puts("Error: diff-mode: difference frame index invalid.");
   if(!(status&0x4)){	// using default configuration file (from executable directory)
	const char* p=argv[0]; int len=strlen(p), indx=len-1;
	while(indx && *(p+indx)!=::dirDelim)--indx; ++indx;
	strncpy(confname, p, indx);
	strcpy(confname+indx, loadConf::defaultConfName);
	confname[strlen(loadConf::defaultConfName)+indx] = 0;
	FILE* fp=fopen(confname,"r");
	if(!fp){		// search in home directory
	  memset(confname,0,strlen(confname));
	  strcpy(confname, getenv("HOME")); size_t len=strlen(confname)+1;
	  confname[strlen(confname)]=::dirDelim;
	  strcpy(confname+len, loadConf::defaultConfName);
	  confname[len+strlen(loadConf::defaultConfName)+1]=0;
	}else fclose(fp);
   }
   if(status&0x80)return 1;
   strcpy(main_arg,argv[optind]);
   return 1;
}

void summaryPlot(const int& x_max, const Logger& log, const bool Plot){
   const int LogSize=10; simpStat<std::vector<float> > ss[LogSize];
   size_t sz=log.get(0).size();
   for(int indx=0; indx<LogSize; ++indx){
	ss[indx].update(log.get(indx).begin(), log.get(indx).end());
   }
   float ymax=0, p75;
   const char* category[]={"Dynamics #1","Dynamics #2","Difference","Hist_Corr",
	"Hist_Chisq","Hist_Inter","Hist_Bhatt","DT_Corr","DT_Inter", "DT_Bhatt"};
   fputs("\nSimple stats:\tMean\tSd\tMedia\t25 Perc\t 75 Perc\n", log.file.fd);
   for(int indx=0; indx<LogSize; ++indx){
	if(ymax<(p75=ss[indx].perc(.75)))ymax=p75;
	fprintf(log.file.fd, "%s: %Lf\t%Lf\t%Lf\t%Lf\t%f\n", category[indx], ss[indx].mean(),
		ss[indx].sd(), ss[indx].perc(.5), ss[indx].perc(.25), p75);
   }
   if(!Plot)return;
   float dims[]={0., static_cast<float>(x_max), 0., ymax};
   std::vector<float> x(sz); Plot2D::LineType lt[11];
   for(int indx=0; indx<3; ++indx)lt[indx]=Plot2D::Solid;
   for(int indx=3; indx<10;)lt[indx++]=Plot2D::Dot,
	lt[indx++]=Plot2D::Dash, lt[indx++]=Plot2D::DashDot;
   for(int indx=1; indx<sz; ++indx)x[indx]=indx;
   Plot2D plot(cv::Size(800,600), dims, cv::Scalar(0), cv::Scalar(255,255,255));
   cv::Scalar fg[]={ // Dynamics, difference
	cv::Scalar(128,100,200), cv::Scalar(128,100,180), cv::Scalar(200,100,128),
	cv::Scalar(128,0,200), cv::Scalar(128,0,200), cv::Scalar(200,0,128), cv::Scalar(250,50,155),
	cv::Scalar(200,20,200), cv::Scalar(200,60,100), cv::Scalar(200,80,150), cv::Scalar(250,150,155)};
   int thick[]={1,1,1, 2,2,2,2, 1,1,1,1};
   for(int indx=0; indx<LogSize; ++indx)
	if(indx<3) plot.set(x, log.get(indx), fg[indx], thick[indx], lt[indx]);
	else{
	   int ratio = std::min(1000,std::max(1,
			static_cast<int>(ymax/ss[indx].perc(1)/20)*20));
	   std::vector<float> f(log.get(indx));
	   std::transform(f.begin(),f.end(),f.begin(),
		   std::bind1st(std::multiplies<float>(), ratio));
	   printf("[%s]: zoom-ratio=%d\n", category[indx],ratio);
	   plot.set(x, f, fg[indx], thick[indx], lt[indx]);
	}
   plot.labelAxesAuto();
   cv::imshow("Summary", plot.get());
   fsToggle fs(plot.get());
   plot.resize(cv::Size(fsToggle::width(), fsToggle::height()));
   fs.setFs(plot.get());
   bool pm=false; char c;
   while((c=cvWaitKey(0))!=VideoCtrlStream::ESC && c!=VideoCtrlStream::Quit){
	if(c=='f')cv::imshow("Summary",fs.toggle());
	else if(!pm&&(pm=true))puts("Press q/Esc to end program.");
   }
}

// -------------fsToggle: shallow-copy required----------------

int fsToggle::Width, fsToggle::Height;

fsToggle::fsToggle(const cv::Mat& mat):fs(false),orig_Mat(mat),orig_img(IplImage(orig_Mat)),
   fs_Mat(cv::Size(width(),height()), orig_Mat.type()),fs_img(IplImage(fs_Mat)){
// interpolation methods: INTER_LINEAR (default),
// INTER_NEAREST, INTER_CUBIC, INTER_AREA, INTER_LANCZOS4,
// INTER_MAX, WARP_INVERSE_MAP
   cv::resize(orig_Mat, fs_Mat, cv::Size(width(),height()), 0, 0, cv::INTER_CUBIC);
}

fsToggle::fsToggle(const IplImage* img):fs(false),orig_Mat(img),orig_img(IplImage(orig_Mat)),
   fs_Mat(cv::Size(width(),height()),orig_Mat.type()){
   cv::resize(orig_Mat, fs_Mat, cv::Size(width(),height()), 0, 0, cv::INTER_CUBIC);
   fs_img = IplImage(fs_Mat);
}

void fsToggle::update(){	// an embarassment of necessity
   cv::resize(orig_Mat, fs_Mat, cv::Size(width(),height()), 0, 0, cv::INTER_CUBIC);
   fs_img = IplImage(fs_Mat);
}
void fsToggle::setFs(const cv::Mat& fs){fs_img=IplImage(fs_Mat=fs);}

void fsToggle::setFs(const IplImage* fs){
   fs_img=*fs; fs_Mat=cv::cvarrToMat(fs);
}

#if defined(__linux__) || defined(__bsdi__)
#include <signal.h>
void proc_quit(int){throw ErrMsg("",1);} 
#include <X11/Xlib.h>
void fsToggle::getScreenSize()throw(ErrMsg){
   if(Width)return;
   Display *dpy = XOpenDisplay(0);
   if(!dpy)throw ErrMsg("",2);
   Screen* screen = DefaultScreenOfDisplay(dpy);
   Width=WidthOfScreen(screen);
   Height=HeightOfScreen(screen);
   XCloseDisplay(dpy);
}
#elif defined(__WIN32)
#include "wtypes.h"
void fsToggle::getScreenSize()throw(ErrMsg){
   if(Width)return;
   RECT desktop;
   const HWND hDesktop=GetDesktopWindow();
   GetWindowRect(hDesktop, &desktop);
   Width=desktop.right;
   Height=desktop.bottom;
}
#endif

void logfSubs(const char* logf, const char* arg, char* dest){
   char *p3=dest;
   while(true){
	while(*logf && *logf!='%')*dest++=*logf++;
	if(!*logf){
	   *dest=0; dest=p3; return;
	}
	if(*(logf+1)=='f')break;
	if(!*(logf+1)){
	   *dest++='%'; *dest=0; dest=p3; return;
	}
	*dest++=*logf++;
   }
   logf+=2;
   while(*arg && *arg!='.' && *arg!='-')*dest++=*arg++;
   while(*logf)*dest++=*logf++; *dest=0; dest=p3;
}

void cv_procOpt(char*const* optargs, const int16_t& status)throw(ErrMsg,cv::Exception,
	std::exception){
   if(status&0x4){
	loadConf lc(loadConf::defaultConfName,true); return;
   }
   char *confname = optargs[0], *reference = optargs[1], *cmp = optargs[2],
   *main_arg = optargs[3], *diff_n=optargs[4];
   loadConf lc(confname, status&0x4?true:false);
   if(status&0x4)return;
   int conf_int[loadConf::IntCap], pos2=0;	// place holder for configurations
   bool conf_bool[loadConf::BoolCap];
   char conf_char, *conf_str[loadConf::StrCap];
   float conf_float[loadConf::FloatCap], noiseLevel[3];
   Criterion conf_crit[loadConf::CritCap];
   frameRegister::DiffMethod conf_dm; VideoDFT::Transform conf_tr;
   createAnimation::Bgvt conf_bgvt; CvScalar conf_scal;
   lc.get(conf_bool, conf_int, conf_char, conf_str, conf_float, conf_crit, conf_dm,
	   conf_tr, conf_bgvt, conf_scal);
   bool normDiff[]={conf_bool[9],conf_bool[10]};
   conf_bool[7]&=!(status&0x100);	// disable frame-register options for diff-mode
   if(status&0x80){			// gen-video
	const int len=strlen(conf_str[4]);
	const cv::Size sz(512,512);	// default video size when bg image not specified
	IplImage* img=len?cvLoadImage(conf_str[4]):0;
	if(img && conf_bgvt==createAnimation::Blur)
	   createAnimation::fadeDuration=static_cast<int>(conf_float[6]);
	createAnimation *anim;
	if(img && conf_bgvt==createAnimation::Elevator)
	   conf_float[7]=std::min(img->width, img->height)/(conf_int[17]*conf_float[7]);
	createAnimation::setBgPattern(conf_bgvt, conf_bgvt==createAnimation::Elevator ?
		   &conf_float[7]:conf_bgvt==createAnimation::Cappuccino?&conf_float[9]:
		   conf_bgvt==createAnimation::Vapor?&conf_float[15]:0);
	if(conf_bgvt==createAnimation::Cappuccino)createAnimation::setWarpSimp(conf_float[14]);
	anim=strlen(conf_str[4]) ? new createAnimation(conf_str[5], img,
		conf_float[5], conf_int[17]) : new createAnimation(conf_str[5],
		   conf_scal, sz, conf_float[5], conf_int[17]);
	anim->rPopulate(conf_int[11]); anim->factory();
	if(img)cvReleaseImage(&img);
	delete anim; return;
   }
   CvCapture *cap_main=cvCreateFileCapture(main_arg), *cap_sec;
   if(!cap_main){
	sprintf(msg,"Cannot capture main \"%s\"\n", main_arg); throw ErrMsg(msg);
   }
   VideoProp vp_main(cap_main);
   if(status&0x18){	   // prepend/register
	VideoRegister vr(vp_main, main_arg, conf_char);
	int tols[] = {conf_int[13], conf_int[14], conf_int[8]};
	vr.setTol(tols, conf_bool[8]);
	memcpy(noiseLevel, conf_float+2, 3*sizeof(float));
	if(status&0x8){
	   simDropFrame *sd=conf_int[18]? new simDropFrame(vp_main.prop.fcount,
		   conf_float[0], conf_float[1]) : 0;
	   vr.prepend(conf_str[0], conf_int[9], conf_int[16], noiseLevel, sd, conf_int[18]);
	   if(sd)delete sd;
	}
	else{
	   VideoProp vp_sec(cvCreateFileCapture(reference));
	   vr.save(conf_str[1], vp_sec.prop.size);
	}
	return;
   }
   const char* names[]={status&0x101?main_arg:cmp, main_arg}, video1[]="Primary",
	   video2[]="Secondary", *video_str[]={video1, video2};
   if(!(cap_sec=cvCreateFileCapture(status&0x101?main_arg:cmp))){
	sprintf(msg,"Cannot capture secondary \"%s\"\n", status&0x1?main_arg:cmp);
	cvReleaseCapture(&cap_main); throw ErrMsg(msg);
   }
   cvSetCaptureProperty(cap_sec,CV_CAP_PROP_POS_FRAMES, status&0x100?atoi(diff_n):1);
   VideoProp vp_sec(cap_sec), *pvp[]={&vp_main, &vp_sec};
   printf("\"%s\" has %d frames, \"%s\" has %d frames.\n", names[0],
	   vp_main.prop.fcount, names[1], vp_sec.prop.fcount);
   if(status&0x10)swap(vp_main, vp_sec);	// register
   // frame-drop simulation
   simDropFrame *psdf=status&0x1 ? new simDropFrame(vp_main.prop.fcount, conf_float[0],
	   conf_float[1]) : 0;
   const bool* dropSeq=status&0x1 ? psdf->dropArray() : 0;
   IplImage *frame1=cvQueryFrame(cap_main), *frame2=cvQueryFrame(cap_sec),
		*frames[]={frame1, frame2}, *roiMask=0;

   frameSizeEq fse(frames); frameBuffer fb(conf_int[5], &fse);
   frameRegister* pfr = !(status&0x100) && conf_bool[6] ? new frameRegister(names, conf_int[7], conf_dm,
	   conf_tr, conf_crit[0], conf_int[4], dropSeq, normDiff, conf_bool[7], conf_int[12]) : 0;
   Hist hist(&fse, conf_int[3], conf_bool[3], conf_int[0],0, cv::Size(conf_int[1], conf_int[2]));// mask used
   VideoDFT::DftPartitionMethod=conf_bool[11]?VideoDFT::Ring:VideoDFT::Fan;
   VideoDFT vd1(const_cast<IplImage*>(fse.get(true)), conf_tr, conf_int[6], conf_bool[4]),
		vd2(const_cast<IplImage*>(fse.get(false)), conf_tr, conf_int[6], conf_bool[4]),
		*vds[]={&vd1, &vd2};
   myROI* roi=0; VideoCtrlStream* vcs=0;
   if(*conf_str[6]){	// load mask data
	FILE* fp=fopen(conf_str[6],"r");
	if(!fp)
	   fprintf(stderr,"Warning: cannot open mask data file \"%s\" for read.\n", conf_str[6]);
	else try{
	   roi=new myROI(roiMask, frames, video_str, true);
	   roi->read(fp);
	}catch(const ErrMsg& ex){
	   delete roi; delete pfr; delete psdf; throw;
	}
   }
   if(conf_bool[0]){	// silence/disable images for diff-mode
	setIplImage(roiMask=cvCreateImage(cvGetSize(fse.get(true)),
		   fse.get(true)->depth, CV_8UC1));
	vcs->setROI(roi=new myROI(roiMask, frames, video_str, true));
	VideoCtrlStream::pfMouseCB pm=&::MouseCallback;
	cv::namedWindow(video_str[0], CV_WINDOW_AUTOSIZE); // required to know window id to toggle FS
	cv::namedWindow(video_str[1], CV_WINDOW_AUTOSIZE);
	vcs = new VideoCtrlStream(frames,video_str,pvp,&pm,1e3/conf_int[15]);
   }
   ARMA_Array<float> ma1(conf_int[10]), ma2(conf_int[10]), ma3(1),
	*mas[]={&ma1,&ma2,&ma3};
   cvSetCaptureProperty(cap_main,CV_CAP_PROP_POS_FRAMES, vp_main.prop.posFrame=1);
   if(!(status&0x100))
	cvSetCaptureProperty(cap_sec,CV_CAP_PROP_POS_FRAMES, vp_sec.prop.posFrame=1);
   frameUpdater::rmAdjEq=conf_bool[12];
   frameUpdater frmUper(frames, fse, &fb, psdf, pvp, mas, conf_int[12]);
   // substitution for logging file
   char logfn[strlen(conf_str[2])+strlen(main_arg)+1];
   logfSubs(conf_str[2], main_arg, logfn);
//    Logger* logs=new Logger(logfn, pvp, mas, hist, vds, normDiff);	// Note: not deleting logs avoid segfault,
   Logger *logs=new Logger(logfn, pvp, mas, hist, vds, normDiff);	// Note: not deleting logs avoid segfault, don't know why
   Updater up(1, pvp, vds, roi, &hist, &frmUper, *logs, conf_bool[2], conf_bool[1]);
   frameRegister::setHsb(conf_float[20]);
   // End of object creation. Starts to loop.
   char keystroke; bool Esc=false, up2;
#if defined(__linux__) || defined(__bsdi__)
   signal(SIGINT, proc_quit); signal(SIGQUIT, proc_quit);
#endif
   try{
	while(!frmUper.eof() && !Esc) {
	   if(pfr)cvSetCaptureProperty(cap_sec, CV_CAP_PROP_POS_FRAMES, pos2=pfr->reg
		   (vp_main.prop.posFrame, pos2));
	   else{
		up2=false;
		while(dropSeq && dropSeq[vp_main.prop.posFrame])++vp_sec.prop.posFrame, up2=true;
		if(up2)cvSetCaptureProperty(cap_sec, CV_CAP_PROP_POS_FRAMES, vp_sec.prop.posFrame);
	   }
	   if(roi){
		cvShowImage(video_str[0], fse.get(true));
		cvShowImage(video_str[1], fse.get(false));
		do{
		   vcs->kbdEventHandle(static_cast<VideoCtrlStream::keyboard>
			   (keystroke=cvWaitKey(vcs->getConstFrameDelay())));
		   if(Esc=vcs->getEsc())break;
		   switch(keystroke){
			case VideoCtrlStream::PrevFrame:
			case VideoCtrlStream::NextFrame:
			case VideoCtrlStream::Startstop:
			    if(keystroke==VideoCtrlStream::NextFrame)	// update history database
				 vcs->update(vp_main.prop.posFrame,vp_sec.prop.posFrame);
			    up.update(false, vcs->getUpdate());	pos2=0;
			    cvShowImage(video_str[0], fse.get(true));
			    cvShowImage(video_str[1], fse.get(false)); break;
			case VideoCtrlStream::ToggleFs:
			    cvShowImage(video_str[0], vcs->getImg(true));
			    cvShowImage(video_str[1], vcs->getImg(false)); break;
			case VideoCtrlStream::ESC:
			case VideoCtrlStream::Quit: break;
			case VideoCtrlStream::SPACE:
			case VideoCtrlStream::NUL:
							    up.update(!vcs->getFrameDelay());
			    if(vcs->getFrameDelay()){
				 vcs->update(vp_main.prop.posFrame, vp_sec.prop.posFrame);
				 cvShowImage(video_str[0], fse.get(true));
				 cvShowImage(video_str[1], fse.get(false));
			    }
			default:;
		   }
		}while(vcs->getFrameDelay()==0);
	   }
	   else up.update(false);
	}
	printf("%s: %d/%d/%d frame dropped/duplicated.\n", names[1],
		frmUper.getNdrop(1), frmUper.getNdrop(2), frmUper.getNdrop(0));
	summaryPlot(vp_main.prop.fcount, *logs, roi);
	delete psdf; delete pfr; delete roi; delete vcs;
	cvReleaseCapture(&cap_main); cvReleaseCapture(&cap_sec);
   }catch(const ErrMsg& err){
	if(err.errnos()==1){
	   printf("%s: %d/%d/%d frame dropped/duplicated.\n", names[1],
		   frmUper.getNdrop(1), frmUper.getNdrop(2), frmUper.getNdrop(0));
	   summaryPlot(vp_main.prop.fcount, *logs, roi);
	}
	delete psdf; delete pfr; delete roi; delete vcs;
	cvReleaseCapture(&cap_main); cvReleaseCapture(&cap_sec);
	if(err.errnos()!=1)throw;
   }
}

