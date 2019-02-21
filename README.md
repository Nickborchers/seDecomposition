# Project Title

OMP Convex/symmetrical structuring element decomposition based on a paper from Vaz et al.

## Running

The first argument specifies the image name, the second argument specifies the radius of the spherical SE.
To see a basic example of the working, run for example

```
make clean && make all && ./sedecomp img1.png 3
./sedecomp img2.png 4
./sedecomp img3.png 4
./sedecomp img4.png 8
```

## 3rd party libraries
 * [stb](https://github.com/nothings/stb) single-file public domain libaries for c/c++, we use stbi_image_write and stbi_image_read for basic image I/O 

## Authors

* **Nick Borchers** - *Initial work* - [Nickborchers](https://github.com/Nickborchers) under the supervision of Dr M.H.F. Wilkinson