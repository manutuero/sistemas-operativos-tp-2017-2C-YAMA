#!/bin/bash
# Crea 3 nuevos data.bin y los trunca a 15 MiB

touch data1.bin data2.bin data3.bin
truncate -s 15728640 data1.bin data2.bin data3.bin
