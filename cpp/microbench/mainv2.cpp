
#include <cstdlib>
#include "fibers.h"
#include "threads.h"
#define MAIN_BENCH

#include <cstddef>
#include <cstdio>

#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <argparse/argparse.hpp>
#include "adapter.h"
#include "run.h"


#include "plaf.h"

#include "globals_extern.h"
#include "globals_t.h"
// #include "globals_t_impl.h"
#include "common/server_clock.h"
#include "workloads/bench_parameters.h"
#include "workloads/thread_loops/thread_loop_impl.h"


template <typename T>
std::unique_ptr<T> ParseJsonFile(const std::string& file_name) {
    std::ifstream fin;
    fin.open(file_name);

    nlohmann::json j = nlohmann::json::parse(fin);
    auto t = std::make_unique<T>(j);

    fin.close();
    return t;
}

int main(int argc, char** argv) {
    printUptimeStampForPERF("MAIN_START");

    std::cout << "binary=" << argv[0] << std::endl;
    argparse::ArgumentParser program("bench");

    // :TODO: extend with all arguments
    program.add_argument("-json-file").help("path to json with workload description");
    program.add_argument("-result-file").help("path to json where results summary will be written");
    program.add_argument("-create-default-prefill")
        .help("create default prefill if not specified in json-file")
        .flag();

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << program;
        return EXIT_FAILURE;
    }
    auto bench_parameters = ParseJsonFile<BenchParameters>(program.get<std::string>("-json-file"));
    auto create_default_prefill = program["-create-default-prefill"];
    if (create_default_prefill == true && bench_parameters->prefill == nullptr) {
        bench_parameters->createDefaultPrefill();
    }

    bench_parameters->init();

    Run<ds_adapter<test_type, VALUE_TYPE>, test_type, VALUE_TYPE, nullptr, fibers> r{
        std::move(bench_parameters),
    };

    r.RunExperiment();
}
