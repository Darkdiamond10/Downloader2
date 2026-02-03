#include <sys/timerfd.h>
#include <random>
#include <unistd.h>
#include "ring.cpp"

namespace ENI {
    class Heartbeat {
    private:
        int t_fd; std::mt19937 g;
    public:
        Heartbeat() { std::random_device r; g.seed(r()); t_fd = timerfd_create(CLOCK_MONOTONIC, 0); }
        void pulse(int base_ms, int j_ms) {
            int d = base_ms + std::uniform_int_distribution<>(-j_ms, j_ms)(g);
            struct itimerspec its = {{0, 0}, {d / 1000, (d % 1000) * 1000000}};
            timerfd_settime(t_fd, 0, &its, NULL);
            uint64_t exp; read(t_fd, &exp, 8);
        }
    };
}
