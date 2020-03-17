# contours-viewer

A graphical frontend for https://github.com/ldmnyblzs/libcontours

## Dependencies

The libcontours library is required with all its dependencies.
Boost, wxWidgets are required. VTK, libGD and TBB are optional.

## Compilation

```
mkdir build
cd build
bliss_DIR=../../bliss-cmake/build/lib/cmake/bliss/ contours_DIR=../../libcontours/build/lib/cmake/contours/ cmake ..
cmake --build .
```

## Usage

Run the `contours_viewer` executable from the `build` folder!

Click Open to load a single file in STL or OFF format or a batch file in CSV format. In the former case you can set the input parameters on the left hand side. Either way, click Compute to run the computations. When finished, click Save to store the results in an image file or in a CSV.

An example CSV file is available in the project root. It is in Hungarian at the moment, please contact the owner of this repository for further assistance if you are interested in using it!
