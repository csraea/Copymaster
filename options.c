#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>

#include "options.h"


struct CopymasterOptions ParseCopymasterOptions(int argc, char *argv[]) 
{
    extern int optind;
    extern char* optarg;

    struct CopymasterOptions cpm_options = {0};
    
    char c = 0, x = 0, *tok = 0;
    unsigned long long num = 0;
    long pos1 = -1, pos2 = -1;
    int i = 0;
    
    const char* usage_error_msg_format = "Usage: %s [OPTION]... SOURCE DEST\n(%s)\n";
    
    if (argc <= 0) {
        fprintf(stderr, usage_error_msg_format, "copymaster", "argc <= 0");
        exit(EXIT_FAILURE);
    }
    
    while (1)
    {
        // viac informacii o spracovani jednotlivych prepinacov 
        //  - pozri: man 3 getopt
        
        int option_index = 0;
        
        static struct option long_options[] = {
            { "fast",      no_argument,       0, 'f' },
            { "slow",      no_argument,       0, 's' }, 
            { "create",    required_argument, 0, 'c' },
            { "overwrite", no_argument,       0, 'o' }, 
            { "append",    no_argument,       0, 'a' }, 
            { "lseek",     required_argument, 0, 'l' }, 
            { "directory", required_argument, 0, 'D' }, 
            { "delete",    no_argument,       0, 'd' },
            { "chmod",     required_argument, 0, 'm' }, 
            { "inode",     required_argument, 0, 'i' },
            { "umask",     required_argument, 0, 'u' }, 
            { "link",      no_argument,       0, 'K' },
            { "truncate",  required_argument, 0, 't' },
            { "sparse",    no_argument,       0, 'S' },
            { 0,             0,               0,  0  },
        };
        c = getopt_long(argc, argv, "fsc:oal:Ddm:i:u:Kt:S", 
                        long_options, &option_index);
        if (c == -1)
            break; 
        
        switch (c)
        {
            case 'f':
                cpm_options.fast = 1;
                break;
            case 's': 
                cpm_options.slow = 1;
                break;
            case 'c': 
                cpm_options.create = 1;
                sscanf(optarg, "%o", &cpm_options.create_mode);
                break;
            case 'o': 
                cpm_options.overwrite = 1;
                break;
            case 'a': 
                cpm_options.append = 1;
                break;
            case 'l': 
                cpm_options.lseek = 1;
                i = sscanf(optarg, "%c,%ld,%ld,%llu", &x, &pos1, &pos2, &num);
                if (i < 4) {
                    fprintf(stderr, usage_error_msg_format, argv[0], "lseek - failed to scan 4 values");
                    exit(EXIT_FAILURE);
                }
                cpm_options.lseek_options.pos1 = (off_t)pos1;
                cpm_options.lseek_options.pos2 = (off_t)pos2;
                cpm_options.lseek_options.num = (size_t)num;
                switch (x) 
                {
                    case 'b':
                        cpm_options.lseek_options.x = SEEK_SET;
                        break;
                    case 'e':
                        cpm_options.lseek_options.x = SEEK_END;
                        break;
                    case 'c':
                        cpm_options.lseek_options.x = SEEK_CUR;
                        break;
                    default:
                        fprintf(stderr, usage_error_msg_format, argv[0], "lseek - invalid value of x");
                        exit(EXIT_FAILURE);
                        break;
                }
                break;
            case 'D': 
                cpm_options.directory = 1;
                break;
            case 'd': 
                cpm_options.delete_opt = 1;
                break;
            case 'm': 
                cpm_options.chmod = 1;
                sscanf(optarg, "%o", &cpm_options.chmod_mode);
                break;
            case 'i': 
                cpm_options.inode = 1;
                sscanf(optarg, "%lu", &cpm_options.inode_number);
                break;
            case 'u': 
                cpm_options.umask = 1;
                tok = strtok(optarg, ",");
                i = 0;
                while (tok != NULL && i < 9) {
                    if (strlen(tok) != 3) {
                        fprintf(stderr, usage_error_msg_format, argv[0], "umask - unexpected value of UTR option");
                        exit(EXIT_FAILURE);
                    }
                    strncpy(cpm_options.umask_options[i], tok, 3);
                    tok = strtok(NULL, ",");
                    ++i;
                }
                memset(cpm_options.umask_options[i], 0, 4);
                break;
            case 'K': 
                cpm_options.link = 1;
                break;
            case 't':
                cpm_options.truncate = 1;
                sscanf(optarg, "%ld", &pos1);
                cpm_options.truncate_size = (off_t)pos1; 
                break;
            case 'S': 
                cpm_options.sparse = 1;
                break;
            case '?':
            default:
                fprintf(stderr, usage_error_msg_format, argv[0]);
                exit(EXIT_FAILURE);
        }
    }
        
