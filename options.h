#ifndef UTIL_H
#define UTIL_H

#include <sys/types.h>
#include <errno.h>

#define kUMASK_OPTIONS_MAX_SZ 10
#define _XOPEN_SOURCE 500           //especially for "pread()" and "pwrite()"

#define WHOLEFILE -1
#define TEMP_FILE_FILENAME "1"

struct CopymasterOptionsLseek {
    int x;
    off_t pos1;
    off_t pos2;
    size_t num;
};

struct CopymasterOptions {
    char * infile;
    const char * outfile;
    int fast;
    int slow;
    int create;
    mode_t create_mode;
    int overwrite;
    int append;
    int lseek;
    struct CopymasterOptionsLseek lseek_options;
    int directory;
    int delete_opt;
    int chmod;
    mode_t chmod_mode;
    int inode;
    ino_t inode_number;
    int umask;
    char umask_options[kUMASK_OPTIONS_MAX_SZ][4];
    int link;
    int truncate;
    off_t truncate_size;
    int sparse;
};

struct CopymasterOptions ParseCopymasterOptions(int argc, char *argv[]);

void FatalError(char c, const char* msg, int exit_status);

enum globalErrors {
    GE_NOFLAGS = 21,
    GE_CREATE = 23,
    GE_OVERWRITE = 24,
    GE_APPEND = 22,
    GE_LSEEK = 33,
    GE_DIRECTORY = 28,
    GE_DELETE = 26,
    GE_INODE = 27,
    GE_UMASK = 32,
    GE_LINK = 30,
    GE_TRUNCATE = 31,
    GE_SPARSE = 41,
    GE_CHMODE = 77
    //GE_CHMODE             ???

} globalErrors;

// different error values have to be adjusted according to ones mentioned in the task
enum errors {
    SUCCESS,
    E_INFILE,
    E_UNEXPECTED_ERROR,
    E_READ,
    E_WRITE,
    E_ARGS,
    E_CREATE_EXISTS,
    E_CREATE_MODE,
    E_OVERWRITE_NEXISTS,
    E_APPEND_NEXISTS,
    E_OPENING,
    E_CHMODE,
    E_CHMODE_MODE,
    E_TRUNC,
    E_TRUNC_SIZE,
    E_DELETE,
    E_INODE_STAT,
    E_INODE_NUM,
    E_LINK,
    E_LINK_NEXISTS,
    E_DIRECTORY,
    E_SEEK,
    E_SEEK_IN,
    E_SEEK_OUT,
    E_SEEK_OPENING,
    E_FSTAT,
    E_MALLOC,
    E_INVALID_UMASK,
    E_UMASK_CONVERTION
    // end so on (for more accurate tracing the execution of a program)
} errors;

size_t fast_copy(int fd1, int fd2, off_t fileOffset1, off_t fileOffset2, int amount);
size_t slow_copy(int fd1, int fd2, off_t fileOffset1, off_t fileOffset2, int amount);

size_t args_control(struct CopymasterOptions cpm_options);
int magic(struct CopymasterOptions cpm_options);
int traceMagicResult(int magicResult);

char *get_perms(mode_t st);
size_t ls_l(const char *path, FILE *fp);
size_t sparse(int fdin, int fdout);
_Bool is_mask_valid(char umask_options[kUMASK_OPTIONS_MAX_SZ][4]);
mode_t umaskAdjustment(char umask_options[kUMASK_OPTIONS_MAX_SZ][4]);

#endif /* UTIL_H */
