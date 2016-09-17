#if defined(__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif

#define _GNU_SOURCE

#if defined(__clang__)
  #pragma clang diagnostic pop
#endif

#include <string.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include "common.h"

typedef FILE* (*fopen_fn_t)(const char*, const char*);
static fopen_fn_t real_fopen = NULL;

typedef int (*open_fn_t)(const char*, int, ...);
static open_fn_t real_open = NULL;

static
bool ends_by(const char * const haystack, const char * const to_match_at_end)
{
  const size_t haystack_len = strlen(haystack);
  const size_t suffix_len = strlen(to_match_at_end);

  if (haystack_len < suffix_len)
  {
    return false;
  }

  return strncmp(&haystack[ haystack_len - suffix_len], to_match_at_end, suffix_len) == 0;
}

static
bool is_music_function_file(const char* const filename)
{
  return ends_by(filename, MUSIC_FUNCTIONS_END_PATH);
}

static
off_t  get_file_size(const char* const input_filename)
{

  struct stat input_file_stat;
  const int stat_ret_code = stat(input_filename, &input_file_stat);
  if (stat_ret_code != 0)
  {
    fprintf(stderr, "ERROR: couldn't stat file [%s] (%s)\n", input_filename, strerror(errno));
    return -1;
  }

  return input_file_stat.st_size;
}

static
bool copy_file_to_buffer(const char* const input_filename, char* const buffer, const size_t filesize)
{
  FILE* const fd = real_fopen(input_filename, "r");
  if (fd == NULL)
  {
    fprintf(stderr, "ERROR: failed to open file [%s] (%s)\n", input_filename, strerror(errno));
    return false;
  }

  const size_t nb_read = fread(buffer, (size_t) 1, filesize, fd);
  fclose(fd);

  const bool success = (nb_read == filesize);
  if (!success)
  {
      fprintf(stderr, "ERROR: failed to copy file [%s] into internal memory\n", input_filename);
  }

  return success;
}

static
bool patch_buffer_to_file(const char* const buffer, const size_t filesize, FILE* const output_fd)
{
  // this uses the fact that the first make-music appearing in the file is the correct one.
  // TODO, make it more robust, by checking it is in the make-repeat function.

  const char* const to_find_1 = "(make-music type";
  const size_t len1 = strlen(to_find_1);

  const char* const make_music_pos = memmem(buffer, filesize, to_find_1, len1);
  if (make_music_pos == NULL)
  {
    fprintf(stderr, "ERROR: couldn't patch the file. Could't find the string to modify there.\n");
    return false;
  }

  const size_t to_copy_at_begin = (size_t) (make_music_pos - buffer);
  fwrite(buffer, (size_t) 1, to_copy_at_begin, output_fd);

  const char* const unfold_repeat_str = "(make-music 'UnfoldedRepeatedMusic";
  const size_t len_unfold_repeat_str = strlen(unfold_repeat_str);
  fwrite(unfold_repeat_str, (size_t) 1, len_unfold_repeat_str, output_fd);

  const size_t to_copy_at_end = filesize - to_copy_at_begin - len1;
  fwrite(make_music_pos + len1, (size_t) 1, to_copy_at_end, output_fd);

  return true;
}

static
void debug_dump_buffer(const void * const buffer, size_t buf_len, const char * const filename)
{
  const char* const value = secure_getenv(DUMP_OUTPUT_DIR);
  if (value == NULL)
  {
    fprintf(stderr, "Failed to find the directory where to output debug dump\n"
	    "Missing [%s] environment variable\n", DUMP_OUTPUT_DIR);
    return;
  }

  const size_t len_value = strlen(value);

  char* fullpath = malloc(len_value + 1 + strlen(filename) + 1);
  if (fullpath == NULL)
  {
    fprintf(stderr, "Failed to allocate memory, no debug output\n");
    return;
  }
  strcpy(fullpath, value);
  fullpath[len_value] = '/';
  strcpy(fullpath + len_value + 1, filename);

  FILE* dst_stream = fopen(fullpath, "w");

  if (dst_stream == NULL)
  {
    fprintf(stderr, "Failed to open [%s] for writing. No debug output\n", fullpath);
    free(fullpath);
    return;
  }
  free(fullpath);

  fwrite(buffer, buf_len, (size_t) 1, dst_stream);
  fclose(dst_stream);
}

