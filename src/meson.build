cxx = meson.get_compiler('cpp')

libyang = dependency('libyang')
libyang_cpp = dependency('libyang-cpp')
libsysrepo = dependency('sysrepo')
libsysrepo_cpp = dependency('sysrepo-cpp')

libsensors = dependency('libsensors', required : false)
if not libsensors.found()
    libsensors = cxx.find_library('libsensors', required : true)
endif

inc = include_directories('utils')
shared_library('ietf-hardware-plugin', 'ietf-hardware-plugin.cc',
                include_directories : inc,
                dependencies : [libyang, libyang_cpp, libsysrepo, libsysrepo_cpp, libsensors],
                install : true,
                install_dir : '/' / get_option('sysrepo-prefix') / 'lib/sysrepo/plugins',
                override_options : ['cpp_std=c++17'])