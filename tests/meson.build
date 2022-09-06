test_deps = deps

test_deps += gtest.get_variable('gtest_dep')


bounded_spsc_test = executable(
    'bounded_spsc_test',
    'main.cc',
    'queue/bounded_spsc_test.cc',
    include_directories: incs,
    dependencies: test_deps,
)

unbounded_spsc_test = executable(
    'unbounded_spsc_test',
    'main.cc',
    'queue/unbounded_spsc_test.cc',
    include_directories: incs,
    dependencies: test_deps,
)

test('bounded spsc queue test', bounded_spsc_test, is_parallel : false)
test('unbounded spsc queue test', unbounded_spsc_test, is_parallel : false)
