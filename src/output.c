#include <stdio.h>
#include <string.h>

#include "output.h"
#include "distro.h"

void generate_install_commands(distro_packages_t *results, int count) {
    if (!results || count <= 0) {
        printf("No packages found.\n");
        return;
    }

    printf("## Dependency Installation Commands\n\n");

    for (int i = 0; i < count; i++) {
        const char *distro_name = results[i].distro_name;
        package_list_t *packages = results[i].packages;

        if (!packages || packages->count == 0) {
            printf("### %s\n", distro_name);
            printf("No packages found or VM not configured.\n\n");
            continue;
        }

        // Get distro info for install command
        const distro_info_t *distro = get_distro_by_name(distro_name);
        if (!distro) continue;

        printf("### %s\n", distro_name);
        printf("```bash\n%s", distro->install_command);

        // Print all packages
        for (int j = 0; j < packages->count; j++) {
            printf(" %s", packages->items[j].name);
        }

        printf("\n```\n\n");
    }
}
