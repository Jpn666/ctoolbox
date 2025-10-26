# major, minor and patch
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
conf.set('version_patch', v2)


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

if get_option('no_stdlib')
  conf.set('CTB_CFG_NOSTDLIB', true)
endif


isunix = false
iswindows = false


# System platform
if target_machine.system() in ['windows', 'cygwin']
  conf.set('CTB_CFG_PLATFORM_WINDOWS', true)
else
  if target_machine.system() in ['android', 'darwin', 'dragonfly', 'freebsd', 'gnu', 'linux', 'netbsd', 'openbsd', 'sunos']
    conf.set('CTB_CFG_PLATFORM_UNIX', true)
  endif
endif


# Checked int
ckdintfailback = false
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


configure_file(input: 'config.h.in', output: 'config.h', configuration: conf, install: true, install_dir: 'include/ctoolbox/config')
