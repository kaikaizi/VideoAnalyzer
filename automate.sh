#!/bin/sh
if [ $# -lt 1 ]; then
   echo 'Usage:' $0 'server_ip [clean]'
   exit 1
elif ! ping -q -c1 $1; then
   echo Remote server unreachable.
   exit 2
fi
dir=~/rtsp_record/
[ $# -gt 1 ] && rm -f ${dir}*.avi ${dir}ping.info
tm=`date +%m-%d-%H%M%S`
connect=eth;
ifconfig eth0 | fgrep -q 'inet ' || connect=wlan;
# record, save to 6 local files:
# 0: demo10-$tm-$connect.avi
# 1: demo2-$tm-$connect.avi
# ...
declare -a targets
targets=(320x240x128k_10FPS_5KF~ 320x240x256k_10FPS_5KF~ 320x240x384k_10FPS_5KF~ 640x480x128k_10FPS_5KF~ 640x480x256k_10FPS_5KF~ 640x480x384k_10FPS_5KF~ AVI-Demo-1~ AVI-Demo-2~ AVI-Demo-3~ A_10_original~ A_15_original~ B_10_original~ B_15_original~ C_30_original~ D_30_original~ T3000-Sample-Video4~ Take2_sample_640x480x128k~ Take2_sample_640x480x192k~ Take2_sample_640x480x256k~
Take2_sample_640x480x384k~)
id=0
echo $connect `date` >> $dir/ping.info
for t in ${targets[@]}; do
   ((id++))
   if ~/Documents/Code/cv/VideoAnalyzer --as-client rtsp://$1:7654/test$id $dir${t/%~/-$tm-$connect.avi}; then
	ping -q -c20 $1 | sed -n '4,5p' >> $dir\ping.info
   else
	echo 'Warning: acceptor failed'
   fi
done
