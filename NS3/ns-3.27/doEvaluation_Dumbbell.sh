
dir="Copa+"
subdir="noise"
endtime=10


#list="1 2 5 10 20"
list="100"


for loop in $list #1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 #4 8 12 16 20 24 28 32 #36 40 44 48 #20 30 40 # 12 14 16 18 20 22 24 26 #2 4 6 8 10 20 30 40 #2 4 6 8 10 20 30 40 50 60 70 80 90 100 110 120 130 140 150 160 
do
postfix=$loop
./waf --run "periodcCCEvaDumbbell --runtime=$endtime --flowNum=1 --delayRB=20 --BW=$loop --Bd=10 --Rc=30 --lamuda=1 --postfix=$postfix --rand_loss=false --rand_BW=false --rand_delay=false --rand_interval=5 --queuesize=200 --cycle=0.5 --CC_mode=5"  > Testdata/Evaluation/"$dir"/output"$postfix".txt 2>&1 \
&& \
# cd Testdata/Evaluation
# echo seperate
# bash doseprate.sh $postfix $postfix $dir\
# && \
# echo cat throughput
# bash doAllThr.sh $postfix $postfix $dir 0.5\
# && \
# echo cat Ideal
# bash docatIdeal.sh $postfix 5 $dir\
# && \
# echo paint throughput
# bash plotThrAnalyse.sh $postfix $postfix $dir\

# echo paint other
# bash plotCwndAnalyse.sh $postfix $postfix $dir $endtime


#rm -rf "$dir"/"$subdir"

mkdir Testdata/Evaluation/$dir/$subdir

#mv "$dir"/flows$loop "$dir"/"$subdir"
mv Testdata/Evaluation/"$dir"/output"$postfix".txt Testdata/Evaluation/"$dir"/"$subdir"
mv Testdata/Evaluation/"$dir"/PDCC.q Testdata/Evaluation/"$dir"/"$subdir"/"$dir"_"$postfix".q
mv Testdata/Evaluation/"$dir"/PDCC.tr Testdata/Evaluation/"$dir"/"$subdir"/"$dir"_"$postfix".tr

done
