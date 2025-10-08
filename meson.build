project('ctoolbox', 'c', default_options: ['c_std=c99', 'warning_level=3'], version: run_command('tools/version.py', check: true).stdout().strip(), meson_version: '>=1.3.0')

subdir('ctoolbox/config')

if get_option('buildtype').startswith('release')
  add_project_arguments('-DNDEBUG', language : 'c')
endif


cc = meson.get_compiler('c')
if cc.get_id() == 'msvc'
  add_project_arguments('-D__MSVC__', language: 'c')
else
  if cc.get_id() == 'gcc' or cc.get_id() == 'clang'
    compilerflags = [
      '-Wconversion',
      '-Wcast-qual',
      '-Wunused-parameter',
      '-Wshadow',
      '-Wswitch',
      '-Wcast-align',
      '-Wdouble-promotion'
    ]
    add_project_arguments(compilerflags, language: 'c')
  endif
endif


projectsources = []

# Source files
projectsources += [
  'src/str2int.c',
  'src/str2flt.c',
  'src/memory.c',
  'src/flt2str.c',
  'src/ctype.c',
  'src/int2str.c',
  'src/assert.c',
  'src/xoshiro.c',
]

headerfiles = []


newline = '''
'''

base = 'ctoolbox'
r = run_command('tools/listfiles.py', base, '.h', check: true).stdout()
foreach file: r.split(newline)
  if not (file == '')
    headerfiles += [file]
  endif
endforeach

base = 'ctoolbox/private'
r = run_command('tools/listfiles.py', base, '.h', check: true).stdout()
foreach file: r.split(newline)
  if not (file == '')
    headerfiles += [file]
  endif
endforeach


largs = []

both_builds = both_libraries(meson.project_name(), sources: projectsources, pic: true, install: true, c_shared_args: ['-DCTB_BUILDSHARED'])

lib = declare_dependency(
  link_with: both_builds, include_directories: include_directories('.'), link_args: largs
)
ctoolbox_dep = lib


pkg = import('pkgconfig')
pkg.generate(libraries: [ctoolbox_dep], version: meson.project_version(), name: meson.project_name(), filebase: meson.project_name(), description: 'A small base library for other projects')

install_headers(headerfiles, preserve_path: true)
