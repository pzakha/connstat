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

.PHONY: \
	all \
	package \
	clean

all :
	cd src ; make all

package: clean
	./configure.sh
	dch --create --package connstat -v 0.0.1 \
			"Automatically generated changelog entry."
	dpkg-buildpackage -b -uc -us

clean:
	if [ -f src/Makefile ]; then \
		cd src; make clean; \
	fi
	rm -f debian/changelog
	rm -f debian/control
	rm -f debian/install
	rm -f src/Makefile