    // Kontrola ci boli zadanie parametre infile a outfile
    if (optind + 2 != argc) {
        fprintf(stderr, usage_error_msg_format, argv[0], "infile or outfile is missing");
        exit(EXIT_FAILURE);
    }
    
    cpm_options.infile = argv[optind];
    cpm_options.outfile = argv[optind + 1];
    
    return cpm_options;
}


// |######################################################################|
// | * * * * * * * * * * * * *   MY FUNCTIONS   * * * * * * * * * * * * * |
// V                                                                      V

int traceMagicResult(int magicResult) {
    switch(magicResult) {
        case E_INFILE:
            perror("SUBOR NEEXISTUJE");
            return GE_NOFLAGS;
        case E_READ: case E_WRITE:
            perror("INA CHYBA");
            return GE_NOFLAGS;
        case E_CREATE_EXISTS:
            perror("SUBOR EXISTUJE");
            return GE_CREATE;
        case E_CREATE_MODE:
            perror("CHYBNE PRAVA");
            return GE_CREATE;
        case E_APPEND_NEXISTS:
            perror("SUBOR NEEXISTUJE");
            return GE_APPEND;
        case E_APPEND_PERMISSIONS:
            perror("INA CHYBA");
            return GE_APPEND;
        case E_OVERWRITE_NEXISTS:
            perror("SUBOR NEEXISTUJE");
            return GE_OVERWRITE;
        case E_DELETE:
            perror("SUBOR NEBOL ZMAZANY");
            return GE_DELETE;
        case E_CHMODE_MODE:
            perror("ZLE PRAVA");
            return GE_CHMODE;
        case E_CHMODE:
            perror("INA CHYBA");
            return GE_CHMODE;
        case E_SEEK_OUT:
            perror("CHYBA POZICIE outfile");
            return GE_LSEEK;
        case E_SEEK_IN:
            perror("CHYBA POZICIE infile");
            return GE_LSEEK;
        case E_SEEK_OPENING: case E_SEEK:
            perror("INA CHYBA");
            return GE_LSEEK;
        case E_LINK_NEXISTS:
            perror("VSTUPNY SUBOR NEEXISTUJE");
            return GE_LINK;
        case E_LINK:
            perror("VYSTUPNY SUBOR NEVYTVORENY");
            return GE_LINK;
        case E_TRUNC:
            perror("VSTUPNY SUBOR NEZMENENY");
            return GE_TRUNCATE;
        case E_TRUNC_SIZE:
            perror("INA CHYBA");
            return GE_TRUNCATE;
        case E_INVALID_UMASK:
            perror("ZLA MASKA");
            return GE_UMASK;
        case E_UMASK_CONVERTION:
            perror("INA CHYBA");
            return GE_UMASK;
        case E_INODE_NUM:
            perror("ZLY INODE");
            return GE_INODE;
        case E_INODE_STAT:
            perror("ZLY TYP VSTUPNEHO SUBORU");
            return GE_INODE;
    }
    return SUCCESS;
}

