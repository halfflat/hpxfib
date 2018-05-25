#include <iostream>
#include <string>

#include "hpxguard.h"
#include "fib.h"

using namespace std::string_literals;

template <typename T>
struct delimited_wrapper {
    const T& items;
    const std::string& delimiter;

    delimited_wrapper(const T& items, const std::string& delimiter):
        items(items), delimiter(delimiter) {}

    friend std::ostream& operator<<(std::ostream& out, const delimited_wrapper& w) {
        bool tail = false;
        for (auto& item: w.items) {
            if (tail) out << w.delimiter;
            out << item;
            tail = true;
        }
        return out;
    }
};

template <typename T>
delimited_wrapper<T> delimited(const T& item, const std::string& delimiter) {
    return delimited_wrapper<T>(item, delimiter);
}

const char* usage_str =
    "Usage: hpxfib [HPXOPTS] [OPTIONS] -n N\n"
    "Compute first N numbers in sequence x(n) = x(n-k) + x(n-k+1),\n"
    "with x(1) = ... = x(k) = 1.\n"
    "\n"
    "Options:\n"
    "  -k K     Set k=K in recurrent sequence (default 2).\n"
    "  -s       Silent mode: do not print result.\n"
    "\n"
    "N must be at least K.\n"
    "HPX options must be in long form and precede any program options.\n";

int usage() {
    std::cerr << usage_str;
    return 1;
}

int usage(const std::string& error) {
    std::cerr << "hpxfib: " << error << "\n\n";
    return usage();
}

int main(int argc, char** argv) {
    hpx_guard _(argc, argv);

    unsigned n = 0;
    unsigned k = 2;
    bool silent = false;

    for (auto arg = argv+1; *arg; ++arg) {
        if (arg[0][0]=='-' && arg[0][1]!=0 && arg[0][2]==0) {
            switch (arg[0][1]) {
            case 'n':
                if (arg[1]) n = std::stoul(arg[1]);
                ++arg;
                continue;
            case 'k':
                if (arg[1]) k = std::stoul(arg[1]);
                ++arg;
                continue;
            case 's':
                silent = true;
                continue;
            default:
                break;
            }
        }
        return usage("unrecognized argument: "s+*arg+".");
    }

    if (n<1) return usage("missing or zero N.");
    if (n<k) return usage("N must be at least K.");

    auto result = run_lagged_fibonacci(k, n);
    if (!silent) std::cout << delimited(result, ", ") << "\n";

    return 0;
}
