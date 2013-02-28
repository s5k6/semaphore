
// Feature Test Macro Requirements
//#define _POSIX_C_SOURCE 200112L
#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <semaphore.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>


/* Exit codes returned by sem */
#define OkError 1 /* Signal acceptable failure to calling process. */
#define UserError 2 /* The user did not use sem in an expected way */
#define SysError 3 /* A systemcall failed */
#define ImplError 4 /* An implementation error, should not happen */
#define KilledError 5 /* The command has been killed */

/* maximum length of semaphore name, plus terminating null. */
#define maxNameLen 252



char const * name = 0;  // semaphore name, req'd by error funcs
pid_t pid; // child pid, req'd by signal handler
int verbose = 1; // print info to stdout



/* All of sem's exit codes may be offset, to disambiguate from the
   command's error. */
int offset = 0;

void exit_offset(int const ec) {
  exit(ec + offset);
}

#define exit exit_offset



/* Sorry, I'm using my own ‘warn’ and ‘err’ functions, since I want to
   add sem's pid and the semaphore name in the output as well. */

void vout(FILE * stream, const char * fmt, va_list ap) {
  if (name) fprintf(stream, "sem[%i,%s] ", getpid(), name);
  else fprintf(stream, "sem[%i] ", getpid());
  vfprintf(stream, fmt, ap);
  fprintf(stream, "\n"); 
}

void out(const char * fmt, ...) {
  va_list ap;
  va_start(ap, fmt); vout(stdout, fmt, ap); va_end(ap);
}

void warn(const char * fmt, ...) {
  va_list ap;
  va_start(ap, fmt); vout(stderr, fmt, ap); va_end(ap);
}

void err(int const ec, const char * fmt, ...) {
  va_list ap;
  va_start(ap, fmt); vout(stderr, fmt, ap); va_end(ap);
  exit(ec);
}

#define vout DO_NOT_USE_VOUT



/* 0-terminated list of signals that should be ignored (sig_ign) by
   `sem`, or forwarded (sig_fwd) to the child process.  These are
   hard-coded, maybe there should be a CLI for changing them. */

int const sig_ign[] = { SIGHUP, SIGINT, 0 };
int const sig_fwd[] = { SIGQUIT, SIGTERM, SIGUSR1, SIGUSR2, SIGCONT
                      , SIGTSTP, SIGTTIN, SIGTTOU, 0
                      };

void sig_forward(int const sig) {
  if (verbose > 1)
    warn("sending signal %i to pid=%i", sig, pid);
  if (kill(pid, sig) != 0) warn("kill(%i,%i): %m", pid, sig);
}



/* Sorry again: I consider the argument processing implemented here
   superior to the GNU standard.  This is a feature, not a bug.

   Arguments for opt(argc, argv, &i, &o, &v):

     - argc, argv from main(argc, argv)

     - i index of next arg to process

     - o is the option char, or 0 if the argument was '-'

     - v points to the value that came with the option, if any.

  Example:
    argc=6 argv=["sem", "-a", "-b", "foo", "-cbar", "qux"];
    i=1; # skip argv[0]
    opt(argc, argv, &i, &o, &v); # ==1, i==2, o=='a', v==0
    opt(argc, argv, &i, &o, &v); # ==1, i==4, o=='b', v=="foo"
    opt(argc, argv, &i, &o, &v); # ==1, i==5, o=='c', v=="bar"
    opt(argc, argv, &i, &o, &v); # ==0, because ‘qux’ is not an option
 */

int opt(int const argc, char * const * const argv, int * i, char * o, char ** v) {
  if (*i >= argc) return 0; // no arguments
  if (argv[*i][0] != '-') return 0; // this is not an option
  if (!(*o = argv[*i][1])) { (*i)++; return 1; } // only a single dash
  *v = 0;
  if (argv[*i][2]) *v = argv[*i]+2; // option with value attached
  (*i)++; // next arg, which may be a value if v==0

  if (*i < argc && argv[*i][0] != '-') {
    if (*v) err(UserError, "Where does argument %i=%s belong to?", *i, argv[*i]);
    *v = argv[(*i)++]; // option with value separated
  }
  return 1;
}



