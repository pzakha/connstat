#!/bin/bash
#
# Copyright 2018 Delphix
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#

#
# The KCENTEVERS is the release up to the minor version treated as a floating
# point number multiplied by 100 (e.g for KVERS=4.18.0-1006-aws, KCENTEVERS=418).
# This is used in module/src/connstat.c to preprocess the code for different kernel
# versions.  The C pre-processor #if conditional statements need integers 
#
if [[ -z "$KVERS" ]]; then
	export KVERS=$(uname -r)
	export KCENTEVERS=$(echo 100*`uname -r | cut -f1 -d.`+`uname -r | cut -f2 -d.` | bc)
fi

sed "s/@@KVERS@@/$KVERS/g" \
	debian/control.in >debian/control
sed "s/@@KVERS@@/$KVERS/g" \
	debian/install.in >debian/install
sed "s/@@KVERS@@/$KVERS/g; s/@@KCENTEVERS@@/$KCENTEVERS/g" \
	src/Makefile.in >src/Makefile
