project('ctoolbox', 'c', default_options: ['c_std=c99', 'warning_level=3'], version: run_command('tools/version.py', check: true).stdout().strip(), meson_version: '>=1.1.0')


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
  'src/version.c',
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

if meson.is_subproject()
  extraargs = []
  if get_option('default_library') == 'shared'
    lib = shared_library(meson.project_name(), sources: projectsources, soversion: versionarray[0], c_args: ['-DCTOOLBOX_BUILDDLL', '-DCTOOLBOX_DLL'])
    if target_machine.system() in ['windows', 'cygwin']
      extraargs += ['-DCTOOLBOX_DLL']
    endif
  endif
  if get_option('default_library') == 'static'
    lib = static_library(meson.project_name(), sources: projectsources, pic: true)
  endif

  if not is_variable('lib')
    error('default_library is not shared or static')
  endif
  ctoolbox_dep = declare_dependency(
    link_with: lib, include_directories: include_directories('.'), compile_args: extraargs
  )
else
  ctoolbox_shared = shared_library(meson.project_name(), sources: projectsources, install: true, soversion: versionarray[0], c_args: ['-DCTOOLBOX_BUILDDLL', '-DCTOOLBOX_DLL'])
  ctoolbox_static = static_library(meson.project_name(), sources: projectsources, install: true, pic: true)

  ctoolbox_dep = declare_dependency(
    link_with: [ctoolbox_static, ctoolbox_shared], include_directories: include_directories('.')
  )

  pkg = import('pkgconfig')
  pkg.generate(libraries: ctoolbox_dep, version: meson.project_version(), name: meson.project_name(), filebase: meson.project_name(), description: 'A small base library for other projects')

  install_headers(headerfiles, preserve_path: true)
endif
