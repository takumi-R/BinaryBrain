#!/bin/sh
iverilog -o tb_mnist_lut_mlp.vvp -s tb_mnist_lut_mlp -c iverilog_lut_mlp_cmd.txt
vvp tb_mnist_lut_mlp.vvp
