#pragma once

struct hpx_guard {
    hpx_guard(int& argc, char** argv);
    ~hpx_guard();
};
