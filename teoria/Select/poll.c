//************
// 
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau 
//
//************

#include <stdio.h>
#include <poll.h>
#include <unistd.h>

/*
    struct pollfd {
        int   fd;         file descriptor
        short events;     requested events 
        short revents;    returned events 
    };
*/

int main() {
    struct pollfd fds[1];

    fds[0].fd = 0;          // Add file descriptor 0 (stdin)
    fds[0].events = POLLIN; // We're interested in data being available for reading

    while (1) {
        int ret = poll(fds, 1, 1000);

        if (ret == 0) {
            printf("No input after 1 second\n");
        } else {
            if (fds[0].revents & POLLIN) {
                char buffer[128];
                read(0, buffer, sizeof(buffer));
                printf("Input received: %s", buffer);
            }
        }
    }

    return 0;
}
