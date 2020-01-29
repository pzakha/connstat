#!/usr/bin/env python3
#
# Copyright (c) 2019 by Delphix. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#

import argparse
import os
import re
import subprocess
import unittest
import yaml
from subprocess import CalledProcessError


#
# Test methods in this class are programatically generated in __main__
#
class ConnstatTest(unittest.TestCase):
    pass


def run_connstat(name, arguments):
    input_filename = 'test_io/' + name + '.in'
    if not os.path.isfile(input_filename):
        input_filename = 'test_io/default.in'
    environ = {'STATS_FILENAME': input_filename,
               'PATH': os.environ['PATH']}

    command = ['../cmd/connstat']
    arguments = arguments.split(' ')
    for arg in arguments:
        if arg:
            command.append(arg)

    proc = subprocess.run(command, capture_output=True, check=True, text=True,
                          env=environ)
    return proc


def normalize_timestamps(output):
    """Replace all timestamps with a fixed string."""
    # Replace ISO datetime formatted timestamps
    isoregex = r'^(= )*\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d$'
    epochdate = '1970-01-01T00:00:00'
    output = re.sub(pattern=isoregex, repl=epochdate, string=output,
                    flags=re.MULTILINE)
    # Replace timestamps in seconds since epoch
    timestampregex = r'^(= )*\d{10}$'
    epochtimestamp = '0000000000'
    output = re.sub(pattern=timestampregex, repl=epochtimestamp, string=output,
                    flags=re.MULTILINE)
    return output


def test_generator(name, expected_exit, arguments_list):
    def output_check(self, stdout):
        self.maxDiff = None
        output_filename = 'test_io/' + name + '.out'
        with open(output_filename) as f:
            expected_output = f.read()
        stdout = normalize_timestamps(stdout)
        expected_output = normalize_timestamps(expected_output)
        self.assertEqual(stdout, expected_output)

    def exit_code_check(self, actual_exit, expected_exit):
        if expected_exit == 0:
            self.assertEqual(actual_exit, 0)
        else:
            self.assertGreaterEqual(actual_exit, 1)

    def run_test(self):
        for arguments in arguments_list:
            with self.subTest(arguments=arguments):
                try:
                    proc = run_connstat(name, arguments)
                    exit_code_check(self, proc.returncode, expected_exit)
                    output_check(self, proc.stdout)
                except CalledProcessError as err:
                    exit_code_check(self, err.returncode, expected_exit)
    return run_test


def load_test_data():
    with open('tests.yml') as f:
        tests = yaml.load(f, Loader=yaml.FullLoader)
    return tests


def normalize_arguments_list(arguments_list):
    """Make sure that arguments_list is a list"""
    if not arguments_list:
        arguments_list = ['']
    elif not isinstance(arguments_list, list):
        arguments_list = [arguments_list]
    return arguments_list


def generate_output_files(tests):
    for test_data in tests:
        #
        # Only generate output files for tests that don't expect connstat to
        # fail.
        #
        if test_data['exit'] != 0:
            continue

        name = test_data['name']
        arguments_list = normalize_arguments_list(test_data['arguments'])
        #
        # We only need to generate one output file, so use the first set of
        # arguments.
        #
        arguments = arguments_list[0]
        try:
            proc = run_connstat(name, arguments)
            output_filename = 'test_io/' + name + '.out'
            with open(output_filename, mode='w') as f:
                f.write(proc.stdout)
        except CalledProcessError as err:
            print(err.stderr)


def run_tests(tests):
    for test_data in tests:
        arguments_list = normalize_arguments_list(test_data['arguments'])
        test_method = test_generator(name=test_data['name'],
                                     expected_exit=test_data['exit'],
                                     arguments_list=arguments_list)
        test_method.__name__ = 'test_' + test_data['name']
        setattr(ConnstatTest, test_method.__name__, test_method)
    unittest.main(verbosity=2)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(prog='connstat_unittest')
    parser.add_argument('-g', '--generate_output', action='store_true',
                        default=False,
                        help='Generate unit test output files')
    args = parser.parse_args()

    tests = load_test_data()

    if args.generate_output:
        generate_output_files(tests)
    else:
        run_tests(tests)
