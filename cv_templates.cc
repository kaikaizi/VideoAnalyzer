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
#ifdef USE_EXPORT_KEYWORD
export
#endif

#include <list>
#include <deque>
#include <iostream>
#include <iterator>
#include <algorithm>
extern char msg[256];

template<typename T> 
struct isNan{
   const bool operator()(const T&val){return val!=val;}
};
template<typename T> 
struct isInf{
   const bool operator()(const T&val){return !isNan<T>(val) && isNan<T>(val-val);}
};
template<typename T> 
struct isInfOrNan{
   const bool operator()(const T&val){return val!=val || (val-val)!=(val-val);}
};
template<typename T> 
struct Min{
   const T operator()(const T& t1, const T& t2){return t1<t2?t1:t2;}
};
template<typename T> 
struct Sqrt{
   const T operator()(const T& val){return static_cast<T>(sqrt(val));}
};
template<typename T> 
struct Divides{
   Divides(const T& _den):den(_den){}
   const T operator()(const T& num){return static_cast<T>(num/den);}
   private:
   T den;
};
//++++++++++++++++++++++++++++++++++++++++


template<typename T>
simpStat<T>::simpStat(typename T::const_iterator _beg, typename T::const_iterator
	_end):beg(_beg),end(_end),mean_update(false),std_update(false),
   Mean(0),Std(0),state(0){
   copy.assign(beg,end);
   std::remove_if(beg,end, isInfOrNan<long double>());
   count=copy.size();
}

template<typename T>
void simpStat<T>::update(typename T::const_iterator _beg, typename T::const_iterator _end) {
   mean_update = std_update = false; ++state;
   copy.assign(beg=_beg, end=_end);
   std::remove_if(copy.begin(), copy.end(), isInfOrNan<long double>());
   count=copy.size();
}

template<typename T>
void simpStat<T>::update(){
   mean_update=std_update=false; ++state;
   std::copy(beg,end,copy.begin());
   std::remove_if(copy.begin(),copy.end(), isInfOrNan<long double>());
   count=copy.size();
}

template<typename T>void simpStat<T>::dump()const{
   std::copy(copy.begin(), copy.end(), std::ostream_iterator<long double>
	   (std::cout, " "));
}

template<typename T>long double simpStat<T>::mean() {
   if(mean_update) return Mean;
   mean_update = true;
   if(copy.empty())return NAN;
   return Mean=std::accumulate(copy.begin(), copy.end(), 0.)/copy.size();
}

template<typename T>long double simpStat<T>::sd() {
   if(std_update) return Std;
   std_update = true;
   if(!mean_update) mean();
   if(copy.empty() || count<2)return NAN;
   long double sd = 0;
   for(typename T::const_iterator iter=beg; iter!=end; ++iter)
	sd += (*iter-Mean)*(*iter-Mean);
   sd /= (count-1);
   return Std = static_cast<long double>(sqrt(sd));
}

template<typename T>long double simpStat<T>::perc(const float& p) {
   if(copy.empty())return NAN;
   typename std::map<float,std::pair<int,long double>, std::less<float> >::iterator
	iter = Perc.find(p);
   if(iter!=Perc.end() && iter->second.first==state)
	return iter->second.second;
   std::sort(copy.begin(), copy.end());
   long double val = static_cast<long double>(p<0?copy[0] :
	   (p>1?copy.at(count-1):copy.at((count-1)*p)));
   if(iter==Perc.end())
	Perc.insert(std::make_pair(p, std::make_pair(state, val)));
   else{
	iter->second.first=p; iter->second.second=val;
   }
   return val;
}

template<typename T, template<typename,typename>class CONT>
arrayDiff<T,CONT>::arrayDiff(const unsigned& size, const T* arr1, const T* arr2,
	const Criterion& method, const bool& norm):size(size),normalize(norm),b1(),b2(),
   method(method==Default? Correlation : method) {
   memset(diffVal, 0, 3*sizeof(T));
   bin1.reserve(size); bin2.reserve(size);
   for(unsigned indx=0; indx<size; ++indx) {
	bin1.push_back(arr1[indx]); bin2.push_back(arr2[indx]);
   }
   meanNormalize(normalize);
   b1.assign(size, 0); b2.assign(size, 0);
}

