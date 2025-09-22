project('ctoolbox', 'c', default_options: ['c_std=c99', 'warning_level=3'], version: run_command('python', join_paths(meson.current_source_dir(), 'tools/version.py'), check: true).stdout().strip(), meson_version: '>=1.1.0')

# major, minor and path
versionarray = [0, 0, 0]

mmp = meson.project_version().split('.')
if mmp.length() == 3
  versionarray = [
    mmp[0].to_int(),
    mmp[1].to_int(),
    mmp[2].to_int()
  ]
endif


cc = meson.get_compiler('c')
if cc.get_id() == 'msvc'
  add_project_arguments('-D__MSVC__', language: 'c')
endif


# Configuration
conf = configuration_data()

# build the version string
v0 = versionarray[0]
v1 = versionarray[1]
v2 = versionarray[2]
version = f'@v0@.@v1@.@v2@'


conf.set('version', version)

conf.set('version_major', v0)
conf.set('version_minor', v1)
conf.set('version_rpath', v2)


# CPU configuration
if target_machine.endian() == 'little'
  conf.set('CTB_CFG_LITTLEENDIAN', true)
else
  if target_machine.endian() == 'big'
    conf.set('CTB_CFG_BIGENDIAN', true)
  else
    error()
  endif
endif

target = target_machine.cpu_family()
conf.set('CTB_CFG_ENV64', target.contains('64'))
if target.contains('x86')
  conf.set('CTB_CFG_FASTUNALIGNED', true)
else
  conf.set('CTB_CFG_STRICTALIGNMENT', true)
endif


# Header support
if not cc.has_header('stdbool.h', required: false)
  if cc.get_id() == 'msvc'
    conf.set('CTB_CFG_NOSTDBOOL', true)
  else
    error('missing header stdbool.h, compiler not supported')
  endif
endif

if not cc.has_header('stdint.h', required: false)
  if cc.get_id() == 'msvc'
    conf.set('CTB_CFG_NOSTDINT', true)
  else
    error('missing header stdint.h, compiler not supported')
  endif
endif

conf.set('CTB_CFG_NOSTDLIB', get_option('no_stdlib'))


isunix    = false
iswindows = false


# System platform
if target_machine.system() == 'windows'
  conf.set('CTB_CFG_PLATFORM_WINDOWS', true)
  iswindows = true
else
  if target_machine.system() == 'haiku'
    conf.set('CTB_CFG_PLATFORM_BEOS', true)
  else
    conf.set('CTB_CFG_PLATFORM_UNIX', true)
    isunix = true
  endif
endif


# Checked int
externalckdintasm = false
ckdintfailback    = false
if cc.has_header('stdckdint.h', required: false)
  conf.set('CTB_CFG_HAS_STDCKDINT', true)
else
  supportedcc = [
    'gcc',
    'clang',
    'armclang',
    'clang-cl',
    'intel',
    'intel-cl',
    'intel-llvm',
    'intel-llvm-cl'
  ]

  if supportedcc.contains(cc.get_id())
    hasckdbuiltin = true
    if cc.has_function('__builtin_add_overflow') == false
      hasckdbuiltin = false
    endif
    if cc.has_function('__builtin_sub_overflow') == false
      hasckdbuiltin = false
    endif
    if cc.has_function('__builtin_mul_overflow') == false
      hasckdbuiltin = false
    endif

    if hasckdbuiltin
      conf.set('CTB_CFG_HAS_CKDINT_INTRINSICS', true)
    endif
  endif
endif

if ckdintfailback
  message('checked int not supported, using fallback')
endif


python = find_program('python3')
script = join_paths(meson.current_source_dir(), 'tools/listfiles.py')
newline = '''
'''


projectsources = []
externalobjs   = []

base = 'src'
r = run_command(python, script, base, '.c', check: true).stdout()
foreach file: r.split(newline)
  if not (file == '')
    projectsources += [file]
  endif
endforeach

base = 'src/crypto'
r = run_command(python, script, base, '.c', check: true).stdout()
foreach file: r.split(newline)
  if not (file == '')
    projectsources += [file]
  endif
endforeach


if target == 'x86_64'
  supported = false

  path = 'src/crypto/asm'
  if isunix
    supported = true
  endif
  if iswindows
    supported = true
  endif

  if supported
    if add_languages('nasm', required: false, native: false)
      if isunix
        if host_machine.system() in ['darwin', 'ios', 'tvos']
          nasmformat = 'macho64'
        else
          nasmformat = 'elf64'
        endif
      endif
      if iswindows
        nasmformat = 'win64'
      endif

      add_project_arguments('-f', nasmformat, language: 'nasm')
      projectsources += [path / 'x64-adler32.asm']
      projectsources += [path /   'x64-crc32.asm']

      conf.set('ADLER32_CFG_EXTERNALASM', true)
      conf.set('CRC32_CFG_EXTERNALASM', true)
    else
      warning('NASM not found, using C fallback code')
    endif
  endif
endif


headerfiles = []

base = 'ctoolbox'
r = run_command(python, script, base, '.h', check: true).stdout()
foreach file: r.split(newline)
  if not (file == '')
    headerfiles += [file]
  endif
endforeach

base = 'ctoolbox/private'
r = run_command(python, script, base, '.h', check: true).stdout()
foreach file: r.split(newline)
  if not (file == '')
    headerfiles += [file]
  endif
endforeach

base = 'ctoolbox/crypto'
r = run_command(python, script, base, '.h', check: true).stdout()
foreach file: r.split(newline)
  if not (file == '')
    headerfiles += [file]
  endif
endforeach

configfile = configure_file(input: 'ctoolboxconfig.h.in', output: 'ctoolboxconfig.h', configuration: conf)
headerfiles += [configfile]


largs = []

static_build = static_library('ctoolbox', sources: projectsources, pic: true, install: true)
lib = declare_dependency(
  link_with: static_build, include_directories: include_directories('.'), link_args: largs
)

install_headers(headerfiles, preserve_path: true)
