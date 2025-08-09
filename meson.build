# meson.build (root)
project('beacon-broker', 'cpp',
        version : '1.0.0',
        default_options : ['warning_level=3', 'cpp_std=c++20']
)

# Read options
with_boost = get_option('with_boost')
use_rsocket = get_option('use_rsocket')
use_websocketpp = get_option('use_websocketpp')
enable_tests = get_option('enable_tests')

# include reusable helpers (keeps root file small)
subdir('meson')

# Now dispatch into source and tests
subdir('src')

if enable_tests
    subdir('tests')
endif
