project(
    'toolbox',
    'cpp',
    version: '0.0.1',
    default_options: [
        'cpp_std=c++17',
        'debug=true',
        'warning_level=3',
        'werror=true',
        'buildtype=debug',
    ],
)

cpp = meson.get_compiler('cpp')

# compile arguments
if cpp.get_id() == 'gcc'
    add_global_arguments(
        '--param',
        'destructive-interference-size=64',
        language: 'cpp',
    )
endif

if get_option('b_sanitize') == 'address'
    add_global_arguments(
        '-fsanitize=address',
        '-fno-omit-frame-pointer',
        language: 'cpp',
    )
endif

# include directories
incs = []
incs += include_directories('./include')

# dependencies
deps = []

# subprojects
gtest = subproject('gtest')
fmt = subproject('fmt')
spdlog = subproject('spdlog')

deps += fmt.get_variable('fmt_dep')
deps += spdlog.get_variable('spdlog_dep')

# program
find_program('valgrind')

# sub-directories
subdir('./tests')
