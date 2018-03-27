project('fontview', 'c', version : '0.10')

gnome = import('gnome')
i18n = import('i18n')

subdir('po')

conf = configuration_data()
conf.set_quoted('PACKAGE', meson.project_name())
conf.set_quoted('VERSION', meson.project_version())
conf.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
configure_file(
  output: 'config.h',
  configuration: conf
)

gtk = dependency('gtk+-3.0', version : '>= 3.12.0')
freetype = dependency('freetype2', version : '>= 22.0.16')
harfbuzz = dependency('harfbuzz', version : '>= 0.6.0')
pangoft = dependency('pangoft2', version : '>= 1.41.1')
deps = [gtk, freetype, harfbuzz, pangoft]

resources = gnome.compile_resources(
  'fontview-resources', 'fontview.gresource.xml',
  source_dir : '.',
  c_name : 'fontview'
)

fontview = executable(
  meson.project_name(),
  'font-model.c', 'font-view.c', 'main.c',
  resources,
  dependencies: deps,
  install: true
)

desktop = 'fontview.desktop'
i18n.merge_file(
  desktop,
  type: 'desktop',
  input: desktop + '.in',
  output: desktop,
  po_dir: 'po',
  install: true,
  install_dir: join_paths(get_option('prefix'), get_option('datadir'), 'applications')
)
