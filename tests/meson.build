# tests/meson.build
test_sources = ['test_schema.cpp']

test_exe = executable('test_schema', test_sources,
                      include_directories : common_inc,
                      link_with : [domain_lib],
                      install : false
)

test('schema', test_exe)
