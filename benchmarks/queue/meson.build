benches = [
    'spsc_bench',
    'mpmc_bench',
]

foreach bench_name : benches
    exe = executable(
        bench_name,
        bench_main,
        bench_name + '.cc',
        include_directories: incs,
        dependencies: bench_deps,
        cpp_args: ['-O2'],
    )
    benchmark(bench_name, exe)
endforeach
