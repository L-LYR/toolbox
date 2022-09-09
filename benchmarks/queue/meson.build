benches = [
    'spsc_bench',
]

foreach bench_name : benches
    exe = executable(
        bench_name,
        bench_main,
        bench_name + '.cc',
        include_directories: incs,
        dependencies: bench_deps,
    )
    benchmark(bench_name, exe)
endforeach
