project('lid_monitor', 'c',
  version : '0.1',
  default_options : ['warning_level=3'])

dep_libudev = dependency('libudev')
dep_libinput = dependency('libinput')

exe = executable('lid_monitor', 'lid_monitor.c',
  dependencies : [dep_libudev, dep_libinput], install : true)

test('basic', exe)
