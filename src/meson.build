libyang = dependency('libyang')
libsysrepo = dependency('sysrepo')
libsysrepo_cpp = dependency('sysrepo-cpp')
inc = include_directories('utils')
shared_library('ietf-hardware-plugin', 'ietf-hardware-plugin.cc',
                include_directories : inc,
                dependencies : [libyang, libsysrepo, libsysrepo_cpp],
                install : true,
                install_dir : '/' / get_option('sysrepo-prefix') / 'lib/sysrepo/plugins')