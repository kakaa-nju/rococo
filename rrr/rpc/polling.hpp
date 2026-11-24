#pragma once

#include <map>
#include <set>

//#include "rrr.hpp"
#include "base/misc.hpp"
#include "utils.hpp"

using rrr::FrequentJob;

namespace rrr {

class Pollable: public rrr::RefCounted {
protected:

    // RefCounted class requires protected destructor
    virtual ~Pollable() {}

public:

    enum {
        READ = 0x1, WRITE = 0x2
    };

    virtual int fd() = 0;
    virtual int poll_mode() = 0;
    virtual void handle_read() = 0;
    virtual void handle_write() = 0;
    virtual void handle_error() = 0;
};

class PollMgr: public rrr::RefCounted {
 public:
    class PollThread;

    PollThread* poll_threads_;
    const int n_threads_;

protected:

    // RefCounted object uses protected dtor to prevent accidental deletion
    ~PollMgr();

public:

    PollMgr(int n_threads = 1);

    void add(Pollable*);
    void remove(Pollable*);
    void update_mode(Pollable*, int new_mode);
    
    // Frequent Job
    void add(FrequentJob*);
    void remove(FrequentJob*);

    // DetSim runs every tracee with a single thread, so an event loop cannot
    // live in a dedicated poll thread: the program's main loop drives all
    // pollers. RRR_MULTI_THREAD=1 restores the original threading model.
    static bool single_threaded();
    static void poll_all(int timeout_ms = 1);
};

} // namespace rrr
