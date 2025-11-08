#ifndef DISTRO_H
#define DISTRO_H

typedef enum {
    DISTRO_DEBIAN,
    DISTRO_UBUNTU,
    DISTRO_ARCH,
    DISTRO_ALPINE,
    DISTRO_GENTOO,
    DISTRO_OPENSUSE
} distro_type_t;

typedef struct {
    const char *name;
    distro_type_t type;
    const char *package_manager;
    const char *install_command;
} distro_info_t;

// Get number of supported distros
int get_distro_count(void);

// Get distro name by index
const char* get_distro_name(int index);

// Get distro info by name
const distro_info_t* get_distro_by_name(const char *name);

#endif // DISTRO_H
