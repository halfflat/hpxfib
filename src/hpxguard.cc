#include <cstring>
#include <stdexcept>

#include <hpx/apply.hpp>
#include <hpx/hpx_start.hpp>
#include <hpx/hpx_suspend.hpp>
#include <hpx/runtime.hpp>
#include <hpx/util/yield_while.hpp>

#include "hpxguard.h"

static int hpx_nonoption_index(char **argv) {
    static std::pair<const char*, int> hpx_opts_with_args[] = {
      {"options-file", 1},
      {"hpx", 1},
      {"agas", 1},
      {"nodefile", 1},
      {"nodes", 1},
      {"ifsuffix", 1},
      {"ifprefix", 1},
      {"iftransform", 1},
      {"localities", 1},
      {"node", 1},
      {"threads", 1},
      {"cores", 1},
      {"affinity", 1},
      {"bind", 1},
      {"queuing", 1},
      {"high-priority-threads", 1},
      {"app-config", 1},
      {"config", 1},
      {"ini", 1},
      {"debug-hpx-log", '?'},
      {"debug-agas-log", '?'},
      {"debug-parcel-log", '?'},
      {"debug-timing-log", '?'},
      {"debug-app-log", '?'},
      {"attach-debugger", 1},
      {"printer-counter-at", 1}
    };

    int i = 1;
    while (argv[i]) {
        if (std::strncmp(argv[i], "--hpx:", 6)) break;

        char* hpxopt = argv[i]+6;
        char* eq = strchr(hpxopt, '=');
        std::size_t hpxopt_len = eq? eq-hpxopt: strlen(hpxopt);

        for (auto kn: hpx_opts_with_args) {
            if (std::strncmp(kn.first, hpxopt, hpxopt_len)) continue;
            if (eq) continue;
            else if (kn.second!='?') i += kn.second;
            else if (argv[i+1] && *argv[i+1]!='-') ++i;
        }
        ++i;
    }
    return i;
}

hpx_guard::hpx_guard(int& argc, char** argv) {
    int hpx_argc = hpx_nonoption_index(argv);
    char* first_nonhpx_arg = argv[hpx_argc];

    try {
        argv[hpx_argc] = 0;
        if (!hpx::start(nullptr, hpx_argc, argv)) throw std::runtime_error("hpx::start failed");

        // Wait for HPX to be ready, or stop if we're stuck at 'state_initialized', wtf.
        auto rt = hpx::get_runtime_ptr();
        hpx::util::yield_while([rt]() { auto state = rt->get_state(); return state!=hpx::state_initialized && state<hpx::state_running; }); 
    }
    catch (...) {
        argv[hpx_argc] = first_nonhpx_arg;
        throw;
    }

    // Move all the non-HPX arguments down.
    if (hpx_argc!=argc) {
        argv[1] = first_nonhpx_arg;
        for (int i = 1; hpx_argc+i<=argc; ++i) {
            argv[i+1] = argv[hpx_argc+i];
        }
        argc = argc-hpx_argc+1;
    }
}

hpx_guard::~hpx_guard() {
    auto state = hpx::get_runtime_ptr()->get_state();
    switch (state) {
    case hpx::state_invalid:
    case hpx::state_initialized:
    case hpx::state_stopped:
        break;
    case hpx::state_suspended:
    case hpx::state_sleeping:
        hpx::resume(); // fall through
    default:
        hpx::apply([](){hpx::finalize();});
        hpx::stop();
    }
}
