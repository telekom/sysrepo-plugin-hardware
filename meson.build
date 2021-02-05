project('ietf-hardware-plugin', 'cpp', default_options: ['cpp_std=c++2a'], version: run_command('./get-version').stdout().strip(), license: 'Apache')
install_subdir('yang', install_dir : get_option('prefix'))
subdir('./src')