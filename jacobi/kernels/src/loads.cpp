#include <ctype.h>
#include <errno.h>
#include <libgen.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

/* By default, print all messages of severity info and above.  */
static int global_debug = 2;

/* Name of this program */
static char const *global_progname = "load gen";

/* Implemention of runtime-selectable severity message printing.  */
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



int hogcpu(long long local_repeats) {
  for (long long i = 0; i < local_repeats; i++) {
    sin(rand());
    sqrt(rand());

  }

  return 0;
}

int hogio (long long local_repeats) {
  for (long long i = 0; i < local_repeats; i++) {
    sync();
  }

  return 0;
}

int hogvm (long long local_repeats, long long bytes, long long stride, long long hang, int keep) {
  long long i;
  char *ptr = 0;
  char c;
  int do_malloc = 1;

  for (i = 0; i < local_repeats; i++) {
      if (do_malloc) {
          dbg (stdout, "allocating %lli bytes ...\n", bytes);

          if (!(ptr = (char *) malloc (bytes * sizeof (char)))) {
              err (stderr, "hogvm malloc failed: %s\n", strerror (errno));

              return 1;
            }

          if (keep)
            do_malloc = 0;
        }

      dbg (stdout, "touching bytes in strides of %lli bytes ...\n", stride);
      for (i = 0; i < bytes; i += stride)
        ptr[i] = 'Z';           /* Ensure that COW happens.  */

      if (hang == 0) {
          dbg (stdout, "sleeping forever with allocated memory\n");
          while (1)
            sleep (1024);
        
        } else if (hang > 0) {
          dbg (stdout, "sleeping for %llis with allocated memory\n", hang);
          sleep (hang);
        }

      for (i = 0; i < bytes; i += stride) {
          c = ptr[i];

          if (c != 'Z') {
              err (stderr, "memory corruption at: %p\n", ptr + i);
              return 1;
            }
        }

      if (do_malloc) {
          free (ptr);
          dbg (stdout, "freed %lli bytes\n", bytes);
        }
    }

  return 0;
}

int
hoghdd (long long local_repeats, long long bytes) {
  long long i, j;
  int fd;
  int chunk = (1024 * 1024) - 1;        /* Minimize slow writing.  */
  char buff[chunk];

  /* Initialize buffer with some random ASCII data.  */
  dbg (stdout, "seeding %d byte buffer with random data\n", chunk);
  for (i = 0; i < chunk - 1; i++) {
      j = rand ();
      j = (j < 0) ? -j : j;
      j %= 95;
      j += 32;
      buff[i] = j;
    }

  buff[i] = '\n';

  for (long long i = 0; i < local_repeats; i++) {
      char name[] = "./load.XXXXXX";

      if ((fd = mkstemp (name)) == -1) {
          err (stderr, "mkstemp failed: %s\n", strerror (errno));
          return 1;
        }

      dbg (stdout, "opened %s for writing %lli bytes\n", name, bytes);

      dbg (stdout, "unlinking %s\n", name);
      if (unlink (name) == -1) {
          err (stderr, "unlink of %s failed: %s\n", name, strerror (errno));
          return 1;
        }

      dbg (stdout, "fast writing to %s\n", name);
      for (j = 0; bytes == 0 || j + chunk < bytes; j += chunk) {
          if (write (fd, buff, chunk) == -1) {
              err (stderr, "write failed: %s\n", strerror (errno));
              return 1;
            }
        }

      dbg (stdout, "slow writing to %s\n", name);
      for (; bytes == 0 || j < bytes - 1; j++) {
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

      dbg (stdout, "closing %s after %lli bytes\n", name, j);
      close (fd);
    }

  return 0;
}