int main(int argc, char ** argv) {
  
  char ** cmd = 0;
  char const * realName = 0; // system name of semaphore
  int daemon = 0 // fork for command execution
    , global = 0 // do not prefix with user name
    , oflag = 0 // do not create new semaphore
    , post = 0 // post after command
    , query = 0 // print semaphore value
    , status = 0 // exit status of command
    , timeout = 0 // timeout in seconds
    , unlink = 0 // rm semaphore after wait
    , wait = 0 // wait before command
    ;
  unsigned int init = 0; // initial value for creation
  mode_t mode = 0600; // rw- for user alone
  sem_t * sem = 0;
  struct timespec time = { 0, 0 }; // holds absolute timeout time
  
  // Argument processing
  {
    int i = 1; // first argv is sem name at index 1
    char o
       , * v
       , * endptr
       ;

    if (argc<2) {
      printf("\n%s\n",
#include "short.inc"
             );
      exit(EXIT_SUCCESS);
    }
    
    if (argv[i][0] != '-') name = argv[i++];

    while (opt(argc, argv, &i, &o, &v)) {
      switch (o) {
        
      case '-':
        if (!v) err(UserError, "-%c requires command.", o);
        int const c = i-1;
        cmd = calloc(argc-c+1, sizeof(char*)); // add  one for terminal null
        if (!cmd) err(SysError, "calloc: %m.");
        cmd[0] = v;
        while (i < argc) { cmd[i-c] = argv[i]; i++; }
        break;
        
      case 'i': case 'I':
        if (!v) err(UserError, "-%c requires initial value.", i-1, o);
        if (oflag) warn("-%c overwrites previous setting.", o);
        oflag = (o == 'i')
              ? O_CREAT
              : O_CREAT|O_EXCL
              ;
        init = strtol(v, &endptr, 10);
        if (*endptr || init < 0)
          err(UserError, "-%c expects positive decimal integer.", o);
        break;
        
      case 'E':
        if (!v) err(UserError, "-%c requires offset.", o);
        if (offset) warn("-%c overwrites previous setting.", o);
        offset = strtol(v, &endptr, 10);
        if (*endptr || offset < 0)
          err(UserError, "-%c expects positive decimal integer.", o);
        if (offset > 250)
          warn("Offset %i > 250, may distort exit code.", offset);
        break;
        
      case 'm':
        if (!v) err(UserError, "-%c requires file mode.", o);
        mode = strtol(v, &endptr, 8);
        if (*endptr || 0666 < mode || mode < 0)
          err(UserError, "-%c expects octal integer in range 0..0666.", o);
        break;

        /* Currently not implemented: Change ownership.  Hard to avoid
           race condition!  An unprivileged process might grab the
           semaphore, before ownership is set properly.

    -o <user>.<group> ― if a new semaphore is created, set ownership
        to <user> and <group> respectively.  One of them may be
        omitted to fall back to the default.  The dot ‘.’ has to be
        present only if the group is to be set.  Note, that this
        operation is not atomic, and thus makes sense only with the
        ‘-I’ option.
        
      case 'o':
        err(ImplError, "Arg %i: change of ownership not supported", i-1);
        break;
        */
        
      case 'f':
        if (v) err(UserError, "-%c does not take arguments.", o);
        daemon = 1;
        break;

      case 'l':
        err(UserError, "The use of -%c is deprecated.", o);
        break;

      case 't':
        if (timeout) warn("-%c overwrites previous setting.", o);
        if (v) {
          timeout = strtol(v, &endptr, 10);
          if (*endptr || timeout < 0)
            err(UserError, "-%c takes optional positive decimal integer.", o);
        }
        if (timeout > 0) {
          if (clock_gettime(CLOCK_REALTIME, &time))
            err(SysError, "clock_gettime: %m.");
          time.tv_sec += timeout;
        } else timeout = -1; // nonblocking mode
        break;

      case 'T':
        if (timeout) warn("-%c overwrites previous setting.", o);
        if (!v) err(UserError, "-%c requires seconds since epoch.", o);
        timeout = strtol(v, &endptr, 10);
        if (*endptr || timeout <= 0)
          err(UserError, "-%c takes positive decimal integer.", o);
        time.tv_sec = timeout;
        break;
        
      case 'v':
        verbose++;
        if (v) {
          verbose = strtol(v, &endptr, 0);
          if (*endptr)
            err(UserError, "-%c takes optional integer value.", o);
        }
        break;
        
      case 'u':
        if (v) err(UserError, "-%c does not take arguments.", o);
        unlink = 1;
        break;
        
      case 'w':
        if (wait || post) warn("-%c overwrites previous setting.", o);
        if (v) err(UserError, "-%c does not take arguments.", o);
        wait = 1;
        break;
        
      case 'p':
        if (wait || post) warn("-%c overwrites previous setting.", o);
        if (v) err(UserError, "-%c does not take arguments.", o);
        post = 1;
        break;
        
      case 'x':
        if (wait || post) warn("-%c overwrites previous setting.", o);
        if (v) err(UserError, "-%c does not take arguments.", o);
        wait = 1;
        post = 1;
        break;

      case 'q':
        if (v) err(UserError, "-%c does not take arguments.", o);
        query = 1;
        break;

      case 'g':
        if (v) err(UserError, "-%c does not take arguments.", o);
        global = 1;
        break;

      case 'V':
        printf( "\n"
                "sem version %s\n"
                "    svnversion: %s\n"
                "compiled\n"
                "    date: %s\n"
                "    by: %s\n"
                ""
                "\n"
              , VERSION, SVNVERSION, COMPILED_DATE, COMPILED_BY
              );
        exit(EXIT_SUCCESS);
        break;

      case 'L':
        printf("\n%s\n",
#include "license.inc"
               );
        exit(EXIT_SUCCESS);
        break;

      default:
        err(UserError, "Unknown argument: -%c.", o);
      }
    }

    if (i < argc) err(ImplError, "Unhandled argument[%i]: %s.", i, argv[i]);

  } // end of arg processing



  // consistency checks.  Further checks may occur later on...

  if (!name) err(UserError, "No semaphore name given.");
  if (!wait && timeout) err(UserError, "Specified timeout, but not waiting.");


  
  // generate per-user, or global, or system name

  if (name[0]=='/') {
    if (global) err(UserError, "System name conflicts with ‘-g’.");
    realName = name;
  } else {
    char * out = calloc(maxNameLen, sizeof(char));
    if (!out) err(SysError, "calloc: %m.");
    out[0] = '/';
    int pos = 1;
    if (!global) {
      char * user = getenv("USER");
      if (!user) err(UserError, "Var $USER not in environment.  Try -g or -l.");
      pos += snprintf(out + pos, maxNameLen - pos, "%s:", user);
    }
    for (char const * c = name; *c; c++) {
      if (strchr(":/%?* ", *c))
        pos += snprintf(out + pos, maxNameLen - pos, "%%%0x", *c);
      else
        pos += snprintf(out + pos, maxNameLen - pos, "%c", *c); 
    }
    if (verbose > 2) warn("Real (system) name is \"%s\".", out);
    if (pos >= maxNameLen)
      err(UserError, "Real (system) name exceeds %i bytes.", maxNameLen - 1);
    realName = out;
  }

  if (strrchr(realName, '/') != realName)
    err(ImplError, "Invalid semaphore system name.");

  
  // open the semaphore
  
  if (wait || post || query || (oflag & O_CREAT)) {
    sem = sem_open(realName, oflag, mode, init);
    if (sem == SEM_FAILED) err(SysError, "sem_open: %m.");
    if (query) {
      int sval;
      if (sem_getvalue(sem, &sval)) err(SysError, "sem_getvalue: %m.");
      if (verbose > 0) out("value = %i", sval);
      else printf("%5i", sval);
    }
  } else {
    if (!unlink) warn("Pointless usage without semaphore operation.");
  }



  // do the waiting
  
  if (sem && wait) {
    if (timeout > 0) {
      if (verbose > 1) warn("Timed waiting...");
      if (sem_timedwait(sem, &time)) {
        if (errno == ETIMEDOUT) {
          if (verbose > 0) warn("Timeout expired.");
          exit(OkError);
        } else err(SysError, "sem_timedwait: %m.");
      }
    } else if (timeout < 0) { // nonblocking
      if (verbose > 1) warn("Trying to wait...");
      if (sem_trywait(sem)) {
        if (errno == EAGAIN) {
          if (verbose > 0) warn("Wait would block.");
          exit(OkError);
        } else err(SysError, "sem_trywait: %m.");
      }
    } else { // block forever
      if (verbose > 1) warn("Waiting...");
      if (sem_wait(sem)) err(SysError, "sem_wait: %m.");
    }
  }



  // daemonize
  
  if (daemon) {
    if (cmd) {
      pid = fork();
      if (pid > 0) {
        waitpid(pid, &status, 0);
        exit(EXIT_SUCCESS);
      }
      if (pid < 0) err(SysError, "fork: %m.");
      
      // fight zombies with a double fork
      pid = fork();
      if (pid > 0) exit(EXIT_SUCCESS);
      if (pid < 0) err(SysError, "fork: %m.");
    } else warn("Pointless usage of -f without command.");
  }



  // run the command

  if (cmd) {
    // replace the sem process with the command if there is nothing left to do.
    pid = (post||unlink) ? fork() : 0;

    if (pid < 0) err(SysError, "fork: %m.");

    if (pid == 0) {
      if (verbose > 2) warn("Exec'ing %s, with pid %i.", cmd[0], getpid());
      execvp(cmd[0], cmd);
      err(SysError, "execvp(%s): %m.", cmd[0]);
    }

    // all the following stuff is run in the parent process, iff we have forked.

    { // set up signal handling
      //sigset_t  sigmask;
      struct sigaction action;
      int const * sig;

      sigfillset(&action.sa_mask);
      action.sa_flags = SA_RESTART;
      action.sa_handler = SIG_IGN;
      for (sig = sig_ign; *sig; sig++)
        if (sigaction(*sig, &action, 0) != 0) err(SysError, "sigaction: %m.");

      action.sa_handler = &sig_forward;
      for (sig = sig_fwd; *sig; sig++)
        if (sigaction(*sig, &action, 0) != 0) err(SysError, "sigaction: %m.");
    }
    
    if (verbose > 1) warn("Waiting for process %i.", pid);
    errno = EINTR;
    waitpid(pid, &status, 0);
    if (verbose > 2) {
      if (WIFEXITED(status))
        warn("Child %i exited with code %d.", pid, WEXITSTATUS(status));
      else if (WIFSIGNALED(status))
        warn("Child %i killed by signal %d.", pid, WTERMSIG(status));
      else
        warn("Child %i terminated due to unknown reason.", pid);
    }
  }



  // post semaphore
  
  if (sem && post) {
    if (sem_post(sem)) err(SysError, "sem_post: %m.");
    if (verbose > 1) warn("Posted.");
  }



  // unlink semaphore
  
  if (unlink) {
    if (sem_unlink(realName)) {
      if (verbose > 0) warn("sem_unlink: %m.");
    } else {
      if (verbose > 1) warn("Removed semaphore.");
    }
  }



  // close semaphore
  
  if (sem && sem_close(sem)) warn("sem_close: %m.");

  if (WIFEXITED(status)) return WEXITSTATUS(status);
  if (WIFSIGNALED(status)) return KilledError + offset;
  
  return EXIT_SUCCESS + offset;
}
