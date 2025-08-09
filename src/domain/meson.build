# src/domain/meson.build
domain_sources = [
    'recommendation_engine.cpp',
    'schema_manager.cpp'
]

domain_lib = static_library('beacon_domain', domain_sources,
                            include_directories : common_inc,
                            install : false
)

# Optionally export a dependency object so other subprojects can depend on domain
domain_dep = declare_dependency(link_with : domain_lib, include_directories : common_inc)
