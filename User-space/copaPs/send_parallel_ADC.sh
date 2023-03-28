#!/bin/bash


flow_num=$1
#IPP="192.168.100.5"
IPP=$MAHIMAHI_BASE

delta=(0 0.4 1)
#upper--10pkts QOE_factor: 2 4 5 10

for ((index=1;index<=$flow_num;index++))
do
	{
		./sender serverip=$IPP offduration=0 onduration=60000 \
		cctype=adc delta_conf=do_ss:auto:0.5 traffic_params=deterministic,num_cycles=1 > $2"$index".log

	}&
done
wait
#constant_delta:
#${delta[$index]}

