# gst-barcode-reader
Gstreamer plugin to decode barcodes with OPTIMOM module

# Barcode plugin : Version 1.3

# About

This plugin implements a barcode detection and decoding using Zxing (Zebra crossing).

# Dependencies

The following libraries are required for this plugin.
- libgstreamer1.0-dev
- libgstreamer-plugins-base1.0-dev

Install them with: 

```
sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
```


# Compilation

First you must make sure that your device's clock is correctly setup.
Otherwise the compilation will fail.

## Cmake 3.23.2
This specific version of cmake is required for Zxing installation. Download it with:
```
wget https://cmake.org/files/v3.23/cmake-3.23.2-linux-aarch64.tar.gz
```
Extract the archive:
```
tar -xvzf cmake-3.23.2-linux-aarch64.tar.gz
```
Install the software:
```
sudo mv cmake-3.23.2-linux-aarch64/bin/* /usr/bin
sudo mv cmake-3.23.2-linux-aarch64/share/* /usr/share
```
## Zxing
You need to install Zxing to use the barcode reader plugin.
You can follow the following command to install Zxing :
```
git clone https://github.com/nu-book/zxing-cpp.git
```
Checkout this specific commit:
```
cd zxing-cpp
git checkout 74101f2b
```
Configure the make options, in source build always works in external projects :
```
mkdir build
cd build
cmake -DBUILD_WRITERS=OFF -DBUILD_READERS=ON -DBUILD_EXAMPLES=OFF -DBUILD_BLACKBOX_TESTS=OFF -DBUILD_UNIT_TESTS=OFF -DBUILD_PYTHON_MODULE=OFF ..
```
And install. Only run the install in sudo :
```
sudo make -j4 install
cd ../..
```
## Gstreamer plugin

In the BarcodeReaderPlugin folder do:
```
bash autogen.sh
```
Now you cake compile with:
```
make
```
# Install
```
sudo make install
```
To test if the plugin has been correctly install, do:
```
export GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0/
export LD_LIBRARY_PATH=/usr/local/lib/
gst-inspect-1.0 barcodereader
```
If the plugin failed to install the following message will be displayed: ```No such element or plugin 'barcodereader'```

# Uninstall
```
sudo rm /usr/local/lib/gstreamer-1.0/libgstbarcodereader.*
```
# Usage
By default the plugin is installed in /usr/local/lib/gstreamer-1.0. 
It is then required to tell gstreamer where to find it with the command:
```
export GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0/
```
In the same way, you need to tell where to find the Zxing installation :
```
export LD_LIBRARY_PATH=/usr/local/lib/
```
The plugin can be used in any gstreamer pipeline by adding "barcodereader", the name of the plugin. You can use it alone or with the autofocus plugin.

The decoding will appear in the terminal.

## Pipeline examples:
Simple pipeline to decode all barcode types:
```
gst-launch-1.0 v4l2src ! barcodereader framing=false type=all number-barcode=5 ! queue ! videoconvert ! queue ! xvimagesink sync=false
```
To optimize the decoding speed, select some barcode types:
```
gst-launch-1.0 v4l2src ! barcodereader framing=false type=Aztec,Codabar,Code128,QRCode number-barcode=2 ! queue ! videoconvert ! queue ! xvimagesink sync=false
```
# Plugin parameters

- type: 
    - Type: string
    - Default value: "Code128,QRCode"
    - Description:  Specifie the different barcode type the plugin should decode. By default, Code128 and QRCode are set.
                    Please, use the ',' separator. You can specifie multiple type amoung these :
                        - all : for all barcodes
                        - 1D-Codes : for all the 1D barcodes
                        - 2D-Codes : for all the 2D barcodes
                        - Aztec
                        - Codabar
                        - Code39
                        - Code93
                        - Code128
                        - DataBar
                        - DataBarExpanded
                        - DataMatrix
                        - EAN-8
                        - EAN-13
                        - ITF
                        - MaxiCode
                        - PDF417
                        - QRCode
                        - UPC-A
                        - UPC-E

- number_barcode: 
    - Type: int
    - Minimal value: 0
    - Maximal value: 25
    - Default value: 1
    - Description: Set the maximum number of barcode it should found

- compressed: 
    - Type: boolean
    - Default value: false
    - Description: Whether or not it should compressed the image for the detection and decoding. It reduce execution time but can find less barcode

- framing: 
    - Type: boolean
    - Default value: true
    - Description: Whether or not it should frame the barcodes detected

- strPos:
    - Type: string
    - Default value: ""
    - Description: Read-only property to get the coordinates of barcodes

- strCode:
    - Type: string
    - Default value: ""
    - Description: Read-only property to get the decoding of barcodes

- clearBarcode: 
    - Type: boolean
    - Default value: false
    - Description: Whether or not it should clear the list of detected barcodes


