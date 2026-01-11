#include "../include/common.h"

static int get_terminal_width(void) {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
        return w.ws_col;
    }
    return 80;
}

static void format_size_human(off_t size, char *buf, size_t bufsize) {
    const char *units[] = {"", "K", "M", "G", "T"};
    int unit = 0;
    double dsize = size;

    while (dsize >= 1024.0 && unit < 4) {
        dsize /= 1024.0;
        unit++;
    }

    if (unit == 0) {
        snprintf(buf, bufsize, "%ld", (long)size);
    } else {
        snprintf(buf, bufsize, "%.1f%s", dsize, units[unit]);
    }
}

static void format_permissions(mode_t mode, char *buf) {
    buf[0] = S_ISDIR(mode) ? 'd' : (S_ISLNK(mode) ? 'l' : '-');
    buf[1] = (mode & S_IRUSR) ? 'r' : '-';
    buf[2] = (mode & S_IWUSR) ? 'w' : '-';
    buf[3] = (mode & S_IXUSR) ? 'x' : '-';
    buf[4] = (mode & S_IRGRP) ? 'r' : '-';
    buf[5] = (mode & S_IWGRP) ? 'w' : '-';
    buf[6] = (mode & S_IXGRP) ? 'x' : '-';
    buf[7] = (mode & S_IROTH) ? 'r' : '-';
    buf[8] = (mode & S_IWOTH) ? 'w' : '-';
    buf[9] = (mode & S_IXOTH) ? 'x' : '-';
    buf[10] = '\0';
}

static int cmp_names(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

static void print_long_format(const char *path, const char *name, int flags) {
    char full_path[PATH_BUFFER];
    struct stat st;
    char perms[11];
    char size_buf[32];
    char time_buf[64];
    struct passwd *pw;
    struct group *gr;
    struct tm *tm;

    snprintf(full_path, sizeof(full_path), "%s/%s", path, name);

    if (lstat(full_path, &st) == -1) {
        return;
    }

    format_permissions(st.st_mode, perms);

    pw = getpwuid(st.st_uid);
    gr = getgrgid(st.st_gid);

    if (flags & FLAG_HUMAN) {
        format_size_human(st.st_size, size_buf, sizeof(size_buf));
    } else {
        snprintf(size_buf, sizeof(size_buf), "%ld", (long)st.st_size);
    }

    tm = localtime(&st.st_mtime);
    strftime(time_buf, sizeof(time_buf), "%b %d %H:%M", tm);

    printf("%s %3ld %-8s %-8s %8s %s %s",
           perms,
           (long)st.st_nlink,
           pw ? pw->pw_name : "???????",
           gr ? gr->gr_name : "???????",
           size_buf,
           time_buf,
           name);

    if (S_ISLNK(st.st_mode)) {
        char link_target[PATH_BUFFER];
        ssize_t len = readlink(full_path, link_target, sizeof(link_target) - 1);
        if (len != -1) {
            link_target[len] = '\0';
            printf(" -> %s", link_target);
        }
    }

    printf("\n");
}

static void print_in_columns(char **names, int count) {
    int term_width = get_terminal_width();
    int max_len = 0;
    int i, col, cols, rows, row;

    for (i = 0; i < count; i++) {
        int len = strlen(names[i]);
        if (len > max_len) {
            max_len = len;
        }
    }

    max_len += 2;
    cols = term_width / max_len;
    if (cols < 1) cols = 1;

    rows = (count + cols - 1) / cols;

    for (row = 0; row < rows; row++) {
        for (col = 0; col < cols; col++) {
            i = row + col * rows;
            if (i < count) {
                printf("%-*s", max_len, names[i]);
            }
        }
        printf("\n");
    }
}

static int list_directory(const char *path, int flags) {
    DIR *dir;
    struct dirent *entry;
    char *names[MAX_ENTRIES];
    int count = 0;
    int i;

    dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "ls: cannot open directory '%s': %s\n", path, strerror(errno));
        return EXIT_FAILURE;
    }

    while ((entry = readdir(dir)) != NULL && count < MAX_ENTRIES) {
        if (!(flags & FLAG_ALL) && entry->d_name[0] == '.') {
            continue;
        }

        names[count] = strdup(entry->d_name);
        if (!names[count]) {
            fprintf(stderr, "ls: memory allocation failed\n");
            closedir(dir);
            return EXIT_FAILURE;
        }
        count++;
    }

    closedir(dir);

    qsort(names, count, sizeof(char *), cmp_names);

    if (flags & FLAG_LONG) {
        for (i = 0; i < count; i++) {
            print_long_format(path, names[i], flags);
        }
    } else {
        print_in_columns(names, count);
    }

    for (i = 0; i < count; i++) {
        free(names[i]);
    }

    return EXIT_SUCCESS;
}

int cmd_ls(int argc, char **argv) {
    int flags = 0;
    int i;
    const char *path = ".";
    int path_count = 0;

    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            int j;
            for (j = 1; argv[i][j] != '\0'; j++) {
                switch (argv[i][j]) {
                    case 'l':
                        flags |= FLAG_LONG;
                        break;
                    case 'a':
                        flags |= FLAG_ALL;
                        break;
                    case 'h':
                        flags |= FLAG_HUMAN;
                        break;
                    default:
                        fprintf(stderr, "ls: invalid option -- '%c'\n", argv[i][j]);
                        return EXIT_FAILURE;
                }
            }
        } else {
            path = argv[i];
            path_count++;
        }
    }

    if (path_count == 0) {
        path = ".";
    }

    return list_directory(path, flags);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: joshbox <command> [options]\n");
        printf("\nAvailable commands:\n");
        printf("  ls [options] [path]    List directory contents\n");
        printf("  cp SOURCE DEST         Copy files\n");
        printf("  mv SOURCE DEST         Move/rename files\n");
        printf("\nOptions for ls:\n");
        printf("  -l    Long format (permissions, owner, size, date)\n");
        printf("  -a    Show hidden files\n");
        printf("  -h    Human-readable sizes (use with -l)\n");
        return EXIT_SUCCESS;
    }

    if (strcmp(argv[1], "ls") == 0) {
        return cmd_ls(argc - 1, argv + 1);
    }

    if (strcmp(argv[1], "cp") == 0) {
        return cmd_cp(argc - 1, argv + 1);
    }

    if (strcmp(argv[1], "mv") == 0) {
        return cmd_mv(argc - 1, argv + 1);
    }

    fprintf(stderr, "joshbox: unknown command: %s\n", argv[1]);
    fprintf(stderr, "Available commands: ls, cp, mv\n");
    return EXIT_FAILURE;
}
