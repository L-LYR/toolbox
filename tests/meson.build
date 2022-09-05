test_deps = deps

test_deps += gtest.get_variable('gtest_dep')


spsc_test = executable(
    'spsc_test',
    'main.cc',
    'spsc_test.cc',
    include_directories: incs,
    dependencies: test_deps,
)

test('spsc test', spsc_test)
