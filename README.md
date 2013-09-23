Speedo-meter: How fast is my program going?

This version works on both SNB and KNC. It will exit with an error message
on other processor models.

Contents:

The command-line speedometer tool and the GUIs are under the bin directory
in mic (for KNC) and intel64 (for SNB). The GUIs run only on the host and
require the QT5 shared libraries.

The test directory contains two small test programs that exercise
vector/floating point operations (matrix multiply) and memory bandwidth (a
version of stream).

libqt contains the QT5 shared libraries and the QWT shared library.

Installation:

The speedo-meter binary must be setuid root since it uses privileged features
of the linux perf system as well as programming some uncore events directly.
The program restores the effective user id before exec'ing the users code so
there is no security risk. Source code and build instructions will be
available shortly for auditing and rebuilding if desired.

The speedometer shell script saves the LD_LIBRARY_PATH which is then restored
before exec'ing the user's code (since setuid binaries delete LD_LIBRARY_PATH
from the environment).

To properly set permissions run the chmod.sh script as root. If you don't
mount the installation directory on the MIC card you will have to ensure that
the copy of speedo-meter for the MIC is setuid root and that the speedometer
script is in the same directory.

Usage:

The command speedometer can be run in two ways:

speedometer -s

causes it to go into server mode and listen for connections on a socket.

speedometer command ...

causes command ... to be run under the control of speedo-meter and prints a
summary at the end.

Additionally

speedometer -o file.csv command...

will run command and put a trace of metrics over time to file.csv .

Tests:

There are a couple of test programs that exercise flops and memory in "test".

GUI:

There are two GUIs. They are based on QT 5. They run only on the host

"monitor"
is for monitoring the server. Just bring it up and click "resume".
"metrics -s" should be running on the mic0 card.
The "File>Open" dialog lets you specify a different host and port.

"plotter"
is for plotting csv files. Run it, bring up the File>Open menu, and
navigate to your .csv.

You can zoom on the plot with left-click and drag, and unzoom with
right-click.

QT libraries:

You need to set your LD_LIBRARY_PATH to point at the libqt directory, unless
you already have your own installation of QT5 and QWT 6.1 set up. Additionally
there is a platform support library in the bin/intel64/platform library.

Enjoy.

-- Larry
lawrence.f.meadows@intel.com
