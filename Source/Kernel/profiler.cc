// profiler.cc

#include "profiler.h"

#include <time.h>

#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/thread_act.h>
#else
#include <sys/resource.h>
#include <sys/time.h>
#endif

double
Profiler::thread_cpu_seconds()
{
#if defined(CLOCK_THREAD_CPUTIME_ID)
    timespec ts{};
    if(clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) == 0)
        return double(ts.tv_sec) + 1e-9 * double(ts.tv_nsec);
#endif

#if defined(__APPLE__)
    mach_port_t thread_port = mach_thread_self();
    thread_basic_info_data_t info{};
    mach_msg_type_number_t count = THREAD_BASIC_INFO_COUNT;
    const kern_return_t kr = thread_info(thread_port,
                                         THREAD_BASIC_INFO,
                                         reinterpret_cast<thread_info_t>(&info),
                                         &count);
    mach_port_deallocate(mach_task_self(), thread_port);
    if(kr != KERN_SUCCESS)
        return 0.0;

    double user = double(info.user_time.seconds) + 1e-6 * double(info.user_time.microseconds);
    double sys  = double(info.system_time.seconds) + 1e-6 * double(info.system_time.microseconds);
    return user + sys;
#else
    rusage usage{};
    if(getrusage(RUSAGE_THREAD, &usage) != 0)
        return 0.0;

    double user = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec * 1e-6;
    double sys  = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec * 1e-6;
    return user + sys;
#endif
}
