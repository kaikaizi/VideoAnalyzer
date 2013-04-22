da1fn=list.files(path='.',pattern='320x240x128k.*.log')
da2fn=list.files(path='.',pattern='320x240x256k.*.log')
da3fn=list.files(path='.',pattern='320x240x384k.*.log')
da4fn=list.files(path='.',pattern='640x480x128k.*.log')
da5fn=list.files(path='.',pattern='640x480x256k.*.log')
da6fn=list.files(path='.',pattern='640x480x384k_.*.log')
da7fn=list.files(path='.',pattern='AVI-Demo-1.*.log')
da8fn=list.files(path='.',pattern='AVI-Demo-2.*.log')
da9fn=list.files(path='.',pattern='AVI-Demo-3.*.log')
daAfn=list.files(path='.',pattern='A_10.*.log')
daBfn=list.files(path='.',pattern='B_10.*.log')
daCfn=list.files(path='.',pattern='B_15.*.log')
daDfn=list.files(path='.',pattern='C_30.*.log')
daEfn=list.files(path='.',pattern='D_30.*.log')
daFfn=list.files(path='.',pattern='T3000.*.log')
daGfn=list.files(path='.',pattern='Take2_sample_640x480x128k.*.log')
daHfn=list.files(path='.',pattern='Take2_sample_640x480x192k.*.log')
daIfn=list.files(path='.',pattern='Take2_sample_640x480x256k.*.log')
daJfn=list.files(path='.',pattern='Take2_sample_640x480x384k.*.log')
dafn=list(d1=da1fn,d2=da2fn,d3=da3fn,d4=da4fn,d5=da5fn,d6=da6fn,d7=da7fn,d8=da8fn,
	    d9=da9fn,dA=daAfn,dB=daBfn,dC=daCfn,dD=daDfn,dE=daEfn,dF=daFfn,dG=daGfn,
	    dH=daHfn,dI=daIfn,dJ=daJfn)
