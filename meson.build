project('asha-support', 'c',
        version : '0.1',
        license : 'MIT')

src = ['src/asha_support.c',
        'src/dbus.c',
        'src/bluez.c',
        'src/loop.c',
        'src/streaming.c']
incdir = include_directories('include')

pthread_lib = dependency('threads')
dbus_dep = dependency('dbus-1')
bluez_dep = dependency('bluez')

executable('asha-support',
        sources: src,
        include_directories: incdir,
        dependencies: [dbus_dep, pthread_lib, bluez_dep])
