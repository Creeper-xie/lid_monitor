#include <stdio.h>
#include <stdlib.h>
#include <libinput.h>
#include <fcntl.h>
#include <unistd.h>
#include <libudev.h>
#include <sys/select.h>

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

static const struct libinput_interface libinput_interface = {
    .open_restricted = open_restricted,
    .close_restricted = close_restricted,
};

static void handle_pointer_motion(struct libinput_event_pointer *pointer_event) {
    double dx = libinput_event_pointer_get_dx(pointer_event);
    double dy = libinput_event_pointer_get_dy(pointer_event);
    printf("Pointer moved by %f, %f\n", dx, dy);
}

int main(void) {
    struct udev *udev = udev_new();
    struct libinput *li = libinput_udev_create_context(&libinput_interface, NULL, udev);

    libinput_udev_assign_seat(li, "seat0");
    
    int fd = libinput_get_fd(li);
    while (1) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        if (select(fd + 1, &fds, NULL, NULL, NULL) > 0) {
            libinput_dispatch(li);
            struct libinput_event *event;
            while ((event = libinput_get_event(li)) != NULL) {
                if (libinput_event_get_type(event) == LIBINPUT_EVENT_POINTER_MOTION) {
                    struct libinput_event_pointer *pointer_event = libinput_event_get_pointer_event(event);
                    handle_pointer_motion(pointer_event);
                }
                libinput_event_destroy(event);
            }
        }
    }

    libinput_unref(li);
    udev_unref(udev);

    return 0;
}