indx_datagrp=1
for(dx in dafn){
   indx_eth=1; indx_wlan=1
   for(fx in dx){	# bad reads are ignored
	isEth=length(grep('eth',fx))
	fn=ifelse(isEth, paste('da',indx_datagrp,'e',indx_eth,sep=''),
		    paste('da',indx_datagrp,'w',indx_wlan,sep=''))
	if(isEth)indx_eth=indx_eth+1
	else indx_wlan=indx_wlan+1
	assign(fn,try(read.table(file=fx,header=T),silent=T))
   }
   indx_datagrp=indx_datagrp+1
}
# hierarchical, stupid hierarchy: fields da1~daJ; subfields wlan,eth; sub-subfields:s1~s10
Data = list(da1=list(wlan=list(s1=da1w1,s2=da1w2,s3=da1w3,s4=da1w4,s5=da1w5,s6=da1w6,s7=da1w7,s8=da1w8,s9=da1w9,sA=da1w10),eth=list(s1=da1e1,s2=da1e2,s3=da1e3,s4=da1e4,s5=da1e5,s6=da1e6,s7=da1e7,s8=da1e8,s9=da1e9,sA=da1e10)),
		da2=list(wlan=list(s1=da2w1,s2=da2w2,s3=da2w3,s4=da2w4,s5=da2w5,s6=da2w6,s7=da2w7,s8=da2w8,s9=da2w9,sA=da2w10),eth=list(s1=da2e1,s2=da2e2,s3=da2e3,s4=da2e4,s5=da2e5,s6=da2e6,s7=da2e7,s8=da2e8,s9=da2e9,sA=da2e10)),
		da3=list(wlan=list(s1=da3w1,s2=da3w2,s3=da3w3,s4=da3w4,s5=da3w5,s6=da3w6,s7=da3w7,s8=da3w8,s9=da3w9,sA=da3w10),eth=list(s1=da3e1,s2=da3e2,s3=da3e3,s4=da3e4,s5=da3e5,s6=da3e6,s7=da3e7,s8=da3e8,s9=da3e9,sA=da3e10)),
		da4=list(wlan=list(s1=da4w1,s2=da4w2,s3=da4w3,s4=da4w4,s5=da4w5,s6=da4w6,s7=da4w7,s8=da4w8,s9=da4w9,sA=da4w10),eth=list(s1=da4e1,s2=da4e2,s3=da4e3,s4=da4e4,s5=da4e5,s6=da4e6,s7=da4e7,s8=da4e8,s9=da4e9,sA=da4e10)),
		da5=list(wlan=list(s1=da5w1,s2=da5w2,s3=da5w3,s4=da5w4,s5=da5w5,s6=da5w6,s7=da5w7,s8=da5w8,s9=da5w9,sA=da5w10),eth=list(s1=da5e1,s2=da5e2,s3=da5e3,s4=da5e4,s5=da5e5,s6=da5e6,s7=da5e7,s8=da5e8,s9=da5e9,sA=da5e10)),
		da6=list(wlan=list(s1=da6w1,s2=da6w2,s3=da6w3,s4=da6w4,s5=da6w5,s6=da6w6,s7=da6w7,s8=da6w8,s9=da6w9,sA=da6w10),eth=list(s1=da6e1,s2=da6e2,s3=da6e3,s4=da6e4,s5=da6e5,s6=da6e6,s7=da6e7,s8=da6e8,s9=da6e9,sA=da6e10)),
		da7=list(wlan=list(s1=da7w1,s2=da7w2,s3=da7w3,s4=da7w4,s5=da7w5,s6=da7w6,s7=da7w7,s8=da7w8,s9=da7w9,sA=da7w10),eth=list(s1=da7e1,s2=da7e2,s3=da7e3,s4=da7e4,s5=da7e5,s6=da7e6,s7=da7e7,s8=da7e8,s9=da7e9,sA=da7e10)),
		da8=list(wlan=list(s1=da8w1,s2=da8w2,s3=da8w3,s4=da8w4,s5=da8w5,s6=da8w6,s7=da8w7,s8=da8w8,s9=da8w9,sA=da8w10),eth=list(s1=da8e1,s2=da8e2,s3=da8e3,s4=da8e4,s5=da8e5,s6=da8e6,s7=da8e7,s8=da8e8,s9=da8e9,sA=da8e10)),
		da9=list(wlan=list(s1=da9w1,s2=da9w2,s3=da9w3,s4=da9w4,s5=da9w5,s6=da9w6,s7=da9w7,s8=da9w8,s9=da9w9,sA=da9w10),eth=list(s1=da9e1,s2=da9e2,s3=da9e3,s4=da9e4,s5=da9e5,s6=da9e6,s7=da9e7,s8=da9e8,s9=da9e9,sA=da9e10)),
		daA=list(wlan=list(s1=da10w1,s2=da10w2,s3=da10w3,s4=da10w4,s5=da10w5,s6=da10w6,s7=da10w7,s8=da10w8,s9=da10w9,sA=da10w10),eth=list(s1=da10e1,s2=da10e2,s3=da10e3,s4=da10e4,s5=da10e5,s6=da10e6,s7=da10e7,s8=da10e8,s9=da10e9,sA=da10e10)),
		daB=list(wlan=list(s1=da11w1,s2=da11w2,s3=da11w3,s4=da11w4,s5=da11w5,s6=da11w6,s7=da11w7,s8=da11w8,s9=da11w9,sA=da11w10),eth=list(s1=da11e1,s2=da11e2,s3=da11e3,s4=da11e4,s5=da11e5,s6=da11e6,s7=da11e7,s8=da11e8,s9=da11e9,sA=da11e10)),
		daC=list(wlan=list(s1=da12w1,s2=da12w2,s3=da12w3,s4=da12w4,s5=da12w5,s6=da12w6,s7=da12w7,s8=da12w8,s9=da12w9,sA=da12w10),eth=list(s1=da12e1,s2=da12e2,s3=da12e3,s4=da12e4,s5=da12e5,s6=da12e6,s7=da12e7,s8=da12e8,s9=da12e9,sA=da12e10)),
		daD=list(wlan=list(s1=da13w1,s2=da13w2,s3=da13w3,s4=da13w4,s5=da13w5,s6=da13w6,s7=da13w7,s8=da13w8,s9=da13w9,sA=da13w10),eth=list(s1=da13e1,s2=da13e2,s3=da13e3,s4=da13e4,s5=da13e5,s6=da13e6,s7=da13e7,s8=da13e8,s9=da13e9,sA=da13e10)),
		daE=list(wlan=list(s1=da14w1,s2=da14w2,s3=da14w3,s4=da14w4,s5=da14w5,s6=da14w6,s7=da14w7,s8=da14w8,s9=da14w9,sA=da14w10),eth=list(s1=da14e1,s2=da14e2,s3=da14e3,s4=da14e4,s5=da14e5,s6=da14e6,s7=da14e7,s8=da14e8,s9=da14e9,sA=da14e10)),
		daF=list(wlan=list(s1=da15w1,s2=da15w2,s3=da15w3,s4=da15w4,s5=da15w5,s6=da15w6,s7=da15w7,s8=da15w8,s9=da15w9,sA=da15w10),eth=list(s1=da15e1,s2=da15e2,s3=da15e3,s4=da15e4,s5=da15e5,s6=da15e6,s7=da15e7,s8=da15e8,s9=da15e9,sA=da15e10)),
		daG=list(wlan=list(s1=da16w1,s2=da16w2,s3=da16w3,s4=da16w4,s5=da16w5,s6=da16w6,s7=da16w7,s8=da16w8,s9=da16w9,sA=da16w10),eth=list(s1=da16e1,s2=da16e2,s3=da16e3,s4=da16e4,s5=da16e5,s6=da16e6,s7=da16e7,s8=da16e8,s9=da16e9,sA=da16e10)),
		daH=list(wlan=list(s1=da17w1,s2=da17w2,s3=da17w3,s4=da17w4,s5=da17w5,s6=da17w6,s7=da17w7,s8=da17w8,s9=da17w9,sA=da17w10),eth=list(s1=da17e1,s2=da17e2,s3=da17e3,s4=da17e4,s5=da17e5,s6=da17e6,s7=da17e7,s8=da17e8,s9=da17e9,sA=da17e10)),
		daI=list(wlan=list(s1=da18w1,s2=da18w2,s3=da18w3,s4=da18w4,s5=da18w5,s6=da18w6,s7=da18w7,s8=da18w8,s9=da18w9,sA=da18w10),eth=list(s1=da18e1,s2=da18e2,s3=da18e3,s4=da18e4,s5=da18e5,s6=da18e6,s7=da18e7,s8=da18e8,s9=da18e9,sA=da18e10)),
		daJ=list(wlan=list(s1=da19w1,s2=da19w2,s3=da19w3,s4=da19w4,s5=da19w5,s6=da19w6,s7=da19w7,s8=da19w8,s9=da19w9,sA=da19w10),eth=list(s1=da19e1,s2=da19e2,s3=da19e3,s4=da19e4,s5=da19e5,s6=da19e6,s7=da19e7,s8=da19e8,s9=da19e9,sA=da19e10)))
