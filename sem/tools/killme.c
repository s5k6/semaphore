
/* This tool prints a message for each signal it is able to catch. */

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
