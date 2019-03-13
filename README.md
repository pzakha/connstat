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

You can specify which kernel to build the connstat module for by setting and
exporting environment variable KVERS (by default, KVERS=$(uname -r)). Note that
the appropriate kernel headers must be installed.

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

## Contributing

All contributors are required to sign the Delphix Contributor Agreement prior
to contributing code to an open source repository. This process is handled
automatically by [cla-assistant](https://cla-assistant.io/). Simply open a pull
request and a bot will automatically check to see if you have signed the latest
agreement. If not, you will be prompted to do so as part of the pull request
process.

This project operates under the [Delphix Code of
Conduct](https://delphix.github.io/code-of-conduct.html). By participating in
this project you agree to abide by its terms.

## Statement of Support

This software is provided as-is, without warranty of any kind or commercial
support through Delphix. See the associated license for additional details.
Questions, issues, feature requests, and contributions should be directed to
the community as outlined in the [Delphix Community
Guidelines](https://delphix.github.io/community-guidelines.html).
