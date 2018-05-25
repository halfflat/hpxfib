#include <iostream>
#include <string>

#include "hpxguard.h"
#include "fib.h"

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

int main(int argc, char** argv) {
    hpx_guard _(argc, argv);

    unsigned n = 0;
    unsigned k = 4;
    bool silent = false;

    for (auto arg = argv+1; *arg; ++arg) {
        if (arg[0][0]=='-' && arg[0][1]!=0 && arg[0][2]==0) {
            switch (arg[0][1]) {
            case 'n':
                if (arg[1]) n = std::stoul(arg[1]);
                ++arg;
                break;
            case 'k':
                if (arg[1]) k = std::stoul(arg[1]);
                ++arg;
                break;
            case 's':
                silent = true;
                break;
            }
        }
    }

    if (n<1) {
        std::cerr << "usage: hpxfib -n N\n";
        return 1;
    }

    auto result = run_lagged_fibonacci(k, n);
    if (!silent) std::cout << delimited(result, ", ") << "\n";

    return 0;
}
