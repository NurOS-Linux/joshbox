#include "../include/common.h"
#include <fcntl.h>

#define BUFFER_SIZE 8192

static int is_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return 0;
}

static void get_basename(const char *path, char *basename, size_t size) {
    const char *last_slash = strrchr(path, '/');
    if (last_slash) {
        snprintf(basename, size, "%s", last_slash + 1);
    } else {
        snprintf(basename, size, "%s", path);
    }
}

static int move_file_fallback(const char *src, const char *dst) {
    int src_fd, dst_fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;
    struct stat src_stat;

    src_fd = open(src, O_RDONLY);
    if (src_fd == -1) {
        fprintf(stderr, "mv: cannot open '%s': %s\n", src, strerror(errno));
        return EXIT_FAILURE;
    }

    if (fstat(src_fd, &src_stat) == -1) {
        fprintf(stderr, "mv: cannot stat '%s': %s\n", src, strerror(errno));
        close(src_fd);
        return EXIT_FAILURE;
    }

    dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, src_stat.st_mode & 0777);
    if (dst_fd == -1) {
        fprintf(stderr, "mv: cannot create '%s': %s\n", dst, strerror(errno));
        close(src_fd);
        return EXIT_FAILURE;
    }

    while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
        bytes_written = write(dst_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            fprintf(stderr, "mv: write error to '%s': %s\n", dst, strerror(errno));
            close(src_fd);
            close(dst_fd);
            unlink(dst);
            return EXIT_FAILURE;
        }
    }

    if (bytes_read == -1) {
        fprintf(stderr, "mv: read error from '%s': %s\n", src, strerror(errno));
        close(src_fd);
        close(dst_fd);
        unlink(dst);
        return EXIT_FAILURE;
    }

    close(src_fd);
    close(dst_fd);

    if (unlink(src) == -1) {
        fprintf(stderr, "mv: cannot remove '%s': %s\n", src, strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int move_file(const char *src, const char *dst, int flags) {
    (void)flags;

    if (rename(src, dst) == 0) {
        return EXIT_SUCCESS;
    }

    if (errno == EXDEV) {
        return move_file_fallback(src, dst);
    }

    fprintf(stderr, "mv: cannot move '%s' to '%s': %s\n", src, dst, strerror(errno));
    return EXIT_FAILURE;
}

int cmd_mv(int argc, char **argv) {
    int flags = 0;
    int i;
    int arg_start = 1;
    char dst_path[PATH_BUFFER];

    for (i = 1; i < argc && argv[i][0] == '-' && argv[i][1] != '\0'; i++) {
        fprintf(stderr, "mv: option '%s' not supported yet\n", argv[i]);
        return EXIT_FAILURE;
    }

    arg_start = i;

    if (argc - arg_start < 2) {
        fprintf(stderr, "mv: missing file operand\n");
        fprintf(stderr, "Usage: mv SOURCE DEST\n");
        fprintf(stderr, "   or: mv SOURCE... DIRECTORY\n");
        return EXIT_FAILURE;
    }

    const char *dest = argv[argc - 1];
    int is_dest_dir = is_directory(dest);
    int num_sources = argc - arg_start - 1;

    if (num_sources > 1 && !is_dest_dir) {
        fprintf(stderr, "mv: target '%s' is not a directory\n", dest);
        return EXIT_FAILURE;
    }

    for (i = arg_start; i < argc - 1; i++) {
        const char *src = argv[i];
        char src_realpath[PATH_BUFFER];
        char dst_realpath[PATH_BUFFER];

        if (is_dest_dir) {
            char basename[NAME_BUFFER];
            get_basename(src, basename, sizeof(basename));
            snprintf(dst_path, sizeof(dst_path), "%s/%s", dest, basename);
        } else {
            snprintf(dst_path, sizeof(dst_path), "%s", dest);
        }

        if (realpath(src, src_realpath) && realpath(dst_path, dst_realpath)) {
            if (strcmp(src_realpath, dst_realpath) == 0) {
                fprintf(stderr, "mv: '%s' and '%s' are the same file\n", src, dst_path);
                return EXIT_FAILURE;
            }
        }

        if (move_file(src, dst_path, flags) != EXIT_SUCCESS) {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
