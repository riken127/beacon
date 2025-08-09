# src/adapters/meson.build
adapter_sources = [
    'websocket_adapter.cpp',
    'storage_adapter.cpp'
]

adapter_deps = []
# if Boost was available, common_deps already has it; we can reuse that
adapter_deps += common_deps

pg_dep = dependency('libpqxx', required : true)
adapter_deps += [pg_dep]

nlohmann_dep = dependency('nlohmann_json', required : true)
adapter_deps += [nlohmann_dep]

adapters_lib = static_library('beacon_adapters', adapter_sources,
                              include_directories : common_inc,
                              dependencies : adapter_deps,
                              install : false
)

adapters_dep = declare_dependency(link_with : adapters_lib, include_directories : common_inc)
