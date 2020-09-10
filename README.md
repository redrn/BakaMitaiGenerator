# BakaMitaiGenerator
An automatic generator for deepfake memes

This software provides a GUI interface to the [first-order-model](https://github.com/AliaksandrSiarohin/first-order-model) that is commonly used for creating deepfake memes.

# Installation
Simply download the build from release (which I will upload it later after thorough testing of compatibility).

The build is static with size optimization, some eseential dependencies, such as msvcp120.dll, will be included in the release. 
Deletion of those included dlls are optional. However, the program is not guaranteed to work.


# Compilation
## Requirement
### [first-order-model](https://github.com/AliaksandrSiarohin/first-order-model)

 - clone [first-order-model](https://github.com/AliaksandrSiarohin/first-order-model) repo:
`git clone https://github.com/AliaksandrSiarohin/first-order-model`

 - This program will execute the [demo](https://github.com/AliaksandrSiarohin/first-order-model/blob/master/demo.py) python code from this repo, 
so for compilation it is important to fullfill the [requirement list](https://github.com/AliaksandrSiarohin/first-order-model/blob/master/requirements.txt).

 - Download the pre-trained checkpoint `vox-cpk.pth.tar`\
`https://drive.google.com/drive/folders/1PyQJmkdCsAkOYwUyaj_l-l0as-iLDgeH`\
and put it inside the root directory of [first-order-model](https://github.com/AliaksandrSiarohin/first-order-model)

 - Locate the line `24` in [mainwindow.cpp](https://github.com/redrn/BakaMitaiGenerator/blob/master/mainwindow.cpp) file, where there are
```
// Assign first order model directory
firstOrderModelDir = QDir("C:\\Users\\RedRN\\Documents\\DamedaneGenerator\\BatchDamedane\\first-order-model");
```

Change the string address in `QDir()` to the [first-order-model](https://github.com/AliaksandrSiarohin/first-order-model) directory on your computer


### Qt
`version >= 5.15.0`
Older version of Qt should work, but is not tested.

### Build
Compilation is done with CMake, MSVC and Ninja.

Run `cmake` directly should start the compilation

