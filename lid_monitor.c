#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <stdbool.h>

#include <libudev.h>
#include <libinput.h>

static int open_restricted(const char *path, int flags, void *user_data) {
    int fd = open(path, flags);
    if (fd < 0) {
        perror("Failed to open device");
    }
    return fd;
}

static void close_restricted(int fd, void *user_data) {
    close(fd);
}

static const struct libinput_interface interface = {
    .open_restricted = open_restricted,
    .close_restricted = close_restricted,
};

void handle_switch_event(struct libinput_event *ev) {
    struct libinput_event_switch *sw = libinput_event_get_switch_event(ev);
    switch (libinput_event_switch_get_switch(sw)) {
        case LIBINPUT_SWITCH_LID:
            printf("state %d\n", libinput_event_switch_get_switch_state(sw));
        default:
            break;
    }
}

void handle_events(struct libinput *li) {
    struct libinput_event *ev;

    libinput_dispatch(li);
    while ((ev = libinput_get_event(li))) {
        switch (libinput_event_get_type(ev)) {
            case LIBINPUT_EVENT_SWITCH_TOGGLE:
                handle_switch_event(ev);
                break;
            default:
                break;
        }
        libinput_event_destroy(ev);
    }
}

int main(void) {
    struct udev *udev = udev_new();
    struct libinput *li = libinput_udev_create_context(&interface, NULL, udev);

    libinput_udev_assign_seat(li, "seat0");
    
    struct pollfd fds;
    fds.fd = libinput_get_fd(li);
    fds.events = POLLIN;
    fds.revents = 0;

    for (;;) {
        if (poll(&fds, 1, -1) > -1) {
            handle_events(li);
        }
    }

    libinput_unref(li);
    udev_unref(udev);

    return 0;
}
