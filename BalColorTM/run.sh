GRAPHS="FullChip.mtx delaunay_n24.mtx "


THREADS="1 2 4 8 14"
sep='/'

for graph in $GRAPHS; do
    for n in $THREADS; do
        echo $n
        graph_cut="$(cut -d'/' -f2 <<< $graph)"
        graph_cut=${graph_cut::-4}
        graph_cut=${graph_cut//[_]/-}
        echo ${graph_cut}
        ./BalColorTM -g ../inputs/$graph -t -n $n  
    done 
done #  graph

