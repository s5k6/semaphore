
/* This tool is used only to test the signal forwarding capabilities of
  `sem`.  `killme` tries to handle all signals, and prints out what it
  catches.  You'll have to shoot it to death with -9.


  Use this to test `killme`

      ./sem foo -p -v3 -f -- ./killme

      pid= pid of killme

      kill -0 "$pid" && for i in $(seq 1 64); do
        case "$i" in
          9|19|32|33) echo "skipping $i";;
          *) echo "sending $i"; kill -"$i" "$pid"; sleep 0.3; ;;
        esac;
      done;
      kill -9 "$pid";

    or
    
      pid= pid of sem

      kill -0 "$pid" && for i in SIGHUP SIGINT SIGQUIT SIGTERM SIGUSR1 SIGUSR2 SIGCONT SIGTSTP SIGTTIN SIGTTOU; do
         kill -"$i" "$pid"; sleep 0.3;
      done;

      

  Use this to test `sem`

      ./sem foo -i1 -t -x -v3 -- ./killme

*/

// Feature Test Macro Requirements
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>


int pid = 0;


void sig_handler(int const sig) {
  printf("killme[%i]: caught signal %i = %s\n", pid, sig, strsignal(sig));
}


int main(int argc, char ** argv) {

  struct sigaction action;
  int sig;

  pid = getpid();

  printf("killme: I'll try to install handlers for all signals.\n"
         "killme: You have to `kill -9 %i`.\n", pid);

  sigfillset(&action.sa_mask);
  action.sa_handler = &sig_handler;
  for (sig = 1; sig < 65; sig++)
    if (sigaction(sig, &action, 0) != 0)
      printf("killme[%i]: sigaction(%i): %m\n", pid, sig);
  
  printf("killme[%i]: running\n", pid);

  errno = EINTR;
  while (errno == EINTR) pause();

  printf("killme[%i]: %m\n", pid);
  
  return 0;
}
