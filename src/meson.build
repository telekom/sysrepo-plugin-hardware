libyang = dependency('libyang')
libsysrepo = dependency('sysrepo')
libsysrepo_cpp = dependency('sysrepo-cpp')
shared_library('ietf-hardware-plugin', 'ietf-hardware-plugin.cc',
                dependencies : [libyang, libsysrepo, libsysrepo_cpp],
                install : true,
                install_dir : '/' / get_option('sysrepo-prefix') / 'lib/sysrepo/plugins')