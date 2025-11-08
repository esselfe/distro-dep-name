#ifndef VM_QUERY_H
#define VM_QUERY_H

#include "parser.h"

typedef struct {
    char *name;
    char *version;
} package_t;

typedef struct {
    package_t *items;
    int count;
    int capacity;
} package_list_t;

// Query a distro's VMs for packages that provide the dependencies
package_list_t* query_distro_packages(const char *distro_name, dependency_list_t *deps);

// Free package list
void free_package_list(package_list_t *list);

#endif // VM_QUERY_H
