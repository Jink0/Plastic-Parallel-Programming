/* Prototypes for worker functions.  */
int hogcpu (long long local_repeats);
int hogio (long long local_repeats);
int hogvm (long long local_repeats, long long bytes, long long stride, long long hang, int keep);
int hoghdd (long long local_repeats, long long bytes);