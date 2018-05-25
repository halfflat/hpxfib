#include <cassert>
#include <vector>

#include <hpx/async.hpp>
#include <hpx/runtime/launch_policy.hpp>
#include <hpx/runtime/threads/thread.hpp>
#include <hpx/lcos/local/channel.hpp>
#include <hpx/lcos/wait_all.hpp>
#include <hpx/hpx_suspend.hpp>

using hpx::lcos::local::channel;
using hpx::lcos::local::receive_channel;
using hpx::lcos::local::send_channel;
using hpx::launch;

// Computing sequence terms x_n = x_(n-k)+x(n-k+1),
// with x_1 = ... = x_(k-1) = 1.

struct lagged_fibonacci {
    channel<unsigned> x_nmkp1;
    channel<unsigned> x_n;

    const unsigned k;
    unsigned n;
    unsigned x_nmk;

    lagged_fibonacci(unsigned k, unsigned n0):
        k(k), n(n0), x_nmk(1u)
    {
        assert(n0>0 && n0<=k);
        x_n.set(x_nmk);
    }

    void run(unsigned max_n) {
        for (;;) {
            n += k;
            if (n>max_n) break;

            unsigned v = x_nmkp1.get(launch::async).get();
            unsigned r = x_nmk + v;
            x_n.set(r);
            x_nmk = r;
        }
    }
};

struct coordinator {
    const unsigned k;
    std::vector<unsigned> result;

    std::vector<send_channel<unsigned>> x_nmkp1;
    std::vector<receive_channel<unsigned>> x_n;

    template <typename LFContainer>
    explicit coordinator(unsigned k, LFContainer& runners):
        k(k)
    {
        for (unsigned j = 0; j<k; ++j) {
            x_nmkp1.emplace_back(runners[j].x_nmkp1);
            x_n.emplace_back(runners[j].x_n);
        }
    }

    std::vector<unsigned> run(unsigned max_n) {
        result.assign(max_n, 0);

        auto run_one_lane = [this, max_n](unsigned i) {
            unsigned recipient = i? i-1: k-1;

            for (unsigned j = i; j<max_n; j += k) {
                unsigned x = x_n[i].get().get();
                result[j] = x;

                if (j>0 && j+k-1<max_n) {
                    x_nmkp1[recipient].set(x);
                }
            }
        };

        std::vector<hpx::future<void>> wait_list;
        for (unsigned i = 0; i<k; ++i) {
            wait_list.emplace_back(hpx::async(run_one_lane, i));
        }

        hpx::lcos::wait_all(std::move(wait_list));
        return result;
    }
};

std::vector<unsigned> run_lagged_fibonacci(unsigned k, unsigned n) {
    std::vector<unsigned> result;

    hpx::async([&result, k, n]() {
        std::vector<lagged_fibonacci> lf;
        for (unsigned i = 0; i<k; ++i) lf.emplace_back(k, i+1);
        for (unsigned i = 0; i<k; ++i) hpx::async([&lf,i,n]() { lf[i].run(n); });

        coordinator coord(k, lf);
        result = coord.run(n);
    });

    hpx::suspend();
    return result;
}