template<typename T, template<typename,typename>class CONT>
arrayDiff<T,CONT>::arrayDiff(const ContT& cont1, const ContT& cont2, const Criterion&
     	method, const bool& norm):size(cont1.size()),normalize(norm),b1(),b2(),
   method(method==Default?Correlation:method){
   memset(diffVal, 0, 3*sizeof(T));
   bin1.assign(size, 0); bin2.assign(size, 0);
   std::copy(cont1.begin(), cont1.end(), bin1.begin());
   std::copy(cont2.begin(), cont2.end(), bin2.begin());
   meanNormalize(normalize);
   b1.assign(size, 0); b2.assign(size, 0);
}

template<typename T, template<typename,typename>class CONT>
void arrayDiff<T,CONT>::meanNormalize(const bool& normalize) {
   for(int indx=0; indx<size; ++indx)
	if(isNan<T>()(bin1.at(indx)) || isNan<T>()(bin2.at(indx)))
	   bin1[indx] = bin2[indx] = 0;
   updated[0]=updated[1]=updated[2]=updated[3]=false;
   mean[0] = static_cast<T>(std::accumulate(bin1.begin(), bin1.end(), 0.))/size;
   mean[1] = static_cast<T>(std::accumulate(bin2.begin(), bin2.end(), 0.))/size;
   if(normalize){
	std::transform(bin2.begin(),bin2.end(),bin2.begin(),Divides<T>(mean[0]==0? 1 :
		   mean[1]/mean[0]));
	mean[1] = static_cast<T>(std::accumulate(bin2.begin(), bin2.end(), 0.))/size;
   }
}

template<typename T, template<typename,typename>class CONT>
void arrayDiff<T,CONT>::update(const T* arr1, const T* arr2, const bool& normalize){
   memset(diffVal, 0, 3*sizeof(T));
   bin1.clear();	bin2.clear();
   for(unsigned indx=0; indx<size; ++indx){
	bin1.push_back(arr1[indx]); bin2.push_back(arr2[indx]);
   }
   meanNormalize(normalize);
}

template<typename T, template<typename,typename>class CONT>
void arrayDiff<T,CONT>::update(const ContT& cont1, const ContT& cont2, const bool&
	normalize)throw(ErrMsg){
   if(cont1.size() == cont2.size()){
	bin1.assign(cont1.begin(),cont1.end());
	bin2.assign(cont2.begin(),cont2.end());
   }else{	// partial update
	if(std::max(cont1.size(),cont2.size())>size){
	   sprintf(msg, "arrayDiff::update: cannot handle unequal arguments size [%d, %d]>%d\n",
		   cont1.size(), cont2.size(), size); throw ErrMsg(msg);
	}
	for(int indx=0; indx<cont1.size(); ++indx)bin1[indx]=cont1[indx];
	for(int indx=0; indx<cont2.size(); ++indx)bin2[indx]=cont2[indx];
   }
   meanNormalize(normalize);
}

template<typename T, template<typename,typename>class CONT>
void arrayDiff<T,CONT>::dump()const {
   puts("\n------------arrayDiff------------");
   printf("Container #1(%u):\n", size);
   std::copy(bin1.begin(), bin1.end(), std::ostream_iterator<T>(std::cout, " "));
   std::cout<<"\nMean="<<mean[0]<<std::endl;
   puts("Container #2:");
   std::copy(bin2.begin(), bin2.end(), std::ostream_iterator<T>(std::cout, " "));
   std::cout<<"\nMean="<<mean[1]<<std::endl;
   puts("------------arrayDiff END------------");
}

