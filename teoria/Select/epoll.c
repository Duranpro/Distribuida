//************
// 
// Operating Systems Code Examples
// author: Xavier Canaleta, Aaron Jeriel, Jordi Malé, Victor Xirau 
//
//************


#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>


/*


Always use `poll` unless all of the following conditions are met:
- You're sure the system (Linux) has support for `epoll`
- Minimum 1000-10000 fds to monitor
- The set of file descriptors is stable over a long period of time
- Adding or removing descriptors from the `epoll` list is as expensive as a `poll` operation
    - involves changes to kernel space.
*/

int main() {
    int epfd = epoll_create1(0);
    if (epfd == -1) {
        //Error
        return 1;
    }

    struct epoll_event event;
    struct epoll_event events[1];

    event.events = EPOLLIN;  // Input events
    event.data.fd = 0;       // File descriptor for stdin (0)

    // afegir stdin
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, 0, &event) == -1) {
        //Error
        close(epfd);
        return 1;
    }

    while (1) {
        int ret = epoll_wait(epfd, events, 1, 1000); // Wait for events. 1s timeout

        if (ret == 0) {
            printf("No input after 1 second\n");
        } else if (ret > 0) {

            if (events[0].events & EPOLLIN) {
                char buffer[128];
                int bytes_read = read(0, buffer, sizeof(buffer));
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    printf("Input received: %s", buffer);
                }
            }

        } else {
            //Error epoll
            break;
        }
    }

    close(epfd);
    return 0;
}
