project('ctoolbox', 'c', default_options: ['c_std=c99', 'buildtype=release', 'warning_level=3'], version: run_command('python', 'tools/version.py', check: true).stdout().strip())

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
  if target_machine.endianess() == 'big'
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

configure_file(input: 'ctbconfig.template', output: 'ctbconfig.h', configuration: conf)


projectsources = []
externalobjs   = []


python = find_program('python3')
script = join_paths(meson.source_root(), 'tools/listfiles.py')
newline = '''
'''


base = 'src'
r = run_command(python, script, base, '.c', check: true).stdout()
foreach file: r.split(newline)
  if not (file == '')
    projectsources += [file]
  endif
endforeach

base = 'crypto/src'
r = run_command(python, script, base, '.c', check: true).stdout()
foreach file: r.split(newline)
  if not (file == '')
    projectsources += [file]
  endif
endforeach


if target == 'x86_64'
  supported = false

  path = 'crypto/src/asm/'
  if isunix
    path = path / 'unix'
    supported = true
  endif
  if iswindows
    path = path / 'windows'
    supported = true
  endif

  if supported
    externalobjs += [path / 'adler32-' + target + '.o']
    externalobjs += [path /   'crc32-' + target + '.o']
    add_project_arguments('-DADLER32_CFG_EXTERNALASM', language: 'c')
    add_project_arguments(  '-DCRC32_CFG_EXTERNALASM', language: 'c')
  endif
endif


largs = []
if cc.get_linker_id() == 'ld.bfd'
  largs += ['-z', 'noexecstack']
endif


static_build = static_library('ctoolbox', sources: projectsources, objects: externalobjs, pic: true, install: true, include_directories: include_directories('.'))

lib = declare_dependency(
  link_with: static_build, include_directories: include_directories('.'), link_args: largs
)
