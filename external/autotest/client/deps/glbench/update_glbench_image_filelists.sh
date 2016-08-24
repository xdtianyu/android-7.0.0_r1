#!/bin/bash

cd ../glbench-images/glbench_reference_images
ls *.png | sort > ../../glbench/glbench_reference_images.txt
ls *.png | sort > index.html

TYPE=(glbench_knownbad_images glbench_fixedbad_images)

for images in ${TYPE[*]}
do
  cd ../../glbench-images/${images}
  ls */*.png 2>/dev/null | sort > ../../glbench/${images}.txt
  ls */*.png 2>/dev/null | sort > index.html
done