int magic(struct CopymasterOptions cpm_options) {
    
    //necessary variables
    off_t fileOffset1 = 0;
    off_t fileOffset2 = 0;
    int amount = WHOLEFILE;
    size_t copying_mode = 0;
    int flags = O_WRONLY;

BFLAGS:

    // setting copying mode
    if(cpm_options.slow) {
        copying_mode = 1;
    }

    // // validating "sparse" conditions
    if(cpm_options.sparse) {
    //     int fd1 = open(cpm_options.infile, O_RDONLY);
    //     int fd2 = creat(cpm_options.outfile, 0777);

    //     size_t ret = sparse_copy(fd1, fd2);
    //     close(fd1);
    //     close(fd2);
    //     return ret;
    // }
        #ifndef SPARSEFILE_SIZETYPE
        #define SPARSEFILE_SIZETYPE int
        #endif
        // for detailed info read "NOTES"(1.2)
        #ifndef SPARSEFILE_LSEEK
        #define SPARSEFILE_LSEEK lseek 
        #endif

        int fd1 = open(cpm_options.infile, O_RDONLY);
        if(fd1 == -1) return E_OPENING;
        int fd2 = open(cpm_options.outfile, O_RDWR);
        if(fd2 == -1) { 
            // create a sparse file to copy in (no mentioned in the task what to do if doesn't exists)
            fd2 = creat(cpm_options.outfile, 0666);
            if(fd2 == -1) {
                return E_OPENING;
            }
            errno = 0;
        }

        size_t ret = sparse(fd1, fd2);
        return ret; // if sparse flag is going to be checked without any flags; else -- continue
    }

    // validating "link" conditions
    if(cpm_options.link) {
        int fd1 = open(cpm_options.infile, O_RDONLY);
        if(fd1 == -1) return E_LINK_NEXISTS;
        if(link(cpm_options.infile, cpm_options.outfile) == -1) {
            return E_LINK;
        } else {
            return SUCCESS;
        }
    }

    // inode part
    if(cpm_options.inode) {
        struct stat buf;
        if(stat(cpm_options.infile, &buf) == -1){
            return E_INODE_STAT;
        } else {
            if(cpm_options.inode_number != buf.st_ino){
                return E_INODE_NUM;
            }
            //create the file if doesn't exist
            int ret = open(cpm_options.outfile, O_RDWR);
            if(ret == -1) {
                cpm_options.create = 1;
                cpm_options.create_mode = 0766;
            } else {
                close(ret);
            }
        }
    }

    // directory part
    if(cpm_options.directory) {
        // for detailed info read "NOTES"(2.1)
        int fd3 = creat(TEMP_FILE_FILENAME, 0777);
        close(fd3);
        FILE *fp = fopen(TEMP_FILE_FILENAME, "w+");
        if(fp != NULL){
            size_t ret = ls_l(cpm_options.infile, fp);
            fclose(fp);
            if(ret != SUCCESS) {
                unlink(TEMP_FILE_FILENAME);
                return ret;
            } else {
                strcpy(cpm_options.infile, TEMP_FILE_FILENAME);
                cpm_options.delete_opt = 1;
                
                //check whether the outfile exists
                int fd2 = open(cpm_options.outfile, O_RDONLY);
                if(fd2 == -1) {
                    cpm_options.create = 1;
                    cpm_options.create_mode = 0766;
                } else {
                    close(fd2);
                }
            }
        } else {
            return E_CREATE_MODE;
        }
    }
    // validating "create" conditions
    if(cpm_options.create) {
        int fd2 = 0;
        if((fd2 = open(cpm_options.outfile, O_RDONLY)) != -1) {
            close(fd2);
            return E_CREATE_EXISTS;
        } else {
            //close(fd2); //?
            flags |= O_CREAT;
            if(cpm_options.create_mode > 00777) {
                return E_CREATE_MODE;
            }
        }
    }
    // validating "append" conditions
    if(cpm_options.append) {
        int fd2 = 0;
        if((fd2 = open(cpm_options.outfile, O_RDONLY)) == -1) {
            return E_APPEND_NEXISTS;
        } else {
            close(fd2);
            fd2 = open(cpm_options.outfile, O_RDWR);    // check permissions 
            if(fd2 == -1) return E_APPEND_PERMISSIONS;  // whether write pesmissions are present
            fileOffset2 = lseek(fd2, 0, SEEK_END);
            close(fd2);
        }
    }
    // validating "overwrite" conditions
    if(cpm_options.overwrite) {
        int fd2 = open(cpm_options.outfile, O_RDONLY);
        if(fd2 == -1) {
            return E_OVERWRITE_NEXISTS;
        } else {
            flags |= O_TRUNC;
            close(fd2);
        }
    }
    // lseek part
    if(cpm_options.lseek) {
        int fd1 = 0, fd2 = 0;
        if((fd1 = open(cpm_options.infile, O_RDONLY)) == -1 || (fd2 = open(cpm_options.outfile, flags, cpm_options.create_mode)) == -1) {
            (fd1 == -1) ? close(fd1) : close(fd2);
            return E_SEEK_OPENING;
        } else {
            fileOffset1 = cpm_options.lseek_options.pos1;
            amount = cpm_options.lseek_options.num;
            fileOffset2 = lseek(fd2, cpm_options.lseek_options.pos2, cpm_options.lseek_options.x);
            close(fd1);
            close(fd2);
            if(fileOffset1 < 0) return E_SEEK_IN;
            if(fileOffset2 < 0) return E_SEEK_OUT;

        }
    }

    // umask part
    if(cpm_options.umask){
        _Bool ret = is_mask_valid(cpm_options.umask_options);
        if(ret != true) return E_INVALID_UMASK;
        else {
            mode_t mask = umaskAdjustment(cpm_options.umask_options);
            printf("%o\n", mask);
            umask(mask);
        }
    }



    // copying
    int fd1 = open(cpm_options.infile, O_RDONLY);
    if(fd1 == -1){
        return E_INFILE;
    }
    int fd2 = open(cpm_options.outfile, flags, cpm_options.create_mode);

    size_t (*copyingFunc[])(int, int, off_t, off_t, int) = {fast_copy, slow_copy};
    size_t copyResult = (*copyingFunc[copying_mode])(fd1, fd2, fileOffset1, fileOffset2, amount);
    if(copyResult != SUCCESS) return copyResult;



    // validating "delete" part
    if(cpm_options.delete_opt) {
        int unlinkResult = unlink(cpm_options.infile);
        if(unlinkResult == -1) return E_DELETE;
    }

    // validating "truncate" attributes
    if(cpm_options.truncate) {
        if(cpm_options.truncate_size < 0) return E_TRUNC_SIZE;
        int truncResult = truncate(cpm_options.infile, cpm_options.truncate_size);
        if(truncResult == -1) return E_TRUNC;
    }

    // validating "chmod" part
    if(cpm_options.chmod) {
        if(cpm_options.chmod_mode > 00777) {
            return E_CHMODE_MODE;
        }
        int ret = fchmod(fd2, cpm_options.chmod_mode);
        if(ret == -1) {
            cpm_options.create = 1;
            cpm_options.create_mode = 0766;
            close(fd1);
            close(fd2);
            goto BFLAGS;
        }
    }

    close(fd1);
    close(fd2);

    return 0;
}

