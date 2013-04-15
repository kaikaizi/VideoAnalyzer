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

#if defined(USE_EXPORT)
#define EXPORT export
#else
#define EXPORT
#endif

#include <vector>
#include "sketch.hh"
#include "hist.hh"

/**
* @brief Calculates simple statistics for a sequential
* container. Missing values inf/nan are removed.
* @tparam T std container
*/
EXPORT template<typename T>
class simpStat {
public:
/**
* @brief Associates to a sequential container
* @param beg Start position of range for stats
* @param end End position of range for stats
*/
   simpStat(typename T::const_iterator beg, typename T::const_iterator end);
   simpStat():copy(),count(0){}
/**
* @name update
* @brief Notify that container (or its elems) has changed
* since last calculation.
* @{ */
   void update(typename T::const_iterator beg, typename T::const_iterator end);
   void update();
/**  @} */
/**
* @name calculates simple stats
* @{ */
   long double mean();
   long double sd();
   long double perc(const float& p=.5);
/**  @} */
   void dump()const;
private:
   std::vector<long double> copy;
   typename T::const_iterator beg,end;
   bool mean_update, std_update;	// are the stats updated?
/**
* @brief <p, <state, percentile(p)> >, used for multiple
* possible percentile values
*/
   std::map<float,std::pair<int,long double>, std::less<float> > Perc;
   long double Mean, Std;
   int state, count;
};

/**
* @brief Tester for simpleStat template.
*/
void meanSdTester();

/**
* @brief Quantifies difference between two arrays using
* specified criterion
* @tparam T numerical type of the C-style arrays
* @tparam CONT container type used in ctor
*/
EXPORT template<typename T, template<typename ELEM, typename
ALLOC=std::allocator<ELEM> >class CONT=std::vector>
class arrayDiff{
public:
   typedef CONT<T> ContT;

/**
* @name Ctors
* @{ */
/**
* @brief Make local copies of two C-style arrays or
* container types and sets states.
* @param size lengths of the two arrays
* @param arr1 First array
* @param arr2 Second array
* @param method Criterion:
* Correlation/Chi_squre/Intersection/Bhattacharyya.
* @param normalize Do I treat the mean value of two arrays
* equal?
*/
   arrayDiff(const unsigned& size, const T* arr1, const T* arr2, const Criterion&
	   method=Correlation, const bool& normalize=false);
   arrayDiff(const ContT& cont1, const ContT& cont2, const Criterion&
	   method=Correlation, const bool& normalize=false);
/**  @} */

/**
* @name update
* @brief Force update container contents
* @{ */
   void update(const T* arr1, const T* arr2, const bool& normalize=false);
   void update(const CONT<T>& arr1, const CONT<T>& arr2,const bool&
	   normalize=false)throw(ErrMsg);
/**  @} */

/**
* @brief Prints container elements (normalized if specified)
* and mean value
*/
   void dump()const;

/**
* @brief Calculates difference between two vectors
* @param norm Normalize?
* @param meth method
* @return difference
*/
   const T diff(const Criterion& meth=Default);
private:
   std::vector<T> bin1, bin2, b1, b2;
   T diffVal[4], mean[2];
   bool updated[4], normalize;
   unsigned size;
   Criterion  method;

   void meanNormalize(const bool&);
   // NOTE: only Intersection method is sensitive to scaling
};

/**
* @brief Auto-regressive moving-average filtering of a
* vector: y[n] = a_1*x[n]+a_2*x[n-1]+...+a_k*x[n-k] -
* (b_1*y[n-1]+b_2*y[n-2]+...+b_l*y[n-l])
* @tparam T numerical type
*/
EXPORT template<typename T>
class ARMA_Array {
public:
/**
* @name Ctors
* @{
* @param _k Moving-average filter length (equivalent to
* averaging of _k data points)
*/
   explicit ARMA_Array(const unsigned& _k);
/** @param _k Moving-average filter length 
* @param _as Optional MA coefficients
* @param _l Auto-regressive filter length
* @param _bs AR coefficients
*/
   ARMA_Array(const unsigned& _k, const T* _as, const unsigned& _l,
	   const T* _bs);
/** @param copy Copy of another object
* @param state_copy Do I need to keep filter states same as
* copy, or reset state?
*/
   ARMA_Array(const ARMA_Array<T>& copy, const bool& state_copy);
   ~ARMA_Array();
/**  @} */

/** @brief Print ARMA filter coefficents (a/b) and
* intput/outpu states 
*/
   void dump()const;

/**
* @name append
* @{
* @brief Append a new data point and updates filter state/output
* @param val element to append to inputs
* @return filter output
*/
   const T append(const T& val);
/**
* @brief Append a vector of "len" elements and gets batch
* outputs
* @param len Number of elements in the batch of data
* @param src Source data to read from
* @param dest Destination for outputs. Have same buffer size
* (len) as source.
*/
   void append(const unsigned& len, const T* src, T* dest);
/**  @} */

/** @name retrieves model order
* @{ */
   const unsigned& get_k()const{return k;}
   const unsigned& get_l()const{return l;}
/**  @} */
   const T& get_val()const{return ma_val;}
   void clear();
private:
   T *input_array, *output_array, *coef_a, *coef_b, ma_val;
   const unsigned k, l;
   unsigned index_a, index_b;
};

/**
* @brief Gloabal method with similar functionality as
* Hist::draw, showing two histograms in a canvass
* @tparam T Numerical data type 
*/
EXPORT template<typename T>
class drawBars {
public:
/**
* @brief Allocate canvass for histogram display
*
* @param cvsSize Size of canvass
*/
   explicit drawBars(const cv::Size& cvsSize);

/**
* @name draw
* @{ */
/**
* @brief Draw histogram from C-style data arrays
* @param data[2] Two vectors
* @param nbins Number of bins in histogram
* @return updated histogram canvass
*/
   cv::Mat& draw(const T* const data[2], const int& nbins);
/**
* @brief Draw histogram from two container types. Users
* ensure that T is of STL container-type
* @param bin1 First container
* @param bin2 Second container
* @return updated histogram canvass
*/
   cv::Mat& draw(const T& bin1, const T& bin2);
/**  @} */
private:
   cv::Mat canvas;
   const cv::Size size;
   int nbins;
   std::vector<double> data1, data2;
   cv::Mat& draw();
};

#ifndef USE_EXPORT
#include "cv_templates.cc"
#endif
