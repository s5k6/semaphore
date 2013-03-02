% Semaphore interface for the command line
% Stefan Klinger
% 2013-Feb-27


Semaphore interface for the command line
========================================

Copyright © 2012, 2013 Stefan Klinger <http://stefan-klinger.de>


Abstract
--------

The `sem` command offers a versatile, yet simple frontend to named
[system
semaphores](http://www.kernel.org/doc/man-pages/online/pages/man7/sem_overview.7.html
"man page").  Using semaphores instead of a [construct with
fifos](http://stackoverflow.com/questions/2733198/command-line-semaphore-utility#2733290)
avoids problems with concurrent initialisation.

To allow for the protection of multiple commands in shell scripts, the
level of operation is more primitive than [GNU
sem](http://www.gnu.org/software/parallel/sem.html).  This also allows
synchronisation of tasks across different scripts and even users.

`sem` comes with some auxilliary shell scripts to further facilitate
semaphore management on Linux.  The provided `barrier` script
implements the eponymous synchronisation pattern.

The operations provided are to initialise, wait, post, and remove
named semaphores.  Blocking and nonblocking operation is supported, as
well as timeout.  A command may be run in between wait and post
operations, optionally in the background.  Most naming restrictions
are liberated, names may even contain slashes.  Global and per-user
semaphores reside in different namespaces.


Getting started
---------------

Download the [sources](downloads), unpack, make, and enjoy.  `sem`
comes with extensive [documentation](sem.html), and it's
[free](COPYING).


Example
-------

Imagine you use the [GraphicsMagick](http://www.graphicsmagick.org/)
package to resize a lot of images.  The compute intensive part is the
command `gm`.

    gm convert -resize x600 image.jpeg small-image.jpeg;

It produces a low-resolution version named “small-image.jpeg” of an
image file “image.jpeg”.  Typically, you would run this on hundreds of
files, as in the following loop:

    find ~/pics -iregex '.*\.jpe?g' | while read f; do
        gm convert -resize x600 "$f" "small-$f";
    done;

However, only one `gm`-process is running at any time.  On a multicore
machine with sufficient memory, it might be fine to run a couple of
them concurrently.  But putting them all in the background
simultaneously will lead to a high load and probably consume all
memory.  So maybe just running four of them would be great.

To this end, simply prefix the `gm` command with a call of `sem`.  The
whole loop then reads

    find ~/pics -iregex '.*\.jpe?g' | while read f; do
        sem foo -i4 -f -x -- gm convert -resize x600 "$f" "small-$f";
    done;

By semaphore magic, you will have exactly four instances of `gm`
running for most of the time — exceptions being when there are less
than four images left to convert, and the small lapse of time when
one `gm` process has just ended and the next one has not yet been
launched.

Enjoy!