template<typename T, template<typename,typename>class CONT>
const T arrayDiff<T,CONT>::diff(const Criterion& meth) {
   Criterion cur_meth = meth==Default? method : meth;
   if(updated[meth]) return diffVal[meth];
   T num, den;
   switch(cur_meth) {
	case Correlation:	// sum((H1-mean(H1)).*(H2-mean(H2)))/sqrt{sum([H1-mean(H1)].^2) * sum([H2-mean(H2)].^2)} ---> 1-x
	   b1.assign(size, mean[0]);	b2.assign(size, mean[1]);
	   std::transform(bin1.begin(), bin1.end(), b1.begin(), b1.begin(),
		   std::minus<T>());
	   std::transform(bin2.begin(), bin2.end(), b2.begin(), b2.begin(),
		   std::minus<T>());
	   num = static_cast<T>(std::inner_product(b1.begin(),b1.end(),b2.begin(),0.));
	   std::transform(b1.begin(), b1.end(), b1.begin(), b1.begin(),
		   std::multiplies<T>());
	   std::transform(b2.begin(), b2.end(), b2.begin(), b2.begin(),
		   std::multiplies<T>());
	   den = sqrt(std::accumulate(b1.begin(), b1.end(), 0.)*std::accumulate(
			b2.begin(), b2.end(), 0.));
	   updated[Correlation] = true;
	   return diffVal[Correlation] = den==0||isinf(den)? 0 : 1-num/den;
	case Chi_square:	// sum((H1-H2).^2./(H1+H2))
	   std::transform(bin1.begin(), bin1.end(), bin2.begin(), b1.begin(),
		   std::minus<T>());
	   std::transform(b1.begin(), b1.end(), b1.begin(), b1.begin(),
		   std::multiplies<T>());
	   std::transform(bin1.begin(), bin1.end(), bin2.begin(), b2.begin(), 
		   std::plus<T>());
	   std::replace(b2.begin(), b2.end(), 0, 1);
	   std::transform(b1.begin(), b1.end(), b2.begin(), b1.begin(),
		   std::divides<T>());
	   updated[Chi_square] = true;
	   return diffVal[Chi_square] = std::accumulate(b1.begin(),b1.end(),0)/(mean[0]==0?1:mean[0]*size);
	case Intersection:   // sum(min(H1,H2))/N ---> 1-x
	   std::transform(bin1.begin(), bin1.end(), bin2.begin(), b1.begin(), Min<T>());
	   updated[Intersection] = true;
	   return diffVal[Intersection] = 1-static_cast<T>(std::accumulate(b1.begin(),
			b1.end(), 0.))/(mean[0]==0?1:mean[0]*size);
	case Bhattacharyya:  // sqrt{1-sum(sqrt(H1.*H2))/sqrt(mH1*mH2*N^2)}
	   den = sqrt(mean[0]*mean[1])*size;
	   den = den==0? 1 : den;
	   std::transform(bin1.begin(), bin1.end(), bin2.begin(), b1.begin(),
		   std::multiplies<T>());
	   std::transform(b1.begin(), b1.end(), b1.begin(), Sqrt<T>());
	   num = std::accumulate(b1.begin(), b1.end(), 0.);
	   num = num>den?den:num;
	   updated[Bhattacharyya] = true;
	   return diffVal[Bhattacharyya] = sqrt(1-num/den);
	default: return static_cast<T>(0.);
   }
}

// ++++++++++++++++++++++++++++++++++++++++
// ARMA_Array<T>: y[n] = a_1*x[n]+a_2*x[n-1]+...+a_k*x[n-k] -
// (b_1*y[n-1]+b_2*y[n-2]+...+b_l*y[n-l])

template<typename T>
ARMA_Array<T>::ARMA_Array(const unsigned& _k):k(_k),index_a(0),index_b(0),l(0),
   output_array(0),coef_a(0),coef_b(0),ma_val(0) {
   input_array = new T[k]; memset(input_array, 0, k*sizeof(T));
}

template<typename T>
ARMA_Array<T>::ARMA_Array(const unsigned& _k, const T* as, const unsigned& _l,
	const T* bs):k(_k),l(_l),index_a(0),index_b(0),coef_a(0),coef_b(0),ma_val(0){
   input_array = new T[k]; memset(input_array, 0, k*sizeof(T));
   output_array = new T[l]; memset(output_array, 0, l*sizeof(T));
   if(as) {
	coef_a = new T[k]; memcpy(coef_a, as, k*sizeof(T));
   }
   if(l>0 && !bs) throw ErrMsg("ARMA::ctor: coefficient b's ptr NULL");
   if(bs) {
	coef_b = new T[l]; memcpy(coef_b, bs, l*sizeof(T));
   }
}

template<typename T>
ARMA_Array<T>::ARMA_Array(const ARMA_Array<T>& copy, const bool& state_copy):
   k(copy.k),l(copy.l),index_a(copy.index_a),index_b(copy.index_b),ma_val(0){
   coef_a = new T[k]; memcpy(coef_a, copy.coef_a, k*sizeof(T));
   coef_b = new T[l]; memcpy(coef_b, copy.coef_b, l*sizeof(T));
   input_array = new T[k]; output_array = new T[l];
   if(state_copy) {
	memcpy(input_array, copy.input_array, k*sizeof(T));
	memcpy(output_array, copy.output_array, l*sizeof(T));
   } else{	   // zero-sate
	index_a=index_b=0;
	memset(input_array, 0, k*sizeof(T));
	memset(output_array, 0, l*sizeof(T));
   }
}

