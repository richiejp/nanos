#include <kernel.h>

//#define CLOCK_DEBUG
#ifdef CLOCK_DEBUG
#define clock_debug(x, ...) do {tprintf(sym(clock), 0, x, ##__VA_ARGS__);} while(0)
#else
#define clock_debug(x, ...)
#endif

/* These should happen in pairs such that odd indicates update in-progress */
#define vdso_update_gen() fetch_and_add((word *)&__vdso_dat->vdso_gen, 1)

void kernel_delay(timestamp delta)
{
    timestamp end = now(CLOCK_ID_MONOTONIC) + delta;
    while (now(CLOCK_ID_MONOTONIC) < end)
        kern_pause();
}

timestamp kern_now(clock_id id)
{
    return now(id);
}

void clock_update_last_raw(timestamp t)
{
    /* Periodically update last_raw to avoid numerical errors from big intervals */
    if (__vdso_dat->base_freq && (t - __vdso_dat->last_raw > (CLOCK_RAW_UPDATE_SECONDS<<CLOCK_FP_BITS))) {
        vdso_update_gen();
        __vdso_dat->rtc_offset += ((s64)(t - __vdso_dat->last_raw) *
            __vdso_dat->base_freq) >> CLOCK_FP_BITS;
        __vdso_dat->last_raw = t;
        vdso_update_gen();
    }
}

void clock_set_freq(s64 freq)
{
    timestamp here = now(CLOCK_ID_MONOTONIC_RAW);
    vdso_update_gen();
    __vdso_dat->rtc_offset += ((s64)(here - __vdso_dat->last_raw) * __vdso_dat->base_freq) >> CLOCK_FP_BITS;
    __vdso_dat->base_freq = freq;
    __vdso_dat->last_raw = here;
    vdso_update_gen();
    timer_reorder(kernel_timers);
}

void clock_set_slew(s64 slewfreq, timestamp start, u64 duration)
{
    vdso_update_gen();
    __vdso_dat->slew_freq = slewfreq;
    __vdso_dat->slew_start = start;
    __vdso_dat->slew_end = start + duration;
    vdso_update_gen();
    timer_reorder(kernel_timers);
}

void clock_rtc_step(s64 step)
{
    vdso_update_gen();
    __vdso_dat->rtc_offset += step;
    vdso_update_gen();
    timer_reorder(kernel_timers);
    rtc_settimeofday(sec_from_timestamp(now(CLOCK_ID_REALTIME)));
}

closure_function(1, 1, boolean, timer_adjust_handler,
                s64, amt,
                void *, v)
{
    timer t = v;
    switch (t->id) {
    case CLOCK_ID_REALTIME:
    case CLOCK_ID_REALTIME_COARSE:
    case CLOCK_ID_REALTIME_ALARM:
        t->expiry += bound(amt);
        break;
    default:
        break;
    }
    return true;
}

void clock_reset_rtc(timestamp wallclock_now)
{
    clock_debug("%s: now %T, wallclock_now %T\n",
                __func__, now(CLOCK_ID_REALTIME), wallclock_now);
    timestamp n = now(CLOCK_ID_REALTIME);
    rtc_settimeofday(sec_from_timestamp(wallclock_now));
    pqueue_walk(kernel_timers->pq, stack_closure(timer_adjust_handler, wallclock_now - n));
    timer_reorder(kernel_timers);
    reset_clock_vdso_dat();
}
