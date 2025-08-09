# meson/meson.build (reusable helper)
# This file is intentionally small & dependency-light so it can be included in other projects.

# Project-wide public include dir (use include/<namespace>/...)
common_inc = include_directories('..' / 'include')

# Convenience array for dependencies (empty unless we find libs)
common_deps = []

# If Boost requested
if get_option('with_boost')
    boost_sys = dependency('boost', required : true)
    if boost_sys.found()
        common_deps += [boost_sys]
    else
        message('WARNING: Boost not found. Either install Boost or set -Dwith_boost=false')
    endif
endif

# Exportable names: common_inc, common_deps