rm(list=ls(pattern='d'))  # free some up
# Fields are: Index1, Index2, Dynmic1, Dynmic2, Difference, Hist_Corr, Hist_Chisq, Hist_Inter, Hist_Bhatta, DT_Corr, DT_Chisq, DT_Inter, DT_Bhatta

nInvalidEntries=array(0,2); nSigs=array(0,5); indx_datagrp=1
for(d in Data){ # each entry is the median of streamed median(|frame index offsets|)
#    par(mfrow=c(2,1), cex.lab=1.3, cex.main=1.5, font.lab=4)
   index=0; buf=matrix(nrow=2,ncol=10)
   rmse1={}; rmse2={}; histCorr1={}; histCorr2={}; dtCorr1={}; dtCorr2={}
   for(dd in d$wlan){
	if(!is.null(names(dd))){
	   if(index==0)
		plot(dd$Index2,dd$Index2-dd$Index1, type='s', col='red',
		     xlab='Transmitted Frame Index', ylab='Received Frame Index',
		     main=names(data)[index], xlim=c(1,200), ylim=c(-80,80),lwd=2)
	   else
		lines(dd$Index1, dd$Index2-dd$Index1, type='s', col='red',lwd=2)
	   index=index+1
	   buf[1,index]=median(abs(dd$Index1-dd$Index2))
	   rmse1=c(rmse1,dd$Diff)
	   histCorr1=c(histCorr1,dd$Hist_Corr)
	   dtCorr1=c(dtCorr1,dd$DT_Corr)
	}else nInvalidEntries[1]=nInvalidEntries[1]+1
   }
   index=0
   for(dd in d$eth){
	if(!is.null(names(dd))){
	   lines(dd$Index2, dd$Index1-dd$Index2, type='s', col='black',lwd=2)
	   index=index+1
	   buf[2,index]=median(abs(dd$Index1-dd$Index2))
	   rmse2=c(rmse1,dd$Diff)
	   histCorr2=c(histCorr2,dd$Hist_Corr)
	   dtCorr2=c(dtCorr2,dd$DT_Corr)
	}else nInvalidEntries[2]=nInvalidEntries[2]+1
   }
   # binomial test NULL: wireless offsets's greater than wired version
   # paired t-test alternative: wireless's larger than wired
   print(buf)
   pVals=c(binom.test(length(which(buf[1,]>buf[2,])),length(buf),alternative='g')['p.value']$p.value,
	t.test(buf[1,],buf[2,],paired=T,alternative='g')['p.value']$p.value,
	t.test(rmse1,rmse2,alternative='g')['p.value']$p.value,
	t.test(histCorr1,histCorr2,alternative='g')['p.value']$p.value,
	t.test(dtCorr1,dtCorr2,alternative='g')['p.value']$p.value)
   pVals[is.nan(pVals)]=1
   index=0
   for(x in pVals){
   	if(x<.05)nSigs[index]=nSigs[index]+1
   	index=index+1
   }
   print(indx_datagrp); print(pVals)
   indx_datagrp=indx_datagrp+1
#    readline()
}
print(nSigs)

# 2nd figure showing historgram differences
# png('hist.png', width=1280, height=1024, units='px', pointsize=12, bg='white')
# par(mfrow=c(2,2), cex.lab=1.3, cex.main=1.5, font.lab=4)
# dev.off()
