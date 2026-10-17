project('kei-utils', 'cpp',
        default_options: [ 'cpp_std=c++23', 'warning_level=3' ])


include = include_directories('include')
kei_utils = declare_dependency(include_directories: include)
meson.override_dependency('kei-utils', kei_utils)

