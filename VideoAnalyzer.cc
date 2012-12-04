#include "conf.hh"
#define ARGLEN 64

int main(int argc, char *argv[])
{
//    DtTest(); return 0;
   try{
	char str0[ARGLEN], str1[ARGLEN], str2[ARGLEN], str3[ARGLEN], str4[ARGLEN],
	     *opt_args[]={str0, str1, str2, str3, str4};
	int16_t status;
	int opt_status=cv_getopt(argc, argv, opt_args, status);
	if(opt_status)cv_procOpt(opt_args, status);
   }catch(const ErrMsg& ex){
	fprintf(stderr,"ErrMsg caught:\n\t%s\n", ex.what());
   }catch(const cv::Exception& ex){
	fprintf(stderr,"cv::Exception:\n\t%s\n", ex.what());
   }catch(const std::exception& ex){
	fprintf(stderr,"std::exception:\n\t%s\n", ex.what());
   }catch(...){
	fputs("Unknown exception:\n",stderr);
   }
   return 0;
}
