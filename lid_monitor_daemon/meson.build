dep_libudev = dependency('libudev')
dep_libinput = dependency('libinput')
dep_sqlite3 = dependency('sqlite3')

exe = executable('lid_monitor_daemon', 'lid_monitor_daemon.c',
  dependencies : [dep_libudev, dep_libinput, dep_sqlite3], install : true)

test('basic', exe)
