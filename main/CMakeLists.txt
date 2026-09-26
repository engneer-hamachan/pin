set(PICORUBY_DIR ${COMPONENT_DIR}/../R2P2-ESP32/components/picoruby-esp32/picoruby)
set(PICORUBY_BUILD_NAME pin-esp32)
set(PICORUBY_CONFIG ${COMPONENT_DIR}/../build_config/${PICORUBY_BUILD_NAME}.rb)
set(LIBMRUBY_FILE ${PICORUBY_DIR}/build/${PICORUBY_BUILD_NAME}/lib/libmruby.a)
set(PICORUBY_TI_DIR ${COMPONENT_DIR}/../picoruby-ti)

file(MAKE_DIRECTORY ${PICORUBY_DIR}/build/${PICORUBY_BUILD_NAME}/include)
file(MAKE_DIRECTORY ${PICORUBY_DIR}/build/prism/include)

file(GLOB_RECURSE EDITOR_SOURCES ${COMPONENT_DIR}/../editor/*.c)
file(GLOB_RECURSE PICORUBY_TI_SOURCES ${PICORUBY_TI_DIR}/src/*.c)
list(REMOVE_ITEM PICORUBY_TI_SOURCES ${PICORUBY_TI_DIR}/src/picoruby_ti.c)

idf_component_register(
  SRCS
    main.c
    canvas.cpp
    keyboard.cpp
    widget.c
    matrix.c
    circuit.c
    board.c
    catalog.c
    footprint.c
    values.c
    parts.c
    elements.c
    simulate.c
    placement.c
    input.c
    info.c
    page.c
    part_info.c
    storage.c
    render_board.c
    render_parts.c
    render_modules.c
    pino.c
    pino_program.c
    pino_machine.c
    pino_gpio.c
    ../game/title.c
    ../game/game.c
    ../game/rules.c
    ../game/hiscore.c
    ${EDITOR_SOURCES}
    ${PICORUBY_TI_SOURCES}
    ${PICORUBY_DIR}/mrbgems/picoruby-machine/ports/common/machine.c
    ${PICORUBY_DIR}/mrbgems/picoruby-io-console/ports/esp32/io-console.c
    ${PICORUBY_DIR}/mrbgems/picoruby-env/ports/esp32/env.c
    ${PICORUBY_DIR}/mrbgems/picoruby-require/ports/esp32/platform.c
  INCLUDE_DIRS
    "."
    "../game"
    "../editor"
  PRIV_INCLUDE_DIRS
    ${PICORUBY_DIR}/include
    ${PICORUBY_DIR}/build/${PICORUBY_BUILD_NAME}/include
    ${PICORUBY_DIR}/build/prism/include
    ${PICORUBY_DIR}/mrbgems/picoruby-mruby/lib/mruby/include
    ${PICORUBY_DIR}/mrbgems/picoruby-mruby/lib/mruby/mrbgems/mruby-task/include
    ${PICORUBY_DIR}/mrbgems/mruby-compiler/include
    ${PICORUBY_DIR}/mrbgems/mruby-compiler/lib/prism/include
    ${PICORUBY_DIR}/mrbgems/picoruby-machine/include
    ${PICORUBY_DIR}/mrbgems/picoruby-gpio/include
    ${PICORUBY_TI_DIR}/include
    ${PICORUBY_TI_DIR}/src
    ${PICORUBY_TI_DIR}/src/base
    ${PICORUBY_TI_DIR}/src/builtin
    ${PICORUBY_TI_DIR}/src/context
    ${PICORUBY_TI_DIR}/src/diagnostic
    ${PICORUBY_TI_DIR}/src/eval
    ${PICORUBY_TI_DIR}/src/eval/method_evaluator
    ${PICORUBY_TI_DIR}/src/generated
    ${PICORUBY_TI_DIR}/src/hover
    ${PICORUBY_TI_DIR}/src/suggest
  PRIV_REQUIRES
    app_update
    esp_driver_gpio
    esp_driver_sdspi
    esp_driver_spi
    esp_partition
    esp_timer
    fatfs
    sdmmc
)

target_compile_definitions(
  ${COMPONENT_LIB}
  PRIVATE
    PIN_BOARD_${PIN_BOARD}
    MRB_TICK_UNIT=10
    MRB_TIMESLICE_TICK_COUNT=1
    MRB_UTF8_STRING
    MRB_INT64
    MRB_NO_BOXING
    MRB_32BIT
    MRB_CONSTRAINED_BASELINE_PROFILE=1
    MRB_USE_TASK_SCHEDULER
    PICORB_VM_MRUBY
    PICORB_ALLOC_ESTALLOC
    PICORB_ALLOC_ALIGN=8
    PRISM_XALLOCATOR
    PRISM_BUILD_MINIMAL
    MRC_TARGET_MRUBY
    MRC_PRISM_ARENA
)

add_custom_command(
  OUTPUT ${LIBMRUBY_FILE}
  COMMAND ${CMAKE_COMMAND} -E env
    MRUBY_CONFIG=${PICORUBY_CONFIG}
    CONFIG_IDF_TARGET=${CONFIG_IDF_TARGET}
    rake
  WORKING_DIRECTORY ${PICORUBY_DIR}
  DEPENDS ${PICORUBY_CONFIG}
  COMMENT "PicoRuby build"
  VERBATIM
)

add_custom_target(picoruby DEPENDS ${LIBMRUBY_FILE})
add_dependencies(${COMPONENT_LIB} picoruby)

add_prebuilt_library(libmruby ${LIBMRUBY_FILE} REQUIRES ${COMPONENT_NAME})
target_link_libraries(${COMPONENT_LIB} PRIVATE libmruby)

set_source_files_properties(
  ${EDITOR_SOURCES}
  ${PICORUBY_TI_SOURCES}
  ${PICORUBY_DIR}/mrbgems/picoruby-machine/ports/common/machine.c
  ${PICORUBY_DIR}/mrbgems/picoruby-io-console/ports/esp32/io-console.c
  ${PICORUBY_DIR}/mrbgems/picoruby-env/ports/esp32/env.c
  ${PICORUBY_DIR}/mrbgems/picoruby-require/ports/esp32/platform.c
  PROPERTIES COMPILE_OPTIONS -Wno-error
)
