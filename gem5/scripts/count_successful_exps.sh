#!/bin/bash

cd ../stats/multiprogram_16GBmem_250Mn.SPEC2017.C4/
find . -maxdepth 1 -type d \( ! -name . \) -exec bash -c \
"cd '{}' && basename '{}' && cat */runscript.log | grep \"max instruction count\" | wc -l" \;