size_t sparse_copy(int fd1, int fd2){
    int i, holes = 0;
    ssize_t numRead;
    char buf[BUFSIZ];

    while ((numRead = read(fd1, buf, BUFSIZ)) > 0) {
        for (i = 0; i < numRead; i++) {
            if (buf[i] == '\0') {
                holes++;
                continue;
            } else if (holes > 0) {
                lseek(fd2, holes, SEEK_CUR);
                holes = 0;
            }
            if (write(fd2, &buf[i], 1) != 1) return E_SPARSE_WRITE;
        }
    }
    return SUCCESS;
}

size_t args_control(struct CopymasterOptions cpm_options) {
    //checking the compatibility of arguments
    if (cpm_options.fast && cpm_options.slow)           return E_ARGS;
    if (cpm_options.create && cpm_options.append)       return E_ARGS;
    if (cpm_options.create && cpm_options.overwrite)    return E_ARGS;
    if (cpm_options.append && cpm_options.overwrite)    return E_ARGS;
    if (cpm_options.lseek && cpm_options.append)        return E_ARGS;
    if (cpm_options.lseek && cpm_options.overwrite)     return E_ARGS;
    if (cpm_options.append && cpm_options.umask)        return E_ARGS;
    if (cpm_options.overwrite && cpm_options.umask)     return E_ARGS;
    if (cpm_options.lseek && cpm_options.umask)         return E_ARGS;
    if (cpm_options.directory && cpm_options.fast)      return E_ARGS; //?
    if (cpm_options.directory && cpm_options.slow)      return E_ARGS; //?
    if (cpm_options.directory && cpm_options.lseek)     return E_ARGS;
    if (cpm_options.directory && cpm_options.delete_opt)return E_ARGS;
    if (cpm_options.directory && cpm_options.truncate)  return E_ARGS;
    if (cpm_options.sparse && cpm_options.lseek)        return E_ARGS;
    if (cpm_options.sparse && cpm_options.directory)    return E_ARGS;
    if (cpm_options.sparse && cpm_options.truncate)     return E_ARGS;
    if (cpm_options.link && cpm_options.fast)           return E_ARGS;
    if (cpm_options.link && cpm_options.slow)           return E_ARGS;
    if (cpm_options.link && cpm_options.append)         return E_ARGS;
    if (cpm_options.link && cpm_options.create)         return E_ARGS;
    if (cpm_options.link && cpm_options.overwrite)      return E_ARGS;
    if (cpm_options.link && cpm_options.lseek)          return E_ARGS;
    if (cpm_options.link && cpm_options.delete_opt)     return E_ARGS;
    if (cpm_options.link && cpm_options.chmod)          return E_ARGS;
    if (cpm_options.link && cpm_options.inode)          return E_ARGS;
    if (cpm_options.link && cpm_options.umask)          return E_ARGS;
    if (cpm_options.link && cpm_options.directory)      return E_ARGS;

    return SUCCESS;
}

