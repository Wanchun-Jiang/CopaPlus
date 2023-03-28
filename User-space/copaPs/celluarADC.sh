#!/bin/bash
list="taxi bus home timessquare"
for scene in $list
do
	mm-delay 10 mm-link celluar_trace/trace_"$scene" 48mbps_data.trace --uplink-log="uplinkLog/celluar/ADC"$scene".log" --downlink-log="downlinkLog/celluar/ADC"$scene".log" --uplink-queue=droptail --uplink-queue-args="packets=100" <<EOF
	./send_parallel_ADC.sh 10 "OutputLog/celluar/ADC"$scene"_.log"
EOF

done
