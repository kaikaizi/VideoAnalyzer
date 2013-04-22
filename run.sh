#!/bin/sh

# compares original video w. transmitted ones.
# NOTE: in conf file, reset Behavior.FR.rmAdjacentEq and Constraint.FRInc
trap 'pkill run && pkill VideoAnalyzer' 1 2
declare -a files matches
files=`ls -r *.avi | fgrep -v _prep`
cd ..
# rm -f rtsp_record/*.log
count=0; nParalell=6
for f in ${files[@]}; do
   matches=`ls rtsp_record/${f%%.avi}*.avi`
   for m in ${matches[@]}; do
	fl=rtsp_record/`echo $m | sed 's:.*/\|\.avi::g'`.log
	[ ! -f $fl ] && ./VideoAnalyzer -c $m ultra2/$f > $fl &
	((count+=1))
	[ $count -ge $nParalell ] && wait && count=0
   done
   wait
done
# sudo systemctl -f start poweroff.target
