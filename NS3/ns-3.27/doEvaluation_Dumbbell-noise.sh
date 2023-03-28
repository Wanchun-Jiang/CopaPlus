
endtime=60
atk_Time=0
atk_Exit=60
C=100
RTT=20
N=30

CC_mode=$1
atk_mode=$CC_mode

test_mod=$2

if [ $CC_mode == 1 ]; then 
	dir="Copa+"
fi

if [ $CC_mode == 2 ]; then 
	dir="Copa"
fi

if [ $CC_mode == 3 ]; then 
	dir="BBR"
fi

if [ $CC_mode == 4 ]; then 
	dir="Copa+-origin"
fi

if [ $CC_mode == 5 ]; then 
	dir="Cubic"
fi
if [ $CC_mode == 7 ]; then 
	dir="Copa+-strawman"
fi

if [ $test_mod == "C" ]; then 
	subdir=noise-C
	list="60 140"
fi

if [ $test_mod == "R" ]; then 
	subdir=noise-R
	list="12 32"
fi

if [ $test_mod == "N" ]; then 
	subdir=noise-N
	list="5 40"
fi

mkdir ../Testdata/$dir/$subdir
#mkdir ../Testdata/$dir/$subdir/F1-5-12-2

for loop in $list #4 8 12 16 20 24 28 32 #36 40 44 48 #20 30 40 # 12 14 16 18 20 22 24 26 #2 4 6 8 10 20 30 40 #2 4 6 8 10 20 30 40 50 60 70 80 90 100 110 120 130 140 150 160 
do
postfix=$loop
# N=2
# C=$loop
# RTT=$loop
#RTT=5

if [ $test_mod == "C" ]; then 
	C=$loop
fi

if [ $test_mod == "R" ]; then 
	RTT=$loop
fi

if [ $test_mod == "N" ]; then 
	N=$loop
fi

subsubdir=C"$C"-N"$N"-D"$RTT"
mkdir ../Testdata/$dir/$subdir/$subsubdir

let buffer=2*$C*$RTT/12
./waf --run "evaCompeteOnDumbbell --runtime=$endtime --flowNum=$N --delayRB=$RTT --BW=$C --postfix=$postfix --queuesize=$buffer --cycle=0.5 --CC_mode=$CC_mode --attackerCC=$atk_mode --attackTime=$atk_Time --atteckerExit=$atk_Exit --rand_delay=1 --poisson_delay=1 --if_comp=0"  > ../Testdata/$dir/$subdir/$subsubdir/output"$postfix".txt 2>&1 

done

