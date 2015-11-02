#!/bin/sh

sub="·þÎñÆ÷¾¯±¨"
msg=

mailto()
{
	python sendmail.py "$sub" "$msg"
}

server="/ssddata/collaborative_filtering/primaryElection/bin/primary_election_test"
date=$(date)
localip=$(ifconfig bond0 | grep 'inet addr' | awk '{print $2}' | cut -f2 -d:)
cnt=0

while :
do
	stat=$(ps aux | grep $server | grep -v 'grep' | awk '{print $8}')
	if [ $stat!="Sl+" ]
	then
		cnt=`expr $cnt + 1`
		sleep 0.1
	else
		cnt=0
		sleep 30
	fi
	if ((cnt >= 3000 && cnt % 300 == 0))
	then
		msg=''$date':'$localip', no cpu used '$(echo $((cnt/10)))'s'
		mailto
	fi
done

