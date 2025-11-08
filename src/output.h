#ifndef OUTPUT_H
#define OUTPUT_H

#include "vm_query.h"

typedef struct {
    const char *distro_name;
    package_list_t *packages;
} distro_packages_t;

// Generate install commands for all distros
void generate_install_commands(distro_packages_t *results, int count);

#endif // OUTPUT_H
