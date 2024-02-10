#ifndef ZDX_FILE_H_
#define ZDX_FILE_H_

#pragma GCC diagnostic error "-Wnonnull"
#pragma GCC diagnostic error "-Wnull-dereference"

#include <stdlib.h>
#include <stdbool.h>

#ifndef FL_MALLOC
#define FL_MALLOC malloc
#endif // FL_MALLOC

#ifndef FL_FREE
#define FL_FREE free
#endif // FL_FREE

typedef struct file_content {
  bool is_valid;
  char *err_msg;
  void *contents;
  size_t size; /* not off_t as size is set to the return value of fread which is size_t. Also off_t is a POSIX thing and size_t is std C */
} fl_content_t;

/*
 * The arguments to fl_read_full_file are the same as fopen.
 * It returns a char* that must be freed by the caller
 */
fl_content_t fl_read_file_str(const char *restrict path, const char *restrict mode);
void fc_deinit(fl_content_t *fc);

#endif // ZDX_FILE_H_

#ifdef ZDX_FILE_IMPLEMENTATION

#define fc_dbg(label, fc) dbg("%s valid %d \t| err %s \t| contents (%p) %s", \
                              (label), (fc).is_valid, (fc).err_msg, ((void *)(fc).contents), (char *)(fc).contents)

/* Guarded as unistd.h, sys/stat.h and friends are POSIX specific */
#if defined(__unix__) || defined(__unix) || defined(__linux__) || defined(__APPLE__) || defined(__MACH__)

/* needed for PATH_MAX */
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

fl_content_t fl_read_file_str(const char *restrict path, const char *restrict mode)
{
  dbg(">> path %s \t| mode %s", path, mode);

  /* guarded as it does a stack allocation */
#ifdef ZDX_TRACE_ENABLE
  char cwd[PATH_MAX] = {0};
  dbg(">> cwd %s", getcwd(cwd, sizeof(cwd)));
#endif

  /* init return type */
  fl_content_t fc = {
    .is_valid = false,
    .err_msg = NULL,
    .contents = NULL,
    .size = 0
  };

  /* open file */
  FILE *f = fopen(path, mode);

  if (f == NULL) {
    fclose(f);
    fc.is_valid = false;

    fc_dbg("<<", fc);
    return fc;
  }

  /* stat file for file size */
  struct stat s;

  if (fstat(fileno(f), &s) != 0) {
    fclose(f);
    fc.is_valid = false;
    fc.err_msg = strerror(errno);

    fc_dbg("<<", fc);
    return fc;
  }

  /* read full file and close */
  char *contents_buf = FL_MALLOC((s.st_size + 1) * sizeof(char));
  size_t bytes_read = fread(contents_buf, sizeof(char), s.st_size, f);
  fclose(f);

  if (ferror(f)) {
    FL_FREE(contents_buf);
    fc.is_valid = false;
    fc.err_msg = "Reading file failed";

    fc_dbg("<<", fc);
    return fc;
  }

  contents_buf[bytes_read] = '\0';

  fc.is_valid = true;
  fc.err_msg = NULL;
  fc.size = bytes_read; /* as contents_buf is truncated with \0 at index bytes_read */
  fc.contents = (void *)contents_buf;

  fc_dbg("<<", fc);
  return fc;
}

void fc_deinit(fl_content_t *fc)
{
  fc_dbg(">>", *fc);

  FL_FREE(fc->contents);
  fc->contents = NULL;
  fc->size = 0;
  fc->is_valid = false;
  fc->err_msg = NULL;

  fc_dbg("<<", *fc);
  return;
}

#elif defined(_WIN32) || defined(_WIN64)
/* No support for windows yet */
#else
/* Unsupported OSes */
#endif

#endif // ZDX_FILE_IMPLEMENTATION
