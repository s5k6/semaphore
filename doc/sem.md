% title: sem(1)
% author: Stefan Klinger
% date: Mon 2017-Feb-13 16:58:50 CET


# Name

`sem` — Semaphore interface for the command line


# Synopsis

    sem ‹name› [‹args›…] [-- ‹command›…]
    sem [-L|-V]

The `sem` command offers a versatile, yet simple front-end to POSIX
named semaphores.  You may initialise, wait for, post, or remove a
semaphore, and run a command in between.


# Description

Invocation without arguments prints a help message, `sem -V` causes
version information to be printed, and `sem -L` shows license
information.  This form terminates without performing any operation.

Usual invocation is the first form:

    sem ‹name› [‹args›…] [-- ‹command›…]

The arguments being as follows:

`‹name›`
  ~ the name of the semaphore to operate on.  If the name starts with
    a slash, it is used literally.  Otherwise, semaphores reside in a
    per-user name-space.  See Notes on Names, and `-g` below.  Must be
    the first argument.

`-- ‹command›…`
  ~ command to be executed, and its arguments.  This must be the last
    argument, i.e., the double-dash indicates the end of sem's
    arguments.

`-i ‹num›`
  ~ initialise semaphore to ‹num› if it has to be created.  Without
    this option, fail if the semaphore does not exist.  The first
    process chosen by the OS's scheduler to create the semaphore will
    set the value.  Also see `-I`.

`-I ‹num›`
  ~ same as `-i`, but force creation of semaphore, i.e., fail if it
    already exists.

`-w`
  ~ wait for the semaphore.  If a command is specified, it is run
    after waiting successfully.

`-p`
  ~ post the semaphore.  If a command is specified, the semaphore is
    posted after the command terminates.

`-x`
  ~ turnstile, i.e., first wait for the semaphore, then post it.  If a
    command is specified, it is run after waiting successfully, and
    the semaphore is posted after the command terminates.

`-f`
  ~ fork into background after waiting, and run any command there.
    Any post operation is also performed in the background, after the
    command terminates.

`-u`
  ~ unlink semaphore.  This happens after waiting, just before running
    a command.

`-m ‹mode›`
  ~ if a new semaphore is created, set its permissions to ‹mode›.  The
    mode must be in the range 0..0666, i.e., the x-bits cannot be set.
    Both read and write permission should be granted to each class of
    user that will access the semaphore.

`-g`
  ~ generate a global name.  Without this option a semaphore whose
    name does not start with a slash resides in a per-user name-space,
    see Notes on Names below.  Take care to also set the permissions
    correctly, see `-m`.

`-t ‹time›`
  ~ Without ‹time›, or if ‹time› equals 0, switch to non-blocking mode,
    i.e., fail immediately if waiting would block.  If ‹time› is
    positive, time out after blocking for ‹time› seconds.  Failure due
    to timeout is signalled by exit code 1.

`-T ‹time›`
  ~ as `-t`, but ‹time› specifies an absolute timeout time, given in
    seconds since the epoch.  Use the `date` command to calculate
    this, see examples.

`-v ‹level›`
  ~ be verbose.  Sets the level of verbosity to the given value, or
    increases by one if no value is given.  The default level is 1, so
    `-v0` is pretty quiet.

`-q`
  ~ Query semaphore value, before performing any wait/post operation.
    Note, that it is *not* save to rely on this information, since it
    may be outdated by the time of printing.  Do not use this unless
    for getting a rough idea while debugging!

`-E ‹offset›`
  ~ offset `sem`'s exit codes by ‹offset›, but leave exit codes from
    the ‹command› alone.  See Notes on Exit Codes below.

`-V`, `-L`
  ~ show version or license information, and exit.

`-h`
  ~ show help, and exit.


## Notes on Names

On Linux, semaphore names are global, must have an initial slash, and
must not contain further slashes.  The length is limited to 251
characters.  The first restriction leads to name clashes between
different users, the others are annoying.

To this end, `sem` distinguishes between “user semaphore names” the
user enters on the command line, and “system semaphore names” that are
provided to the OS's semaphore system calls.

If the name provided to `sem` starts with a slash ‘/’, it is
considered a system name, and used without further conversion.
Otherwise, it is prefixed with the user's name (from the environment
variable $USER) and a colon, unless `-g` is given.

To allow for colons and slashes in the names, user semaphore names
undergo a poor variant of URL-escaping (aka. %-escaping).  Note, that
this may significantly elongate the name.

With verbosity level 3 or higher (`-v3`), the system semaphore name is
shown.

Note, that the operating system may further mangle the semaphore name.
Under Linux, semaphores are stored under `/dev/shm/sem.*`.  The
command `sem tool/mutex -i1 -v3` usually involves the following names
(the user is named “foo”):

    name passed to sem   :  tool/mutex
    system semaphore name:  /foo:tool%2fmutex
    file system name     :  /dev/shm/sem.foo:tool%2fmutex

With `-g`, a “global” semaphore is created, i.e., the name is not
prefixed with the user's name.  Example `sem tool/mutex -i1 -v3 -g`:

    name passed to sem   :  tool/mutex
    system semaphore name:  /tool%2fmutex
    file system name     :  /dev/shm/sem.tool%2fmutex

