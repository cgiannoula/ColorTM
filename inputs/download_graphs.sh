#!/bin/sh
  
# This script downloads some graph files
# Download matrix files in mtx format from: https://sparse.tamu.edu/

echo "Starting download..."


if [ ! -f "delaunay_n24.mtx" ]; then
  wget -N https://suitesparse-collection-website.herokuapp.com/MM/DIMACS10/delaunay_n24.tar.gz
  tar xzf delaunay_n24.tar.gz 
  mv delaunay_n24/delaunay_n24.mtx .
  rm -rf delaunay_n24/
  rm "delaunay_n24.tar.gz"
fi

if [ ! -f "FullChip.mtx" ]; then
  wget -N https://suitesparse-collection-website.herokuapp.com/MM/Freescale/FullChip.tar.gz
  tar xzf FullChip.tar.gz 
  mv FullChip/FullChip.mtx .
  rm -rf FullChip/
  rm "FullChip.tar.gz"
fi


echo "Finished downloading and extracting .mtx files"
