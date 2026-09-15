project(
  'voxelgame',
  'c',
  meson_version : '>= 1.3.0',
  version : '0.1',
  default_options : ['warning_level=3'],
)

dependencies = [
]

sources = [
  'voxelgame.c',
]

exe = executable(
  'voxelgame',
  sources,
  dependencies : dependencies,
  install : true,
)

test('basic', exe)