_Bool is_mask_valid(char umask_options[kUMASK_OPTIONS_MAX_SZ][4]){
    for(size_t i = 0; i < kUMASK_OPTIONS_MAX_SZ; i++) {
        if(umask_options[i][0] != '\0' && umask_options[i] != NULL && strlen(umask_options[i]) != 0){
            if(umask_options[i][0] != 'u' && umask_options[i][0] != 'g' && umask_options[i][0] != 'o') return false;
            if(umask_options[i][2] != 'r' && umask_options[i][2] != 'w' && umask_options[i][2] != 'x') return false;
            if(umask_options[i][1] != '-' && umask_options[i][1] != '+' && umask_options[i][1] != '=') return false;
        }
    }
    return true;
}

mode_t umaskAdjustment(char umask_options[kUMASK_OPTIONS_MAX_SZ][4]) {
    int newMask[4] = {0, 0, 0, 0};
    size_t bitNum = 0;
    size_t permission = 0;
    char buf[5];
    for(size_t i = 0; i < kUMASK_OPTIONS_MAX_SZ; i++) {
        //  UGO
        switch(umask_options[i][0]) {
            case 'u': case 'U':
                bitNum = 1;
                break;
            case 'g': case 'G':
                bitNum = 2;
                break;
            case 'o': case 'O':
                bitNum = 3;
                break;
        }
        //  PERMISSIONS
        switch(umask_options[i][2]){
            case 'r': case 'R':
                permission = 4;
                break;
            case 'w': case 'W':
                permission = 2;
                break;
            case 'x': case 'X':
                permission = 1;
                break;
        }
        //  WHAT TO DO
        switch(umask_options[i][1]){
            case '+':
                if(newMask[bitNum] >= (signed) permission && newMask[bitNum] + permission != 7) newMask[bitNum] -= permission;
                break;
            case '-':
                if(newMask[bitNum] <= (signed) permission && newMask[bitNum] - permission != 0) newMask[bitNum] += permission;
                break;
            case '=':
                newMask[bitNum] = permission;
        }
        // //  SOME SECURITY STUFF ?????????????????/
        // for(size_t j = 0; j < 4; j++){
        //     if(newMask[j] < 0) newMask[j] = 0;
        // }
    }
    // initialze buffer 
    memset(buf, 0, sizeof(buf));
    for(ssize_t l = 0; l < 4; l++) {
        buf[l] = (char)(newMask[l] + 48);
    }

    //  FINALLY MAGIC!
    long ret = strtol(buf, NULL, 8);
    if(ret < 0) return E_UMASK_CONVERTION;
    return (mode_t) ret;
}

char *get_perms(mode_t st) {
    char *perms = calloc(11, sizeof(char));
    if(st && S_ISREG(st)) perms[0]='-';
    else if(st && S_ISDIR(st)) perms[0]='d';
    else if(st && S_ISFIFO(st)) perms[0]='|';
    else if(st && S_ISSOCK(st)) perms[0]='s';
    else if(st && S_ISCHR(st)) perms[0]='c';
    else if(st && S_ISBLK(st)) perms[0]='b';
    else perms[0]='l';  // S_ISLNK
    perms[1] = (st && S_IRUSR) ? 'r':'-';
    perms[2] = (st && S_IWUSR) ? 'w':'-';
    perms[3] = (st && S_IXUSR) ? 'x':'-';
    perms[4] = (st && S_IRGRP) ? 'r':'-';
    perms[5] = (st && S_IWGRP) ? 'w':'-';
    perms[6] = (st && S_IXGRP) ? 'x':'-';
    perms[7] = (st && S_IROTH) ? 'r':'-';
    perms[8] = (st && S_IWOTH) ? 'w':'-';
    perms[9] = (st && S_IXOTH) ? 'x':'-';
    perms[10] = '\0';
    return perms;
}

