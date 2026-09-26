MRuby::CrossBuild.new('pin-esp32') do |conf|
  conf.toolchain('gcc')

  conf.cc.command = "xtensa-#{ENV['CONFIG_IDF_TARGET']}-elf-gcc"
  conf.linker.command = "xtensa-#{ENV['CONFIG_IDF_TARGET']}-elf-ld"
  conf.archiver.command = "xtensa-#{ENV['CONFIG_IDF_TARGET']}-elf-ar"

  conf.cc.host_command = 'gcc'
  conf.cc.flags << '-Wall'
  conf.cc.flags << '-Wno-format'
  conf.cc.flags << '-Wno-unused-function'
  conf.cc.flags << '-Wno-maybe-uninitialized'
  conf.cc.flags << '-mlongcalls'

  conf.cc.defines << 'MRB_TICK_UNIT=10'
  conf.cc.defines << 'MRB_TIMESLICE_TICK_COUNT=1'
  conf.cc.defines << 'MRB_UTF8_STRING'
  conf.cc.defines << 'MRB_INT64'
  conf.cc.defines << 'MRB_NO_BOXING'
  conf.cc.defines << 'MRB_32BIT'
  conf.cc.defines << 'PICORB_ALLOC_ESTALLOC'
  conf.cc.defines << 'PICORB_ALLOC_ALIGN=8'
  conf.cc.defines << 'NDEBUG'
  conf.cc.defines << 'MRC_PRISM_ARENA_BLOCK=2048'
  conf.cc.defines << 'MRC_PRISM_ARENA_LIBC'

  mruby_gems = "#{MRUBY_ROOT}/mrbgems/picoruby-mruby/lib/mruby/mrbgems"

  conf.picoruby
  conf.gembox 'minimum'
  conf.gem gemdir: "#{mruby_gems}/mruby-kernel-ext"
  conf.gem gemdir: "#{mruby_gems}/mruby-string-ext"
  conf.gem gemdir: "#{mruby_gems}/mruby-array-ext"
  conf.gem gemdir: "#{mruby_gems}/mruby-error"
  conf.gem gemdir: "#{mruby_gems}/mruby-sprintf"
  conf.gem gemdir: "#{mruby_gems}/mruby-math"
  conf.gem core: 'picoruby-require'
  conf.gem core: 'picoruby-machine'
  conf.gem core: 'picoruby-gpio'
end
