project(
  'voxelgame',
  'c',
  meson_version : '>= 1.3.0',
  version : '0.1',
  default_options : [
    'warning_level=3'
  ],
)

cc = meson.get_compiler('c')

c_args = []
cpp_args = []
link_args = []
name_suffix = []
dependencies = [
  dependency('enet'),
  dependency('stc'),
]

inc_dir = include_directories('src')

sources = [
  'src/main.c',
]

# Raylib configuration
cmake = import('cmake')
raylib_opts = cmake.subproject_options()
raylib_opts.set_install(false)
raylib_opts.add_cmake_defines({
  'BUILD_EXAMPLES': 'OFF',
  # Add other CMake options for Raylib, if needed
})
if cc.get_id() == 'emscripten'
  raylib_opts.add_cmake_defines({
    'PLATFORM': 'Web',
  })
else
  raylib_opts.add_cmake_defines({
    'PLATFORM': 'Desktop',
  })
endif
raylib_subproject = cmake.subproject('raylib', options: raylib_opts)
dependencies += raylib_subproject.dependency('raylib')

# Box3D configuration
box3d_opts = cmake.subproject_options()
box3d_opts.set_install(false)
box3d_opts.add_cmake_defines({
  'BOX3D_SAMPLES': 'OFF',
  'BOX3D_BENCHMARKS': 'OFF',
  'BOX3D_DOCS': 'OFF',
  'BOX3D_UNIT_TESTS': 'OFF',
})
box3d_subproject = cmake.subproject('box3d', options: box3d_opts)
dependencies += box3d_subproject.dependency('box3d')

# General configuration
if host_machine.system() == 'windows'
  dependencies += [
    cc.find_library('winmm'),
  ]
elif host_machine.system() == 'darwin'
  link_args += [
    '-framework', 'AppKit',
    '-framework', 'IOKit',
  ]
elif host_machine.system() == 'linux'
  dependencies += [
    cc.find_library('m'),
    cc.find_library('dl'),
  ]
elif host_machine.system() == 'emscripten'
  link_args += [
    '-s', 'ENVIRONMENT=web',
    '-s', 'USE_GLFW=3',
  ]
  name_suffix = 'html'
endif

dependencies += dependency('raygui')

exe = executable(
  'voxelgame',
  sources,
  c_args: c_args,
  cpp_args: cpp_args,
  dependencies: dependencies,
  include_directories: inc_dir,
  link_args: link_args,
  name_suffix: name_suffix,
)

test('basic', exe)
