#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libudev.h>
#include <libinput.h>

#define PROJECT_NAME "lid_monitor"

static int open_restricted(const char *path, int flags, void *user_data) {
    int fd = open(path, flags);
    return fd < 0 ? -errno : fd;
}

static void close_restricted(int fd, void *user_data) {
    close(fd);
}

static const struct libinput_interface interface = {
    .open_restricted = open_restricted,
    .close_restricted = close_restricted,
};

int main(void) {
    struct libinput *li;
    struct libinput_event *event;
    //struct libinput_device *device;
    struct udev *udev;

    udev = udev_new();
    if (!udev) {
        fprintf(stderr, "failed to initialize udev");
        return 0;
    }


    li = libinput_udev_create_context(&interface, NULL, udev);

    if (libinput_udev_assign_seat(li, "seat0") != 0) {
        fprintf(stderr, "failed to assign seat");
    }
    
    // li = libinput_path_create_context(&interface, NULL);
    // device = libinput_path_add_device(li, "/dev/input/event1");
    // libinput_device_ref(device);
    libinput_dispatch(li);
    while ((event = libinput_get_event(li)) != NULL) {
        if (libinput_event_get_type(event) == LIBINPUT_EVENT_SWITCH_TOGGLE) {
            
        }
        libinput_event_destroy(event);
        libinput_dispatch(li);
    }
    libinput_unref(li);
    return 0;
}