size_t ls_l(const char *path, FILE *fp) {

    //  Frankly speaking, the output format is stupid. Personally I'd prefer "ls -al" format.
    //  (some changes in date & time representation, shows hidden files)

    DIR * dir; 
    struct dirent * file; 
    struct stat sbuf;
    char buf[128];
    struct passwd pwent, * pwentp;
    struct group grp, * grpt;
    char datestring[256];
   
    dir = opendir(path);
    if(dir == NULL) {
        return E_DIRECTORY;
    }
    
    time_t now;
    time(&now);
    struct tm time, *current;
    current = localtime(&now);

    char *perms;
    while((file = readdir(dir)) != NULL) {
        stat(file->d_name, &sbuf);
        if(strcmp(file->d_name, TEMP_FILE_FILENAME) == 0) continue;
        //optionally the "." and ".." directories, hidden files
        if(strcmp(file->d_name, ".") == 0 || strcmp(file->d_name, "..") == 0 || file->d_name[0] == '.') continue;
        perms = get_perms(sbuf.st_mode);
        fprintf(fp, "%s", perms);
        fprintf(fp," %d", (int)sbuf.st_nlink);
        if (!getpwuid_r(sbuf.st_uid, &pwent, buf, sizeof(buf), &pwentp)) {
            fprintf(fp," %s", pwent.pw_name);
        } else {
            fprintf(fp," %d", sbuf.st_uid);
        }
        if (!getgrgid_r (sbuf.st_gid, &grp, buf, sizeof(buf), &grpt)) {
            fprintf(fp," %s", grp.gr_name);
        } else {
            fprintf(fp," %d", sbuf.st_gid);
        }
        fprintf(fp," %5d", (int)sbuf.st_size);
        localtime_r(&sbuf.st_mtime, &time);
        /* Get localized date string. */
        if(time.tm_year == current->tm_year) strftime(datestring, sizeof(datestring), "%b %d %T", &time);
        else strftime(datestring, sizeof(datestring), "%b %d %Y", &time);
 
        fprintf(fp," %s %s\n", datestring, file->d_name);
        
        free(perms);
    }
    closedir(dir);
    return SUCCESS;
}

/* sparse -- copy in to out while producing a sparse file */
size_t sparse(int fdin, int fdout) {
	static const char skipbyte = '\0';

	struct stat st;
	SPARSEFILE_SIZETYPE blocksize, skip, nskip, nbytes, eof, n, i;
	char* buf;

	/* get the blocksize used on the output media, allocate buffer */
	if (fstat(fdout, &st) == -1) {
		return E_FSTAT;
	}

	blocksize = st.st_blksize;
	buf = (char*)malloc(blocksize);
	if (!buf) {
		return E_MALLOC;
	} else {
        memset(buf, 0, sizeof(char)*blocksize);
    }

	for (eof = 0, skip = 0;;) {
		/* read exactly one block, if necessary in multiple chunks */
		for (nbytes = 0; nbytes < blocksize; nbytes += n) {
			n = read(fdin, &buf[nbytes], blocksize - nbytes);
			if (n == -1) { /* error -- don't write this block */
				free(buf);
				return E_READ;
			}
			if (n == 0) { /* eof */
				eof++;
				break;
			}
		}
		/* check if we can skip this part */
		nskip = 0;
		if (nbytes == blocksize) {
			/* linear (slow?) search for a byte other than skipbyte */
			for (n = 0; n < blocksize; n++) {
				if (buf[n] != skipbyte) {
					break;
				}
			}
			if (n == blocksize) {
				nskip = skip + blocksize;
				if (nskip > 0) { /* mind 31 bit overflow */
					skip = nskip;
					continue;
				}
				nskip = blocksize;
			}
		}
		/* do a lseek over the skipped bytes */
		if (skip != 0) {
			/* keep one block if we got eof, i.e. don't forget to write the last block */
			if (nbytes == 0) {
				/* note that the following implies using the eof flag */
				skip -= blocksize;
				nbytes += blocksize;
				/* we don't need to zero out buf since the last block was skipped, i.e. zero */
			}
			i = SPARSEFILE_LSEEK(fdout, skip, SEEK_CUR);
			if (i == -1) { /* error */
				free(buf);
				return E_SEEK;
			}
			skip = 0;
		}
		/* continue skipping if just skipped near overflow */
		if (nskip != 0) {
			skip = nskip;
			continue;
		}
		/* write exactly nbytes */
		for (n = 0; n < nbytes; n += i) {
			i = write(fdout, &buf[n], nbytes - n);
			if (
				i == -1 || /* error */
				i == 0 /* can't write?? */
			) {
				free(buf);
				return E_WRITE;
			}
		}
		if (eof) { /* eof */
			break;
		}
	}

	free(buf);
	return SUCCESS;
}


