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

STATUS_SUCCESS=0
STATUS_FAILURE=1

connstat_arg0="$(basename $0)"
connstat_prog=/usr/bin/connstat
connstat_curcmd=

fatal()
{
	typeset msg="$*"
	[[ -z "$msg" ]] && msg="failed"
	echo "TEST FAILED: $connstat_arg0: $msg" >&2
	exit 1
}

#
# Do sanity check calls to connstat; comparing the expected status
# and the number of expected words on the data lines for a particular
# argument set.
#
compare()
{
	typeset expected_status=$1
	typeset expected_word_count=$2
	typeset val ret

	connstat_curcmd="$connstat_prog $3 $4 $5 $6 $7 $8 $9 ${10}"
	echo Testing $connstat_curcmd
	$connstat_curcmd &> /dev/null
	ret=$?
	if [[ $expected_status -eq $STATUS_SUCCESS ]]; then
		if [[ $ret -ne $expected_status ]]; then
			fatal "For $connstat_curcmd unexpected exit status: $ret"
		fi
		wc=$($connstat_curcmd | tail -1 | wc -w)
		if [[ $wc -ne $expected_word_count ]]; then
			fatal "For $connstat_curcmd unexpected output: $($connstat_curcmd | tail -1)"
		fi
	elif [[ $ret -eq $STATUS_SUCCESS ]]; then
		fatal "For $connstat_curcmd unexpected exit status: $ret"
	fi
}

compare $STATUS_SUCCESS 5
compare $STATUS_FAILURE 0 -b
compare $STATUS_SUCCESS 6 -h
compare $STATUS_SUCCESS 5 -e
compare $STATUS_SUCCESS 5 -L
compare $STATUS_FAILURE 0 -F badf=3
compare $STATUS_SUCCESS 5 -F lport=22
compare $STATUS_FAILURE 0 -F lport=22 -o
compare $STATUS_FAILURE 0 -F lport=22 -o laddr,bado
compare $STATUS_SUCCESS 18 -F lport=22 -o all
compare $STATUS_SUCCESS 5 -F lport=22 -o laddr,lport,cwnd,inbytes,outbytes
compare $STATUS_FAILURE 0 -P
compare $STATUS_SUCCESS 1 -F lport=22 -o laddr,lport,cwnd,inbytes,outbytes -P
compare $STATUS_FAILURE 0 -F lport=22 -c
compare $STATUS_FAILURE 0 -F lport=22 -c 2
compare $STATUS_FAILURE 0 -F lport=22 -i
compare $STATUS_FAILURE 0 -F lport=22 -i 0
compare $STATUS_FAILURE 0 -F lport=22 -i 5 -c
compare $STATUS_FAILURE 0 -F lport=22 -i 5 -c 0
compare $STATUS_SUCCESS 5 -F lport=22 -i 5 -c 2
compare $STATUS_FAILURE 0 -F lport=22 -T
compare $STATUS_FAILURE 0 -F lport=22 -T a
compare $STATUS_SUCCESS 5 -F lport=22 -T u
compare $STATUS_SUCCESS 5 -F lport=22 -T d
compare $STATUS_SUCCESS 5 -F lport=22 -i 5 -c 2 -T d
compare $STATUS_SUCCESS 6 --help
compare $STATUS_SUCCESS 5 --established
compare $STATUS_SUCCESS 5 --no-loopback
compare $STATUS_FAILURE 0 --filter badf=3
compare $STATUS_SUCCESS 5 --filter lport=22
compare $STATUS_FAILURE 0 --filter lport=22 --output
compare $STATUS_FAILURE 0 --filter lport=22 --output laddr,bado
compare $STATUS_SUCCESS 18 --filter lport=22 --output all
compare $STATUS_SUCCESS 5 --filter lport=22 --output laddr,lport,cwnd,inbytes,outbytes
compare $STATUS_FAILURE 0 --parsable
compare $STATUS_SUCCESS 1 --filter lport=22 --output laddr,lport,cwnd,inbytes,outbytes --parsable
compare $STATUS_FAILURE 0 --filter lport=22 --count
compare $STATUS_FAILURE 0 --filter lport=22 --count 2
compare $STATUS_FAILURE 0 --filter lport=22 --interval
compare $STATUS_FAILURE 0 --filter lport=22 --interval 0
compare $STATUS_FAILURE 0 --filter lport=22 --interval 5 --count
compare $STATUS_FAILURE 0 --filter lport=22 --interval 5 --count 0
compare $STATUS_SUCCESS 5 --filter lport=22 --interval 5 --count 2
compare $STATUS_FAILURE 0 --filter lport=22 --timestamp
compare $STATUS_FAILURE 0 --filter lport=22 --timestamp a
compare $STATUS_SUCCESS 5 --filter lport=22 --timestamp u
compare $STATUS_SUCCESS 5 --filter lport=22 --timestamp d
compare $STATUS_SUCCESS 5 --filter lport=22 --interval 5 --count 2 --timestamp d

echo "TEST PASSED: $connstat_arg0"
