#define VVAR_DEF(type, name) type name
#define VVAR_REF(name) __vdso_ ## name
#define VVAR_DECL(type, name) extern VVAR type __vdso_ ## name

#define VDSO_NO_NOW (timestamp)-1

/* An instance of this struct is shared between kernel and userspace
 * Make sure there are no pointers embedded in it
 */
struct vdso_dat_struct {
    vdso_clock_id clock_src;
    timestamp rtc_offset;
    u64 pvclock_offset;
    s64 temp_cal;   /* temporary calibration value (valid until sync_complete) */
    timestamp sync_complete;    /* time at which temporary calibration ceases to take effect */
    s64 cal;    /* calibration value (from monotonic raw to monotonic); 0 means no drift */
    s64 last_drift; /* last calculated drift from monotonic raw to monotonic */
    timestamp last_raw; /* time at which last_drift has been calculated */
    struct arch_vdso_dat machine;
};

/* VDSO accessible variables */
VVAR_DECL(struct vdso_dat_struct, vdso_dat);

/* now() routines that are accessible from both the VDSO and the core kernel */
struct pvclock_vcpu_time_info;
VDSO u64 vdso_pvclock_now_ns(volatile struct pvclock_vcpu_time_info *);
VDSO timestamp vdso_now(clock_id id);
VDSO int vdso_getcpu(unsigned *cpu, unsigned *node);
