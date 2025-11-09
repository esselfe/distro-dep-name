#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ddn test
#include <jansson.h>

#include "ddn_config.h"
#include "vm_query.h"
#include "distro.h"

#define INITIAL_CAPACITY 32
#define MAX_CMD_LEN 2048
#define MAX_OUTPUT_LEN 8192

static package_list_t* create_package_list(void) {
    package_list_t *list = malloc(sizeof(package_list_t));
    if (!list) return NULL;

    list->items = malloc(INITIAL_CAPACITY * sizeof(package_t));
    if (!list->items) {
        free(list);
        return NULL;
    }

    list->count = 0;
    list->capacity = INITIAL_CAPACITY;
    return list;
}

static void add_package(package_list_t *list, const char *name, const char *version) {
    if (!list || !name) return;

    // Check for duplicates
    for (int i = 0; i < list->count; i++) {
        if (strcmp(list->items[i].name, name) == 0) {
            return; // Already exists
        }
    }

    // Expand capacity if needed
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->items = realloc(list->items, list->capacity * sizeof(package_t));
        if (!list->items) return;
    }

    list->items[list->count].name = strdup(name);
    list->items[list->count].version = version ? strdup(version) : NULL;
    list->count++;
}

void free_package_list(package_list_t *list) {
    if (!list) return;

    for (int i = 0; i < list->count; i++) {
        free(list->items[i].name);
        if (list->items[i].version) {
            free(list->items[i].version);
        }
    }
    free(list->items);
    free(list);
}

// Execute command via SSH and return output
static char* execute_ssh_command(const char *host, const char *command) {
    if (config.debug)
        printf("ddn:execute_ssh_command(): (subdir) running '%s'\n", command);
    
    char ssh_cmd[MAX_CMD_LEN + 1024];
    snprintf(ssh_cmd, sizeof(ssh_cmd), "ssh %s -- %s 2>/dev/null",
             host, command);

    FILE *pipe = popen(ssh_cmd, "r");
    if (!pipe) return NULL;

    char *output = malloc(MAX_OUTPUT_LEN);
    if (!output) {
        pclose(pipe);
        return NULL;
    }

    size_t len = fread(output, 1, MAX_OUTPUT_LEN - 1, pipe);
    output[len] = '\0';

    pclose(pipe);
    return output;
}

// Get VM hostname for a distro from environment variable
static char* get_vm_host(const char *distro_name) {
    char env_var[128];
    snprintf(env_var, sizeof(env_var), "DISTRO_VM_%s", distro_name);

    char *host = getenv(env_var);
    return host ? strdup(host) : NULL;
}

// Map header file to library name (simple heuristic)
static const char* header_to_library(const char *header) {
    // Common mappings
    if (strstr(header, "ssl.h")) return "ssl";
    if (strstr(header, "crypto.h")) return "ssl";
    if (strstr(header, "curl.h")) return "curl";
    if (strstr(header, "pthread.h")) return "c";
    if (strstr(header, "z.h")) return "z";
    if (strstr(header, "sqlite3.h")) return "sqlite3";
    if (strstr(header, "mysql.h")) return "mysqlclient";
    if (strstr(header, "postgres.h") || strstr(header, "pq")) return "pq";
    if (strstr(header, "pcre.h")) return "pcre";
    if (strstr(header, "xml2.h")) return "xml2";
    if (strstr(header, "json.h")) return "json-c";
    if (strstr(header, "readline.h")) return "readline";
    if (strstr(header, "ncurses.h")) return "ncurses";
    if (strstr(header, "pcap.h")) return "pcap";
    if (strstr(header, "jansson.h")) return "jansson";
    if (strstr(header, "stdio.h")) return "c";
    if (strstr(header, "stdlib.h")) return "c";
    if (strstr(header, "unistd.h")) return "c";

    return NULL;
}

// Query package for a specific dependency
static void query_dependency(const char *host, const distro_info_t *distro,
                            dependency_t *dep, package_list_t *packages) {
    char command[MAX_CMD_LEN];
    const char *lib_name = NULL;

    if (dep->type == DEP_TYPE_HEADER) {
        lib_name = header_to_library(dep->name);
        if (!lib_name) {
            // Try to extract library name from header path
            char *last_slash = strrchr(dep->name, '/');
            char *base_name = last_slash ? last_slash + 1 : (char*)dep->name;

            // Remove .h extension
            char *dot = strrchr(base_name, '.');
            if (dot) {
                size_t len = dot - base_name;
                char *tmp = malloc(len + 1);
                strncpy(tmp, base_name, len);
                tmp[len] = '\0';
                lib_name = tmp;
            }
        }
    } else {
        lib_name = dep->name;
    }

    if (!lib_name) return;

    // Build query command based on distro
    switch (distro->type) {
        case DISTRO_DEBIAN:
        case DISTRO_UBUNTU:
            // Try pkg-config first, then search for lib*-dev packages
            snprintf(command, sizeof(command),
                    "apt-cache search --names-only 'lib%s.*-dev' | head -5 | cut -d' ' -f1",
                    lib_name);
            break;

        case DISTRO_ARCH:
            snprintf(command, sizeof(command),
                    "pacman -Ss '^%s$' | grep -v '^ ' | cut -d'/' -f2 | cut -d' ' -f1",
                    lib_name);
            break;

        case DISTRO_ALPINE:
            snprintf(command, sizeof(command),
                    "apk search -v %s-dev | cut -d'-' -f1-2",
                    lib_name);
            break;

        case DISTRO_GENTOO:
            snprintf(command, sizeof(command),
                    "emerge -s '^%s$' | grep -A1 'Latest version' | tail -1 | awk '{print $1}'",
                    lib_name);
            break;

        case DISTRO_OPENSUSE:
            snprintf(command, sizeof(command),
                    "zypper search --match-substrings -t package 'lib%s' | awk '/-devel/ { print $2 }'",
                    lib_name);
            break;

        default:
            return;
    }

    char *output = execute_ssh_command(host, command);
    if (output) {
        // Parse output line by line
        char *line = strtok(output, "\n");
        while (line) {
            // Trim whitespace
            while (*line && (*line == ' ' || *line == '\t')) line++;
            if (*line) {
                add_package(packages, line, NULL);
            }
            line = strtok(NULL, "\n");
        }
        free(output);
    }
}

package_list_t* query_distro_packages(const char *distro_name, dependency_list_t *deps) {
    if (!distro_name || !deps) return NULL;

    printf("Querying %s packages...\n", distro_name);

    // Get VM host
    char *host = get_vm_host(distro_name);
    if (!host) {
        fprintf(stderr, "Warning: No VM configured for %s (set DISTRO_VM_%s environment variable)\n",
                distro_name, distro_name);
        return create_package_list();
    }

    // Get distro info
    const distro_info_t *distro = get_distro_by_name(distro_name);
    if (!distro) {
        fprintf(stderr, "Error: Unknown distro %s\n", distro_name);
        free(host);
        return create_package_list();
    }

    package_list_t *packages = create_package_list();

    // Query each dependency
    for (int i = 0; i < deps->count; i++) {
        query_dependency(host, distro, &deps->items[i], packages);
    }

    free(host);
    return packages;
}
