project('coop', 'cpp', version : '1.2.4', default_options : ['warning_level=3', 'werror=false', 'cpp_std=c++23'])
cpp_compiler = meson.get_compiler('cpp')
if cpp_compiler.get_id() == 'msvc'
  add_project_arguments(['/W3', '/DNOMINMAX'], language: 'cpp')
else
  add_project_arguments('-Wfatal-errors', language: 'cpp')
  add_project_arguments('-Wno-missing-field-initializers', language: 'cpp')
endif

deps = []
if host_machine.system() == 'windows'
  cpp = meson.get_compiler('cpp')
  deps += [
    cpp.find_library('ws2_32'),
  ]
endif

coop = declare_dependency(dependencies : deps, include_directories: include_directories('include'))

install_subdir('include/coop', install_dir : 'include')

pkgconfig = import('pkgconfig')
pkgconfig.generate(
  description: 'C++ asynchronous coroutine runtime',
  name : 'coop',
)

if get_option('examples')
  executable('example', ['example/main.cpp'], dependencies : coop)
  executable('pipe-test', ['example/pipe-test.cpp'], dependencies : coop)
endif
