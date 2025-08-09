# src/meson.build
# This file relies on variables from meson/meson.build (common_inc, common_deps)

subdir('domain')
subdir('adapters')

# main application (links domain + adapters libraries)
executable('beacon_broker',
           'main.cpp',
           include_directories : common_inc,
           link_with : [domain_lib, adapters_lib],
           dependencies : common_deps,
           install : true
)
