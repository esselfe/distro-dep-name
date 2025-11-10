#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>

#include "ddn_config.h"
#include "parser.h"

#define INITIAL_CAPACITY 32

static dependency_list_t* create_dependency_list(void) {
    dependency_list_t *list = malloc(sizeof(dependency_list_t));
    if (!list) return NULL;

    list->items = malloc(INITIAL_CAPACITY * sizeof(dependency_t));
    if (!list->items) {
        free(list);
        return NULL;
    }

    list->count = 0;
    list->capacity = INITIAL_CAPACITY;
    return list;
}

void add_dependency(dependency_list_t *list, const char *name, dependency_type_t type) {
    if (!list || !name) return;

    // Check for duplicates
    for (int i = 0; i < list->count; i++) {
        if (strcmp(list->items[i].name, name) == 0 && list->items[i].type == type) {
            return; // Already exists
        }
    }

    // Expand capacity if needed
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->items = realloc(list->items, list->capacity * sizeof(dependency_t));
        if (!list->items) return;
    }

    list->items[list->count].name = strdup(name);
    list->items[list->count].type = type;
    list->count++;
}

void free_dependency_list(dependency_list_t *list) {
    if (!list) return;

    for (int i = 0; i < list->count; i++) {
        free(list->items[i].name);
    }
    free(list->items);
    free(list);
}

// Parse #include statements from C/C++ files
static void parse_c_file(const char *filepath, dependency_list_t *list) {
    if (config.debug)
        printf("ddn:parse_c_file(): parsing '%s'\n", filepath);

    FILE *f = fopen(filepath, "r");
    if (!f) return;

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        // Skip whitespace
        char *p = line;
        while (*p && isspace(*p)) p++;

        // Check for #include
        if (*p == '#') {
            p++;
            while (*p && isspace(*p)) p++;

            if (strncmp(p, "include", 7) == 0) {
                p += 7;
                while (*p && isspace(*p)) p++;

                // Extract header name
                char *start = NULL;
                char *end = NULL;

                if (*p == '<') {
                    start = p + 1;
                    end = strchr(start, '>');
                } /* ignore local headers
                else if (*p == '"') {
                    start = p + 1;
                    end = strchr(start, '"');
                } */

                if (start && end) {
                    size_t len = end - start;
                    char *header = malloc(len + 1);
                    strncpy(header, start, len);
                    header[len] = '\0';

                    if (config.debug) printf("  found header '%s'\n", header);
                    add_dependency(list, header, DEP_TYPE_HEADER);
                    free(header);
                }
            }
        }
    }

    fclose(f);
}

// Parse -l flags from Makefile
static void parse_makefile(const char *filepath, dependency_list_t *list) {
    if (config.debug)
        printf("ddn:parse_makefile(): parsing '%s'\n", filepath);

    FILE *f = fopen(filepath, "r");
    if (!f) return;

    char line[2048];
    while (fgets(line, sizeof(line), f)) {
        char *p = line;

        // Look for -l flags
        while ((p = strstr(p, "-l"))) {
            p += 2;

            // Extract library name
            char libname[256] = {0};
            int i = 0;
            while (*p && (isalnum(*p) || *p == '_' || *p == '-') && i < 255) {
                libname[i++] = *p++;
            }

            if (i > 0) {
                if (config.debug) printf("  found library '%s'\n", libname);
                add_dependency(list, libname, DEP_TYPE_LIBRARY);
            }
        }
    }

    fclose(f);
}

// Recursively scan directory for source files and Makefiles
static void scan_directory(const char *path, dependency_list_t *list) {
    if (config.debug)
        printf("ddn:scan_directory(): scanning '%s'\n", path);

    struct stat st;
    if (stat(path, &st) != 0) return;
    if (config.debug)
        printf("ddn:scan_directory(): stat() ok\n");

    if (S_ISREG(st.st_mode)) {
        // Single file
        const char *ext = strrchr(path, '.');
        if (ext) {
            if (config.debug) printf("ddn:scan_directory(): got ext '%s'\n", ext);
            if (strcmp(ext, ".c") == 0 || strcmp(ext, ".h") == 0 ||
                strcmp(ext, ".cpp") == 0 || strcmp(ext, ".cc") == 0 ||
                strcmp(ext, ".cxx") == 0 || strcmp(ext, ".hpp") == 0) {
                parse_c_file(path, list);
            }
        }

        const char *filename = strrchr(path, '/');
        filename = filename ? filename + 1 : path;
        if (strcmp(filename, "Makefile") == 0 || strcmp(filename, "makefile") == 0) {
            parse_makefile(path, list);
        }
        return;
    }

    if (!S_ISDIR(st.st_mode)) return;

    DIR *dir = opendir(path);
    if (!dir) {
        if (config.debug)
            printf("ddn:scan_directory(): (subdir) opendir() failed: %s\n", strerror(errno));

        return;
    }
    if (config.debug) printf("ddn:scan_directory(): (subdir) opendir() ok\n");

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/%s", path, entry->d_name);

        struct stat entry_st;
        if (stat(filepath, &entry_st) != 0) continue;
        if (config.debug) printf("ddn:scan_directory(): (subdir) stat() ok\n");

        if (S_ISDIR(entry_st.st_mode)) {
            scan_directory(filepath, list);
        } else if (S_ISREG(entry_st.st_mode)) {
            if (config.debug) printf("ddn:scan_directory(): (subdir) regular file found\n");
            const char *ext = strrchr(entry->d_name, '.');
            if (ext) {
                if (config.debug) printf("ddn:scan_directory(): (subdir) ext '%s'\n", ext);
                if (strcmp(ext, ".c") == 0 || strcmp(ext, ".h") == 0 ||
                    strcmp(ext, ".cpp") == 0 || strcmp(ext, ".cc") == 0 ||
                    strcmp(ext, ".cxx") == 0 || strcmp(ext, ".hpp") == 0) {
                    parse_c_file(filepath, list);
                }
            }

            if (strcmp(entry->d_name, "Makefile") == 0 || strcmp(entry->d_name, "makefile") == 0) {
                parse_makefile(filepath, list);
            }
        }
    }

    closedir(dir);
}

dependency_list_t* parse_dependencies(const char *path) {
    if (config.debug)
        printf("ddn:parse_dependencies(): parsing '%s'\n", path);
    
    dependency_list_t *list = create_dependency_list();
    if (!list) {
        if (config.debug)
            printf("ddn:parse_dependencies(): create_dependency_list() returned NULL!\n");
        
        return NULL;
    }

    scan_directory(path, list);

    return list;
}
