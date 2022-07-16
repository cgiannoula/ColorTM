## ColorTM: High-Performance and Balanced Parallel Graph Coloring on Multicore Platforms

<p align="center">
  <img width="545" height="408" src="https://github.com/cgiannoula/ColorTM/blob/main/images/ColorTM_Logo.png">
</p>

ColorTM contains two parallel graph coloring algorithms for multicore platforms: (i) ColorTM: assigns colors to the vertices of the graph using multiple parallel threads and leveraging Hardware Transactional Memory (HTM), and (ii) BalColorTM: re-colors a subset of the vertices of the graph to produce color classes with almost the same sizes, i.e., having almost the same number of vertices assigned to each color class produced. Both algorithms are written in C programming language and can be used to color any arbitrary real-world graph.

## Cite <i>ColorTM</i>

Please cite the following papers if you find this repository useful:


Bibtex entries for citation:
```
@inproceedings{Giannoula2018ISC,
    author={Giannoula, Christina and Goumas, Georgios and Koziris, Nectarios},
    title={{Combining HTM with RCU to Speed up Graph Coloring on Multicore Platforms}}, 
    year = {2018},
    booktitle={High Performance Computing},
}
```

## Repository Structure
We point out next the repository structure and some important folders and files.<br> 
The "ColorTM" includes the (unbalanced) ColorTM algorithm.<br>
The "BalColorTM" includes the balanced BalColorTM algorithm.<br>
The "inputs" directory includes a bash script to download real-world graph files (in mtx format) from the [Suite Sparse Matrix Collection](https://sparse.tamu.edu/).<br> 

```
.
+-- LICENSE
+-- README.md
+-- inputs/ 
+-- ColorTM/ 
+-- BalColorTM/ 
```

## Requirements 

The ColorTM and BalColorTM kernels are designed to run on a multicore CPU machine with Hardware Transactional Memory (HTM) support.

## Running <i> ColorTM </i> and <i> BalColorTM </i>

### Clone the Git Repository

```sh
git clone https://github.com/cgiannoula/ColorTM.git

cd ColorTM
```

### Download Input Graph Files

```sh
cd inputs

./download_graphs.sh 
```


### Run the ColorTM algorithm 

```sh
cd ColorTM 
make

## Manually Run  
./ColorTM -g /path/to/input/graph/file.mtx -n <number_of_parallel_threads>

## Use the Script
./run.sh 
```


### Run the BalColorTM algorithm 

```sh
cd BalColorTM 
make

## Manually Run  
./BalColorTM -g /path/to/input/graph/file.mtx -n <number_of_parallel_threads>

## Use the Script
./run.sh 
```


## Support
For any suggestions for improvement, any issues related to the <i>ColorTM</i> and <i>BalColorTM</i> kernels or for reporting bugs, please contact Christina Giannoula at christina.giann\<at\>gmail.com.