template<typename T>ARMA_Array<T>::~ARMA_Array() {
   delete[] input_array; delete[] output_array;
   delete[] coef_a; delete[] coef_b;
}

template<typename T>void ARMA_Array<T>::clear(){
   memset(input_array, 0, k*sizeof(T));
   memset(output_array, 0, l*sizeof(T));
}

template<typename T>void ARMA_Array<T>::dump()const {
   puts("-------------ARMA::dump-------------");
   puts("Coef a's:");
   if(coef_a)
	std::copy(coef_a, coef_a+k, std::ostream_iterator<T>(std::cout," "));
   else
	std::cout<<k<<" repetitions of "<<static_cast<T>(1.)/k;
   if(coef_b){
	puts("\nCoef b's:");
	std::copy(coef_b, coef_b+l, std::ostream_iterator<T>(std::cout," "));
   }
   // current input/output states
   puts("\nInput states:");
   std::copy(input_array, input_array+k, std::ostream_iterator<T>(std::cout," "));
   if(output_array) {
	puts("Output states:");
	std::copy(output_array, output_array+k, std::ostream_iterator<T>(std::cout," "));
   }
   puts("\n-------------ARMA-------------");
}

template<typename T>const T ARMA_Array<T>::append(const T& val) {
   input_array[index_a++]=val;
   if(index_a >= k) index_a=0;	   // roll back
   ma_val=0;
   for(int index=0; index<k; ++index)
	ma_val += input_array[(index_a+index+1)%k]*(coef_a?coef_a[index]:1./k);
   for(int index=0; index<l; ++index)
	ma_val -= output_array[(index_b+index+1)%l]*(coef_b?coef_b[index]:1./l);
   if(output_array) output_array[index_b++] = ma_val;
   if(index_b >= l) index_b = 0;
   return ma_val;
}

template<typename T>
void ARMA_Array<T>::append(const unsigned& len, const T* src, T* dest){
   // dest have (at least) same size as src
   for(unsigned index=0; index<len; ++index)dest[index] = append(src[index]);
}

//========================================

template<typename T>
drawBars<T>::drawBars(const cv::Size& cvsSize):size(cvsSize) {
   canvas.create(size, CV_8UC1);
}

template<typename T>
cv::Mat& drawBars<T>::draw(const T*const data[2], const int& bins){
   nbins = bins;
   data1.clear(); data2.clear();
   for(int indx=0; indx<nbins; ++indx) {
	data1.push_back(data[0][indx]); data2.push_back(data[1][indx]);
   }
   return draw();
}

template<typename T>
cv::Mat& drawBars<T>::draw(const T& bin1, const T& bin2) {
   data1.clear();	   data2.clear();
   nbins = bin1.size();
   data1.assign(nbins,0); data2.assign(nbins, 0);
   std::copy(bin1.begin(), bin1.end(), data1.begin());
   std::copy(bin2.begin(), bin2.end(), data2.begin());
   return draw();
}

template<typename T>cv::Mat& drawBars<T>::draw(){   // no thresholding provided
   double minElem = data1.at(0), maxElem = minElem;
   for(int indx=0; indx<nbins; ++indx) {
	if(minElem > data1.at(indx)) minElem = data1[indx];
	if(minElem > data2.at(indx)) minElem = data2[indx];
	if(maxElem < data1.at(indx)) maxElem = data1[indx];
	if(maxElem < data2.at(indx)) maxElem = data2[indx];
   }
   const int &width = size.width, &height = size.height,
	x_inc = static_cast<int>((width+0.)/nbins), binWidth = x_inc/2-1;
   const double ratio = height/(maxElem-minElem+1e-15);
   int x_indx;
   canvas = cv::Scalar(0);
   for(int indx=x_indx=0; indx<nbins; ++indx, x_indx+=x_inc) {
	int curh0 = (data1[indx]-minElem)*ratio, curh1 = (data2[indx]-minElem)*ratio;
	cv::rectangle(canvas, cv::Point(x_indx, height), cv::Point(x_indx+binWidth,
		   height-curh0), 255, CV_FILLED);
	cv::rectangle(canvas, cv::Point(x_indx+binWidth+1, height),
		cv::Point(x_indx+2*binWidth+1, height-curh1), 125, CV_FILLED);
   }
   return canvas;
}
