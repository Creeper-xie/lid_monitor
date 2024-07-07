// lid_monitor -- a program to record state changes of your lid
// Copyright (C) 2024  Sniventals
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <argp.h>
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

const char* argp_program_version = "lib_monitor 1.0";
const char* argp_program_bug_address = "<mingfengpigeon@gmail.com>";

static char doc[] = "lib_monitor -- a program to record you lid";
static struct argp argp = {0, 0, NULL, doc};

// Connect to database with given path and execute given SQL. Create database
// if not exists. Open and close connection every times. When connection or
// execution fails, close connection and return 1.
static int sqlite_execute(char *path, char *sql) {
    sqlite3 *db;
    if (sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX, NULL) != SQLITE_OK) {
        fprintf(stderr, "unable to open database");
        sqlite3_close(db);
        return 1;
    }
    char *err;
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "failed to insert value: %s",  err);
        sqlite3_close(db);
        return 1;
    }
    sqlite3_close(db);
    return 0;
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

// Handle switch lid event. Record to database.
static void handle_switch_lid_event(struct libinput_event_switch *sw) {
    char *sql = sqlite3_mprintf(
        "INSERT INTO lid_switch_events(created, lid_state) VALUES (%d, %d)",
        (unsigned long) time(NULL), libinput_event_switch_get_switch_state(sw));
    sqlite_execute("./lid_monitor.sqlite3", sql);
}

// Handle switch event. Call other function to handle details.
static void handle_switch_event(struct libinput_event *ev) {
    struct libinput_event_switch *sw = libinput_event_get_switch_event(ev);
    switch (libinput_event_switch_get_switch(sw)) {
        case LIBINPUT_SWITCH_LID:
            handle_switch_lid_event(sw);
        default:
            break;
    }
}

// Receive events from given libinput context. Call other function to handle
// diffencent types of events.
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

// Parse command line arguments. Create table if not exists. Initialize udev
// and libinput. Wait events and call `handle_events` to receive and handle
// events.
int main(int argc, char **argv) {
    argp_parse(&argp, argc, argv, 0, 0, 0);

    char *sql = "CREATE TABLE IF NOT EXISTS lid_switch_events ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "created INTEGER NOT NULL,"
            "lid_state INTEGER CHECK(lid_state IN (0, 1)));";
    sqlite_execute("./lid_monitor.sqlite3", sql);

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
