//************
//
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau
//
//************

// Catching Ctrl+C
//
// Installs a handler for SIGINT instead of dying, then waits in pause().
// Uncomment the lines inside the handler to see re-arming and SIG_DFL.

#include <unistd.h>  // For functions pause and alarm
#include <signal.h>  // For constants SIGALRM and similar
#include <stdio.h>

// This functions is the one used as handler of the SIGINT signal
void farewell(int signum __attribute__((unused))) {
    write(STDOUT_FILENO, "------------------------\n", 25);
    write(STDOUT_FILENO, "Why do you interrupt me?\n", 25);
    write(STDOUT_FILENO, "------------------------\n", 25);
    // signal(SIGINT, farewell);
    // raise(SIGTERM);
    // signal(SIGINT, SIG_DFL);

    write(STDOUT_FILENO, ">End of handler\n", 16);
}

int main() {
    // Assign the signal SIGINT with the function "farewell"
    signal(SIGINT, farewell);

    // Start an infinite loop that will display the message "Nothing new here"
    while (1) {
        write(STDOUT_FILENO, "Nothing new here.\n", 18);
        pause();
    }
    write(STDOUT_FILENO, "See you!\n", 9);
}