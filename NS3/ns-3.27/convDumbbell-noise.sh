

for i in 1 2
do
	for j in C N R
	do
		echo $i $j
		bash doEvaluation_Dumbbell-noise.sh $i $j
	done
done

