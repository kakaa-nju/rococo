#include "all.h"
#include <unistd.h>
#include <signal.h>

namespace deptran {

bool TimerLoop::run_ = false;
pthread_mutex_t TimerLoop::run_mutex_ = PTHREAD_MUTEX_INITIALIZER;

// The wall-clock alarm is host time, not simulated time: under DetSim the
// signal never fires in the modeled timeline. Keep the same interface but
// compare the emulated gettimeofday clock instead, so a run of "N seconds" is
// a deterministic amount of simulated time.
static uint64_t timer_start_us_ = 0;
static uint64_t timer_duration_us_ = 0;

void TimerLoop::set_alarm(unsigned int time) {
    timer_start_us_ = Time::now();
    timer_duration_us_ = uint64_t(time) * Time::RRR_USEC_PER_SEC;
}

void TimerLoop::init(uint32_t time) {
    pthread_mutex_lock(&run_mutex_);
    if (!run_) {
        run_ = true;
        set_alarm((unsigned int)time);
    }
    pthread_mutex_unlock(&run_mutex_);
}

void TimerLoop::stop() {
    run_ = false;
}

void TimerLoop::alarm_handler(int sig) {
    run_ = false;
}

bool TimerLoop::is_run() {
    if (!run_) {
        return false;
    }
    if (Time::now() - timer_start_us_ >= timer_duration_us_) {
        run_ = false;
        return false;
    }
    return true;
}

}