size_t fast_copy(int fd1, int fd2, off_t fileOffset1, off_t fileOffset2, int amount) {
    char buf[BUFSIZ * 5]; //especially for matching num of fuction calling
    memset(buf, 0, sizeof(buf));
    ssize_t ret;

    while ((ret = pread (fd1, buf, (amount > BUFSIZ*5 || amount == WHOLEFILE) ? BUFSIZ*5 : amount, fileOffset1)) != 0 && (amount - ret >= 0 || amount == WHOLEFILE)) {
        if (ret == -1) {
            if (errno == EINTR) continue; // handling some frequent interruptions
            return E_READ;
        }
        fileOffset1 += ret;
        if((ret = pwrite(fd2, buf, ret, fileOffset2)) != 0){
            if (ret == -1) {
                if (errno == EINTR) continue; // handling some frequent interruptions
                return E_WRITE;
            }
        }
        fileOffset2 += ret;
        if(amount != WHOLEFILE) amount -= (int) ret;
    }
    return SUCCESS;
}

size_t slow_copy(int fd1, int fd2, off_t fileOffset1, off_t fileOffset2, int amount){
    char buf;
    ssize_t ret;
    
    while((ret = pread(fd1, &buf, 1, fileOffset1)) != 0 && (amount == WHOLEFILE || amount > 0)){
        if(ret == -1){
            if (errno == EINTR) continue; // handling some frequent interruptions
            return E_READ;
        }
        fileOffset1++;
        if((ret = pwrite(fd2, &buf, 1, fileOffset2)) != 0){
            if(ret == -1){
                if (errno == EINTR) continue; // handling some frequent interruptions
                return E_WRITE;
            }
        }
        if(amount != WHOLEFILE) amount -= (int) ret;
        fileOffset2++;
    }
    return SUCCESS;
}

/*
    |###############################################################|
    | * * * * * * * * * * * * *   NOTES   * * * * * * * * * * * * * |
    V                                                               V


    $(1): Sparse Copying
        (1.1)
            * Size type used with offsets and read/write operations;
            * To avoid (the overhead of) successive lseek operations
            * with nul gaps of 2 GB or more on LP64 64 bit (Linux) systems:
            * compile with e.g.: -DSPARSEFILE_SIZETYPE=off_t
            * (where blksize_t, size_t, off_t, etc are all long long int).
        (1.2)
            * For support for files of 2 GB and more on 32 bit (Linux) systems:
            * compile with: -DSPARSEFILE_LSEEK=lseek64 -D_LARGEFILE64_SOURCE;
            * although we take care not to overflow in the skip parameter,
            * 32 bit lseek will likely fail without these options
            * with EOVERFLOW ('Value too large for defined data type')
            * as soon as the 2 GB file size limit is hit.
    $(2): Directory flag
        (2.1)
            The directory flag is compatible with some others. Thus, we need to save the output of the ls_l function
            into some buffer. The buffer is a newly created file, which contains the info which is obliged to be copied 
            according to other flags. 
             - create new file in the program directory
             - write all the info in it
             - save cpm.infile properties into buffer
             - replace cpm.infile with the newly created file from above
             - complete copying
             - return default infile into the cpm properties
             - delete the file with the original ls_l output
             - continue the execution of the program
             * Perhaps, it's possible to achieve the goal of printing directory info, using some dynamically allocated memory
            (via malloc, memset, free / calloc, free) and strcat / strcpy functions. In this way, the whole program complexity will be increased. 


    Contact autor:
    MAIL -          csraea@gmail.com
    TELEGAM -       t.me/csraea
    INSTAGRAM -     @korotetskiy_

    Korotetskiy©. All rights reserved. 2019.

*/