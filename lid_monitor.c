#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <stdbool.h>

#include <sqlite3.h>
#include <libudev.h>
#include <libinput.h>

const char *PATH = "./lid_monitor.sqlite3";

void sqlite_execute(char *path, char *sql) {
    sqlite3 *db;
    if (sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX, NULL) != SQLITE_OK) {
        fprintf(stderr, "unable to open database");
        abort();
    }
    char *err;
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "failed to insert value: %s",  err);
        abort();
    };
    sqlite3_close(db);
}

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

static void handle_switch_event(struct libinput_event *ev) {
    struct libinput_event_switch *sw = libinput_event_get_switch_event(ev);
    switch (libinput_event_switch_get_switch(sw)) {
        case LIBINPUT_SWITCH_LID:
            sqlite_execute(
                PATH,
                sqlite3_mprintf(
                    "INSERT INTO lid_switch_events(created, lid_state) VALUES (%d, %d)",
                    (unsigned long)time(NULL), libinput_event_switch_get_switch_state(sw)
                )
            );
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
    char *sql = "CREATE TABLE IF NOT EXISTS lid_switch_events ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "created INTEGER NOT NULL,"
            "lid_state INTEGER CHECK(lid_state IN (0, 1)));";
    sqlite_execute(PATH, sql);
    
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

    return EXIT_SUCCESS;
}