If the first character of the provided name is a slash ‘/’, then `sem`
falls back to system names, i.e., does not mangle the name on its own.
Note, that *no* further slashes are allowed in the semaphore name in
this case.  Hence, `sem /tool.mutex -i1` leads to

    name passed to sem   :  /tool.mutex
    system semaphore name:  /tool.mutex
    file system name     :  /dev/shm/sem.tool.mutex


## Notes on Signals

Pressing C-c sends SIGTERM to `sem`.  However, you probably don't want
to terminate `sem` itself, but rather its child, and have `sem` post a
semaphore as soon as the child dies.

To this end, iff there is a child process, `sem` ignores the signals

    SIGHUP, SIGINT

and forwards the signals

    SIGQUIT, SIGTERM, SIGUSR1, SIGUSR2,
    SIGCONT, SIGTSTP, SIGTTIN, SIGTTOU

to the child.  Signal handlers are installed only if there is a command
to be run.


## Notes on Exit Codes

If `sem` runs a command in the foreground, it tries to exit with the
exit code of the command, or one of the following.  To disambiguate
failure of `sem` from failure of the command, use `-E ‹offset›` to
offset (i.e., add a constant value to) these codes.  See examples.
‹offset› defaults to 0.

0 
  ~ All actions have been carried out successfully, and the
    command exited with 0.

1 + ‹offset›
  ~ Failed to wait due to timeout or in non-blocking mode.

2 + ‹offset›
  ~ Inappropriate usage of `sem`, i.e., usage error.

3 + ‹offset›
  ~ A system-call failed.  A more detailed description is given
    in the error message.

4 + ‹offset›
  ~ An error in the implementation of `sem`.  You should not
    see this.

5 + ‹offset›
  ~ The command was killed by a signal.

‹other›
  ~ The command returned with the respective exit code.


## General Notes

The semaphores have kernel persistence: if not removed, a semaphore
will exist until the system is shut down.  Scripts that do not clean
up after themselves may leave behind unused semaphores.

All messages created by `sem` appear on stderr, and are prefixed with
the string “sem[‹pid›,‹name›]”, where ‹pid› is replaced by the process
id, and ‹name› is replaced by the semaphore name.  If no semaphore
name is used, the prefix is just “sem[‹pid›]”.

If a wait fails, due to timeout or other errors, no further actions
(unlink, post, running command) are performed, and `sem` returns with
a non-null exit code.

The `sem` process may replace itself with the command if it does not
have to wait for the command to return, i.e., if no post is requested,
and the command is not forked into the background.

_Beware:_ Debugging synchronisation code is difficult.  Testing is *not*
a sufficient indication of correctness of code.  For really cool
examples of synchronisation problems, see Downey's Little Book of
Semaphores (below).


# Examples

All the following examples use bash(1) syntax, operate on a
semaphore named “foo”, and run the command “command”. We assume
“path/*” to point to a bunch of files.

  * To run a command on some files, but make sure only 4 instances run
    at the same time:

        for i in path/*; do
            sem foo -i4 -x -f -- command "${i}";
        done;

    Since `sem` forks only after the waiting, you'll never have more
    than 5 instances of `sem` running, the 5th waiting, and hence
    there are at most 4 of the commands running.


  * If multiple commands need to be run, use separate instances of
    `sem` for waiting and posting.  This is bash syntax creating a
    sub-shell for each file, but never more than four at the same time:

        for i in path/*; do
            sem foo -i4 -w -v;
            ( command1 "${i}";
              command2 "${i}";
              sem foo -p -v;
            )&                          # note the ampersand
        done;

    Since the waiting happens before invocation of the sub-shell,
    you'll never have more than 4 sub-shells running.

    
  * Imagine you want to trigger a command on a certain event, but avoid
    event-bursts to trigger too many commands.  The usual way to do this
    is:

        sem foo -i1 -x -t -f -- command;

    The return code tells whether the command was executed.  But is may
    also make sense to throttle the triggering with a delay, i.e., make
    sure that the command is not executed more often than once every,
    say, 5 seconds:

        sem foo -i1 -x -t -f -- sleep 5 && command;


  * If you want `sem` to bail out after waiting for one minute, use

        sem foo -t 60 -w

    If you want to time out at tea time on a certain Friday in April
    of the year 2077, you may use the `date` command to calculate the
    seconds since the epoch.

        sem foo -T $(date -d '2077-4-30 17:00' +%s) -w


  * Use of `-E` to find out whether `sem` failed, or the command:

        sem foo -E100 -t -x -- sh -c 'exit 42';
        ec="${?}";
        if test "${ec}" -gt 100; then
           echo "sem exit code is $((ec-100))";
        else
           echo "command returned ${ec}";
        fi;


# Home

Updates are available from <https://github.com/s5k6/semaphore>.
Please report bugs there.


# See also

  * sem_overview(7)

  * Allen B. Downey.  The Little Book of Semaphores.  Green Tea Press.
    <http://www.greenteapress.com/semaphores/index.html>