static
void debug_dump(FILE* fd, const char * const filename)
{
  fseek(fd, 0L, SEEK_END);
  const long size = ftell(fd);
  rewind(fd);
  char* const buffer = malloc((size_t) size);
  if (buffer == NULL)
  {
    fprintf(stderr, "Failed to allocate memory, no debug output\n");
    return;
  }

  const size_t nb_read = fread(buffer, (size_t) size, (size_t) 1, fd);
  if (nb_read != (size_t) size)
  {
    if (feof(fd))
    {
      fprintf(stderr, "Unexpected end of file found. Was the file modified externally while being used by this program?\n");
    }
    else
    {
      fprintf(stderr, "Error occured while reading file (%s)\n", strerror(errno));
    }
  }

  debug_dump_buffer(buffer, nb_read, filename);
  free(buffer);
  rewind(fd);
}

static
bool force_unfold_repeat(const char* const input_filename, FILE* const output_fd)
{
  // replaces the occurence of "(make-music type" inside the make-repeat function by "(make-music
  // UnfoldedRepeatedMusic"

  // method will be to allocate a big buffer, copy the full input file there, look for the place where the
  // make-music is in the make-repeat function, write everything from the beginning up to that place into the
  // output file, write "(make-music UnfoldedRepeatedMusic ", and then everything that appears after the
  // "(make-music type" in the buffer will be copied to the output file

  const off_t file_size = get_file_size(input_filename);
  if (file_size == -1)
  {
    return false;
  }

  char* const buffer = malloc((size_t) file_size);
  if (buffer == NULL)
  {
    fprintf(stderr, "ERROR: couldn't allocate memory to copy file [%s] into internal memory\n", input_filename);
    return false;
  }

  const bool copy_ret_code = copy_file_to_buffer(input_filename, buffer, (size_t) file_size);
  if (! copy_ret_code)
  {
    free(buffer);
    return false;
  }

  debug_dump_buffer(buffer, (size_t) file_size, MUSIC_FUNCTIONS_FILENAME);

  const bool patch_ret_code = patch_buffer_to_file(buffer, (size_t) file_size, output_fd);

  free(buffer);

  debug_dump(output_fd, PATCHED_FILE_NAME);

  return patch_ret_code;
}

static
FILE* get_modified_file(const char* pathname)
{
  FILE* const tmpfile_fd = tmpfile();
  if (tmpfile_fd == NULL)
  {
    fprintf(stderr, "Error failed to real-fopen the temporary file (%s)\n", strerror(errno));
    return NULL;
  }

  if (! force_unfold_repeat(pathname, tmpfile_fd))
  {
    fclose(tmpfile_fd);
    return NULL;
  }
  rewind(tmpfile_fd);
  return tmpfile_fd;
}

__attribute__((nonnull (1,2))) static
bool init_function_ptr(const char* const symbol, void** ptr_to_set)
{
  if (*ptr_to_set == NULL)
  {
    void* const objptr = dlsym(RTLD_NEXT, symbol);;
    *ptr_to_set = objptr;

    if (*ptr_to_set == NULL)
    {
      fprintf(stderr, "Error: failed to wrap symbol [%s]\n", symbol);
      return false;
    }
  }

  return true;
}

static
bool init_fn_ptr(void)
{
  return init_function_ptr("open", (void**) &real_open)
    && init_function_ptr("fopen", (void**) &real_fopen);
}

static
bool set_perms_if_out_file(const char * const pathname)
{
  const bool is_out_file = (ends_by(pathname, ".notes") || ends_by(pathname, ".sn2in"));
  if (! is_out_file)
  {
    return true;
  }

  const int ret_code = chmod(pathname, S_IRUSR | S_IWUSR);
  if ((ret_code != 0) && (errno != ENOENT))
  {
    fprintf(stderr, "Error: failed to change file permissions of [%s] (error %s)\n", pathname, strerror(errno));
    return false;
  }

  return true;
}

__nonnull ((1))
int open(const char *pathname, int flags, ...)
{
  if (!init_fn_ptr())
  {
    return -1;
  }

  set_perms_if_out_file(pathname);

  if (!is_music_function_file(pathname))
  {
    return real_open(pathname, flags);
  }

  FILE* const res = get_modified_file(pathname);
  if (res == NULL)
  {
    return -1;
  }

  return fileno(res);
}

__nonnull ((1))
int open64(const char *pathname, int flags, ...)
{
  return open(pathname, flags | O_LARGEFILE);
}


FILE *fopen(const char *pathname, const char *mode)
{
  if (!init_fn_ptr())
  {
    return NULL;
  }

  set_perms_if_out_file(pathname);

  if (!is_music_function_file(pathname))
  {
    return real_fopen(pathname, mode);
  }

  return get_modified_file(pathname);
}
