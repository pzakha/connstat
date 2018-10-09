# Connstat - TCP Connection Statistics

## Quickstart

### Building

Run this command on "dlpxdc.co" to create the VM used to do the build:

    $ dc clone-latest --size COMPUTE_LARGE bootstrap-18-04 USER-bootstrap

Log into that VM using the "ubuntu" user, and run these commands:

    $ git clone https://github.com/delphix/connstat.git
    $ cd connstat
    $ ansible-playbook bootstrap/playbook.yml
    $ make package

You can specify which kernel to build the connstat module for by setting and exporting environment variable KVERS (by default, KVERS=$(uname -r)). Note that the appropriate kernel headers must be installed.

This will generate debian packages in project root directory.  You
can install the kernel module and userland packages with the following
commands:

    $ apt install ./connstat-module-$(KVERS)_$(VERSION).deb
    $ apt install ./connstat-util_$(VERSION).deb

Once installed the following commands will execute, test, and view the
man page for the connstat utility.

    $ connstat
    $ /usr/share/connstat/test/connstat_test.ksh
    $ man connstat

## Statement of Support

This software is provided as-is, without warranty of any kind or
commercial support through Delphix. See the associated license for
additional details. Questions, issues, feature requests, and
contributions should be directed to the community as outlined in the
[Delphix Community Guidelines](http://delphix.github.io/community-guidelines.html).
