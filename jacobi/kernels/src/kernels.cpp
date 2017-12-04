#include "kernels.hpp"

#include <math.h>
#include <string.h>
#include <unistd.h>

#include "general_utils.hpp"



// By default, print all messages of severity info and above
static int global_debug = 2;

// Name of this program
static char const *global_progname = "kernels ";

// Implementation of runtime-selectable severity message printing
#define dbg(OUT, STR, ARGS...) if (global_debug >= 3) \
	fprintf (stdout, "%s: dbug: [%lli] ", \
		global_progname, (long long)getpid()), \
		fprintf(OUT, STR, ##ARGS), fflush(OUT)

#define out(OUT, STR, ARGS...) if (global_debug >= 2) \
	fprintf (stdout, "%s: info: [%lli] ", \
		global_progname, (long long)getpid()), \
		fprintf(OUT, STR, ##ARGS), fflush(OUT)

#define wrn(OUT, STR, ARGS...) if (global_debug >= 1) \
	fprintf (stderr, "%s: WARN: [%lli] (%d) ", \
		global_progname, (long long)getpid(), __LINE__), \
		fprintf(OUT, STR, ##ARGS), fflush(OUT)

#define err(OUT, STR, ARGS...) if (global_debug >= 0) \
	fprintf (stderr, "%s: FAIL: [%lli] (%d) ", \
		global_progname, (long long)getpid(), __LINE__), \
		fprintf(OUT, STR, ##ARGS), fflush(OUT)



// Generates cpu load, repeats for given amount
volatile int hogcpu(long long repeats) {

	for (long long i = 0; i < repeats; i++) {
		sqrt(i);
	}

	return 0;
}



// Generates io load, repeats for given amount
int hogio(long long repeats) {

	for (long long i = 0; i < repeats; i++) {
		sync();
	}

	return 0;
}



// Generates vm load by allocating memory, touching certain bytes, and optionally hanging. Repeats for given amount
int hogvm(long long repeats) {

  char *ptr = 0;
  int do_malloc = 1;

	for (long long i = 0; i < repeats; i++) {
		if (do_malloc) {
			dbg(stdout, "allocating %lli bytes ...\n", vm_bytes);

			if (!(ptr = (char *) malloc (vm_bytes * sizeof (char)))) {
				err (stderr, "hogvm malloc failed: %s\n", strerror (errno));

				return 1;
			}

			if (vm_keep) {
				do_malloc = 0;
			}
		}

		dbg(stdout, "touching bytes in strides of %lli bytes ...\n", vm_stride);

		for (long long i = 0; i < vm_bytes; i += vm_stride) {
			ptr[i] = 'Z'; // Ensure that COW happens
		}

		if (vm_hang == 0) {
			dbg(stdout, "sleeping forever with allocated memory\n");

			while (1) {
				sleep (1024);
			}
		
		} else if (vm_hang > 0) {
			dbg(stdout, "sleeping for %llis with allocated memory\n", vm_hang);

			sleep (vm_hang);
		}

		for (long long i = 0; i < vm_bytes; i += vm_stride) {
			char c = ptr[i];

			if (c != 'Z') {
				err (stderr, "memory corruption at: %p\n", ptr + i);

				return 1;
			}
		}

		if (do_malloc) {
			free (ptr);

			dbg(stdout, "freed %lli bytes\n", vm_bytes);
		}
	}

	return 0;
}



// Generates hdd load by writing random data to the hdd, repeats for given amount
int hoghdd(long long repeats) {

	long long j;
	int fd;
	int chunk = (1024 * 1024) - 1; // Minimize slow writing
	char buff[chunk];

	dbg(stdout, "seeding %d byte buffer with random data\n", chunk);

	// Initialize buffer with some random ASCII data
	for (long long i = 0; i < chunk - 1; i++) {
		j = rand_long_long(0, RAND_MAX);
		j = (j < 0) ? -j : j;
		j %= 95;
		j += 32;
		buff[i] = j;
	}

	buff[chunk - 1] = '\n';

	for (long long i = 0; i < repeats; i++) {
		char name[] = "./load.XXXXXX";

		if ((fd = mkstemp (name)) == -1) {
			err (stderr, "mkstemp failed: %s\n", strerror (errno));

			return 1;
		}

		dbg(stdout, "opened %s for writing %lli bytes\n", name, hdd_bytes);

		dbg(stdout, "unlinking %s\n", name);

		if (unlink (name) == -1) {
			err (stderr, "unlink of %s failed: %s\n", name, strerror (errno));

			return 1;
		}

		dbg(stdout, "fast writing to %s\n", name);

		for (long long j = 0; hdd_bytes == 0 || j + chunk < hdd_bytes; j += chunk) {
			if (write (fd, buff, chunk) == -1) {
				err (stderr, "write failed: %s\n", strerror (errno));

				return 1;
			}
		}

		dbg(stdout, "slow writing to %s\n", name);

		for (; hdd_bytes == 0 || j < hdd_bytes - 1; j++) {
			if (write (fd, &buff[j % chunk], 1) == -1) {
				err (stderr, "write failed: %s\n", strerror (errno));

				return 1;
			}
		}

		if (write (fd, "\n", 1) == -1) {
			err (stderr, "write failed: %s\n", strerror (errno));

			return 1;
		}

		++j;

		dbg(stdout, "closing %s after %lli bytes\n", name, j);

		close (fd);
	}

	return 0;
}