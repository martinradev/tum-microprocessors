#!/bin/bash
MAX_THREADS=$1

exeprog()
{
	progname=$1
	startsz=10000
	endsz=80000000
	stepsz=10000
	i=1
	while [ "$i" -le "$MAX_THREADS" ]; do
		f="${progname}_${startsz}_${endsz}_${i}.txt"
		cmd="./$progname -l $startsz $endsz $stepsz -t $i > $f"
		echo "Running $cmd"
		eval $cmd
		i=$((i+1))
	done
}

for u in 1 2 3 4; do
	exeprog "toupper_variant$u"
done


