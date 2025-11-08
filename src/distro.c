#include <string.h>
#include <strings.h>
#include "distro.h"

static const distro_info_t distros[] = {
    {"debian", DISTRO_DEBIAN, "apt", "apt-get install -y"},
    {"ubuntu", DISTRO_UBUNTU, "apt", "apt-get install -y"},
    {"arch", DISTRO_ARCH, "pacman", "pacman -S --noconfirm"},
    {"alpine", DISTRO_ALPINE, "apk", "apk add"},
    {"gentoo", DISTRO_GENTOO, "emerge", "emerge"},
    {"opensuse", DISTRO_OPENSUSE, "zypper", "zypper install -y"}
};

int get_distro_count(void) {
    return sizeof(distros) / sizeof(distros[0]);
}

const char* get_distro_name(int index) {
    if (index < 0 || index >= get_distro_count()) {
        return NULL;
    }
    return distros[index].name;
}

const distro_info_t* get_distro_by_name(const char *name) {
    if (!name) return NULL;

    for (int i = 0; i < get_distro_count(); i++) {
        if (strcasecmp(distros[i].name, name) == 0) {
            return &distros[i];
        }
    }

    return NULL;
}
