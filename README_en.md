﻿[Japanese version](README.md)

# BinaryBrain Version 3<br> --binary neural networks platform for LUT-networks

[Detailed documentation](https://binarybrain.readthedocs.io/en/latest/)

## Overview
BinaryBrain is a platform for deep learning. It can train to LUT(Look-up Table)-Network.
LUT-Network is one of binary neural networks.

It has the following features

- The main target is FPGA(field-programmable gate array).
- Regression analysis is possible though it is binary model.
- Efficient learning with original Stocastic-LUT model.
- It can compute sparse matrix with high performance.
- Developed in C++
- Accelerated with GPU (CUDA)


## Performance

Fully-Binary deep neural network.
1000fps Real-time recognition.

![fpga_environment.jpg](documents/images/fpga_environment.jpg "sample's photo image")
![block_diagram.png](documents/images/block_diagram.png "sample's block diagram")


It can be implemented on a FPGA with a small amount of resources.

![fpga_resource.png](documents/images/fpga_resource.png "FPGA resource")

A unique network model is available.

![Sparse-LUT_model.png](documents/images/Sparse-LUT_model.png "Sparse-LUT model")




## How to use sample program (MNIST)

Please, read "main.cpp" for usage.

### windows
1. install VisualStudio 2019 + CUDA 10.1
2. git clone --recursive -b ver3_release https://github.com/ryuz/BinaryBrain.git 
3. download MNIST from http://yann.lecun.com/exdb/mnist/
4. decompress MNIST for "\samples\mnist"
5. open VC++ solution "samples\mnist\sample_mnist.sln"
6. build "x64 Release"
7. run

### Linux(Ubuntu 18.04.1)
1. install tools 
```
% sudo apt update
% sudo apt upgrade
% sudo apt install git
% sudo apt install make
% sudo apt install g++
% sudo apt install nvidia-cuda-toolkit
```
2. build and run
```
% git clone --recursive -b ver3_release  https://github.com/ryuz/BinaryBrain.git
% cd BinaryBrain/samples/mnist
% make
% make dl_data
% ./sample-mnist All
```

If you don't use GPU, please add "WITH_CUDA=No" option to make.

### Google Colaboratory
Currently you can use nvcc on Google Colaboratory.
Please select GPU runtime.
```
!git clone --recursive -b ver3_release  https://github.com/ryuz/BinaryBrain.git
%cd BinaryBrain/samples/mnist
!make all
!make run
```
You can build C++ source code from iPython Notebook.


### Python (Beta version)

#### Preparation

Install packeges.
```
% pip3 install setuptools
% pip3 install pybind11
% pip3 install numpy
% pip3 install tqdm
```

When using Windows, 64-bit version of VisualStudio is required.
('x64' option is important)

```
> "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
```

#### Install

```
% # install
% cd python
% python3 setup.py install
```

#### Run sample programs (MNIST)

```
% # Simple DNN sample
% python3 MnistSparseLutSimple.py

% # CNN sample
% python3 MnistSparseLutCnn.py
```


#### Install with pip
```
% pip3 install binarybrain
```


## What is LUT networks?

There is also a document on [slideshare](https://www.slideshare.net/ryuz88/lutnetwork-revision2-english-version).

### Design flow
FPGA circuit is constructed from LUTs.
This platform let the LUT's table learn directly.

![LutNet_design_flow.png](documents/images/LutNet_design_flow.png "design flow")


### Binary modulation model
BinaryBrain can handle binary modulated models.
The binary modulation model is as follows.

![modulation_model.png](documents/images/modulation_model.png "modulation_model")

For example, PWM(Pulse Width Modulation), delta sigma modulation, and Digital amplifier are also a kind of binary modulation.


## License
This source code's license is MIT license.

(Note : This program using CEREAL)

## Reference
- BinaryConnect: Training Deep Neural Networks with binary weights during propagations<br>
https://arxiv.org/pdf/1511.00363.pdf

- Binarized Neural Networks<br>
https://arxiv.org/abs/1602.02505

- Binarized Neural Networks: Training Deep Neural Networks with Weights and Activations Constrained to +1 or -1<br>
https://arxiv.org/abs/1602.02830

- XNOR-Net: ImageNet Classification Using Binary Convolutional Neural Networks<br>
https://arxiv.org/abs/1603.05279

- Xilinx UltraScale Architecture Configurable Logic Block User Guide<br>
https://japan.xilinx.com/support/documentation/user_guides/ug574-ultrascale-clb.pdf


## Author's information
Ryuji Fuchikami
- github : https://github.com/ryuz
- blog : http://ryuz.txt-nifty.com
- twitter : https://twitter.com/ryuz88
- facebook : https://www.facebook.com/ryuji.fuchikami
- web-site : http://ryuz.my.coocan.jp/
- e-mail : ryuji.fuchikami@nifty.com


