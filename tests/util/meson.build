tests = [
    'statistics_test',
]

foreach test_name : tests
    exe = executable(
        test_name,
        test_main,
        test_name + '.cc',
        include_directories: incs,
        dependencies: test_deps,
    )
    test(test_name, exe, is_parallel: false)
endforeach
