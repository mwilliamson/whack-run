#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#ifndef CLONE_NEWNS
#define CLONE_NEWNS 0x00020000
#endif


char* apps_dest_dir = "/usr/local/whack";
        
int unshare_mount_namespace() {
    return syscall(__NR_unshare, CLONE_NEWNS);
}

int directory_exists(char* path) {
    struct stat st;
    return stat(path, &st) == 0;
}

int ensure_apps_dest_dir_exists() {
    if (directory_exists(apps_dest_dir)) {
        return 0;
    } else {
        return mkdir(apps_dest_dir, 0755);
    }
}

int mount_bind(char* src_path, char* dest_path) {
    return mount(src_path, dest_path, NULL, MS_BIND, NULL);
}

int str_starts_with(const char *str, const char *prefix) {
    size_t prefix_len = strlen(prefix);
    size_t str_len = strlen(str);
    return str_len < prefix_len ? 0 : strncmp(prefix, str, prefix_len) == 0;
}

int should_fork(int argc, char **argv) {
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--fork") == 0) {
            return 1;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "--source-hash") == 0) {
        printf("%s\n", WHACK_RUN_SOURCE_HASH);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        printf("%s\n", WHACK_VERSION);
        return 0;
    }
    
    int app_index = 2;
    while (app_index < argc && str_starts_with(argv[app_index], "--")) {
        app_index++;
    }
    
    if (app_index >= argc) {
        printf("Usage: %s <apps-dir> [--fork] <app> <args>\n", argv[0]);
        return 1;
    }
    if (unshare_mount_namespace() != 0) {
        printf("ERROR: Could not unshare mount namespace\n");
        return 1;
    }
    if (ensure_apps_dest_dir_exists() != 0) {
        printf("ERROR: Could not create %s\n", apps_dest_dir);
        return 1;
    }
    char* apps_src_dir = argv[1];
    if (mount_bind(apps_src_dir, apps_dest_dir) != 0) {
        printf("ERROR: Could not mount %s\n", apps_src_dir);
        return 1;
    }
    
    if (setgid(getgid()) != 0) {
        printf("ERROR: Could not drop group privileges");
        return 1;
    }
    if (setuid(getuid()) != 0) {
        printf("ERROR: Could not drop user privileges");
        return 1;
    }
    
    char* app = argv[app_index];
    char** app_args = (char**)malloc(sizeof(char*) * (argc - 1));
    for (int i = 0; i < argc - app_index; i++) {
        app_args[i] = argv[i + app_index];
    }
    app_args[argc - app_index] = 0;
    
    if (should_fork(app_index, argv)) {
        int pid = fork();
        if (pid < 0) {
            printf("ERROR: failed to fork\n");
            return 1;
        }
        if (pid > 0) {
            int status = 0;
            waitpid(pid, &status, 0);

            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                return 128 + WTERMSIG(status);
            } else {
                return 255;
            }
        }
    }
    
    if (execvp(app, app_args) != 0) {
        printf("ERROR: failed to exec %s\n", app);
        return 1;
    }
    
    return 2;
}
