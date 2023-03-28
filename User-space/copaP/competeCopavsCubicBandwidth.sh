#!/bin/bash

delay=$(awk 'BEGIN{print '$2'/2}')

kill -9 $(ps -ef | awk '/receiver/{print $2}' )
kill -9 $(ps -ef | awk '/iperf/{print $2}' )
sleep 2s
./../receiver &
iperf -Z cubic -s &

bandwidthRatio=$(($3/12))

echo $bandwidthRatio
#delay=30
queueLen=$(awk -v rtt=$delay 'BEGIN{print '$1'*'$2'*'$bandwidthRatio'}')

echo $queueLen

mm-delay $delay  mm-link ../"$3"mbps_data.trace ../"$3"mbps_data.trace --uplink-log uplinkLog/compete/Copa"$1"_$2.log --uplink-queue=droptail --uplink-queue-args="packets=$queueLen" <<EOF
			echo start $MAHIMAHI_BASE
			./../sender serverip=100.64.0.1 offduration=0 onduration=30000 \
			cctype=markovian delta_conf=do_ss:auto:0.1 traffic_params=deterministic,num_cycles=1 > outputGraph/competeCopa/Bandwidth/"$3"mbps/Copa"$1"_"$2"_"$3"mbps_1.log 2>&1 &
			./../sender serverip=100.64.0.1 offduration=0 onduration=30000 \
			cctype=markovian delta_conf=do_ss:auto:0.1 traffic_params=deterministic,num_cycles=1 > outputGraph/competeCopa/Bandwidth/"$3"mbps/Copa"$1"_"$2"_"$3"mbps_2.log 2>&1 &
			./../sender serverip=100.64.0.1 offduration=0 onduration=30000 \
			cctype=markovian delta_conf=do_ss:auto:0.1 traffic_params=deterministic,num_cycles=1 > outputGraph/competeCopa/Bandwidth/"$3"mbps/Copa"$1"_"$2"_"$3"mbps_3.log 2>&1 &
			#sleep 10s
			iperf -Z cubic -c 100.64.0.1 -t 30 -i 0.1 -P 3 > outputGraph/competeCopa/Bandwidth/"$3"mbps/Cubic"$1"_"$2"_"$3"mbps.log 2>&1
			wait

EOF
