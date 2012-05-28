
// Feature Test Macro Requirements
#define _POSIX_C_SOURCE 200112L

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
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



/* All of sem's exit codes may be offset, to disambiguate from the
   command's error. */
int offset = 0;



void exit_offset(int const ec) {
  exit(ec + offset);
}

#define exit exit_offset



char * name = 0;



/* Sorry, I'm using my own ‘warn’ and ‘err’ functions, since I want to
   add sem's pid and the semaphore name in the output as well. */

void vwarn(const char * fmt, va_list ap) {
  if (name) fprintf(stderr, "sem[%i,%s] ", getpid(), name);
  else fprintf(stderr, "sem[%i] ", getpid());
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n"); 
}

void warn(const char * fmt, ...) {
  va_list ap;
  va_start(ap, fmt); vwarn(fmt, ap); va_end(ap);
}

void err(int const ec, const char * fmt, ...) {
  va_list ap;
  va_start(ap, fmt); vwarn(fmt, ap); va_end(ap);
  exit(ec);
}



/* Sorry again: I consider the argument processing implemented her
   superior to the GNU standard.  This is a feature, not a bug.

   Arguments for opt(argc, argv, &i, &o, &v):

     - arc, argv from main(argc, argv)

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
  
  char ** cmd = 0
     , * realName = 0 // system name of semaphore
     ;
  int daemon = 0 // fork for command execution
    , global = 0 // do not prefix with user name
    , literally = 0 // use name without encoding
    , oflag = 0 // do not create new semaphore
    , post = 0 // post after command
    , query = 0 // print semaphore value
    , status = 0 // exit status of command
    , timeout = 0 // timeout in seconds
    , unlink = 0 // rm semaphore after wait
    , verbose = 0 // print info to stdout
    , wait = 0 // wait before command
    ;
  unsigned int init = 0; // initial value for creation
  mode_t mode = 0600; // rw- for user alone
  sem_t * sem = 0;
  struct timespec time = { 0, 0 }; // holds absolute timeout time
  pid_t pid;

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
        if (oflag) warn("-%c overwrites previous setting.", o);
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
        if (v) err(UserError, "-%c does not take arguments.", o);
        literally = 1;
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
        puts(
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


  
  // generate per-user or global name

  if (literally) realName = name;
  else {
    realName = calloc(maxNameLen, sizeof(char));
    if (!realName) err(SysError, "calloc: %m.");
    realName[0] = '/';
    int pos = 1;
    if (!global) {
      char * user = getenv("USER");
      if (user) {
        pos += snprintf(realName + pos, maxNameLen - pos, "%s:", user);
      } else {
        uid_t uid = geteuid();
        warn("Variable USER unset, using uid=%i.", uid);
        pos += snprintf(realName + pos, maxNameLen - pos, "%i:", uid);
      }
    }
    for (char * c = name; *c; c++) {
      if (index(":/%?* ", *c))
        pos += snprintf(realName + pos, maxNameLen - pos, "%%%0x", *c);
      else
        pos += snprintf(realName + pos, maxNameLen - pos, "%c", *c); 
    }
    if (verbose > 1) warn("Real (system) name is \"%s\".", realName);
    if (pos >= maxNameLen)
      err(UserError, "Real (system) name exceeds %i bytes.", maxNameLen - 1);
  }

  if (rindex(realName, '/') != realName)
    err(UserError, "First character must be the slash in real semaphore name.");

  
  // open the semaphore
  
  if (wait || post || query || (oflag & O_CREAT)) {
    sem = sem_open(realName, oflag, mode, init);
    if (sem == SEM_FAILED) err(SysError, "sem_open: %m.");
    if (query) {
      int sval;
      if (sem_getvalue(sem, &sval)) err(SysError, "sem_getvalue: %m.");
      warn("value is %i.", sval);
    }
  } else {
    if (!unlink) warn("Pointless usage without semaphore operation.");
  }



  // do the waiting
  
  if (sem && wait) {
    if (timeout > 0) {
      if (verbose > 0) warn("Timed waiting...");
      if (sem_timedwait(sem, &time)) {
        if (errno == ETIMEDOUT) {
          if (verbose>0) warn("Timeout expired.");
          exit(OkError);
        } else err(SysError, "sem_timedwait: %m.");
      }
    } else if (timeout < 0) { // nonblocking
      if (verbose > 0) warn("Trying to wait...");
      if (sem_trywait(sem)) {
        if (errno == EAGAIN) {
          if (verbose>0) warn("Wait would block.");
          exit(OkError);
        } else err(SysError, "sem_trywait: %m.");
      }
    } else { // block forever
      if (verbose > 0) warn("Waiting...");
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
      if (verbose > 1) warn("Executing %s, with pid %i.", cmd[0], getpid());
      execvp(cmd[0], cmd);
      err(SysError, "execvp(%s): %m.", cmd[0]);
    }

    // all the following stuff is run in the parent process, iff we have forked.
      
    if (verbose > 0) warn("Waiting for process %i.", pid);
    waitpid(pid, &status, 0);
    if (verbose > 1) {
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
    if (verbose > 0) warn("Posted.");
  }



  // unlink semaphore
  
  if (unlink) {
    if (sem_unlink(realName)) {
      if (verbose > 0) warn("sem_unlink: %m.");
    } else {
      if (verbose > 0) warn("Removed semaphore.");
    }
  }



  // close semaphore
  
  if (sem && sem_close(sem)) warn("sem_close: %m.");

  if (WIFEXITED(status)) return WEXITSTATUS(status);
  if (WIFSIGNALED(status)) {
    warn("Command signalled with %i.", WTERMSIG(status));
    return KilledError + offset;
  }
  
  return EXIT_SUCCESS + offset;
}
