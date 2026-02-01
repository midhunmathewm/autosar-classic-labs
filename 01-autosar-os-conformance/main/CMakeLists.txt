#------------------------------------------------------------------------------
# ESP-IDF component build configuration
#
# - Registers application and AUTOSAR OS source files as a single component
# - Exposes only project headers via INCLUDE_DIRS
# - Applies strict GCC warnings to application code
# - Marks ESP-IDF headers as SYSTEM to suppress third-party warnings
#
# Notes:
# - Warning flags apply to this component only
# - ESP-IDF headers are added using -isystem, so diagnostics are not emitted
#   from vendor code while remaining enabled for application sources
#------------------------------------------------------------------------------
idf_component_register(
    SRCS
        "app_entry.c"
        "../src/Os.c"
        "../src/OsTimer_Cfg.c"
        "../src/OsAlarm_Cfg.c"
        "../src/OsCounter_Cfg.c"
        "../src/OsEvent_Cfg.c"
    INCLUDE_DIRS
        "../inc"
)

# ------------------------------------------------------------------------------
# Strict warnings for APPLICATION CODE ONLY
# ------------------------------------------------------------------------------
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall
    -Wextra
    -Wconversion
    -Wsign-conversion
    -Wshadow
    -Wstrict-prototypes
    -fno-common
)

# ------------------------------------------------------------------------------
# Treat ESP-IDF headers as SYSTEM headers (suppress warnings from them)
# ------------------------------------------------------------------------------
target_include_directories(${COMPONENT_LIB}
    SYSTEM PRIVATE
    ${IDF_PATH}/components
)
