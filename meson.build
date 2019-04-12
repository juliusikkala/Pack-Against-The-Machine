project(
  'patm',
  ['cpp', 'c'],
  default_options : [
    'cpp_std=c++1z',
    'buildtype=debugoptimized',
    'warning_level=3',
    'werror=true'
  ]
)

src = [
  'main.cc',
  'rect_packer.cc',
  'board.cc',
]

cc = meson.get_compiler('cpp')
m_dep = cc.find_library('m', required : false)
sfml_dep = dependency('sfml-all')

executable(
  'patm',
  src,
  dependencies: [
    sfml_dep,
    m_dep
  ],
  install: true,
)

