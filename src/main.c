#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include "ddn_config.h"
#include "parser.h"
#include "vm_query.h"
#include "distro.h"
#include "output.h"

#define VERSION "0.0.5"

static const struct option long_options[] = {
    {"debug", no_argument, 0, 'D'},
    {"distro", required_argument, 0, 'd'},
    {"all", no_argument, 0, 'a'},
    {"list-distros", no_argument, 0, 'l'},
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'V'},
    {0, 0, 0, 0}
};
static const char *short_options = "Dd:alhV";

config_t config;

void print_usage(const char *prog_name) {
    printf("Usage: %s [OPTIONS] <source_path>\n", prog_name);
    printf("\nAnalyze source code and generate distro-specific dependency install commands.\n\n");
    printf("Options:\n");
    printf("  -D, --debug            Show detailed informations for debugging purposes\n");
    printf("  -d, --distro <name>    Specify a distro (can be used multiple times)\n");
    printf("  -a, --all              Query all supported distros (default)\n");
    printf("  -l, --list-distros     List supported distros and exit\n");
    printf("  -h, --help             Show this help message\n");
    printf("  -V, --version          Show version information\n");
    printf("\nSupported distros:\n");
    for (int i = 0; i < get_distro_count(); i++) {
        printf("  - %s\n", get_distro_name(i));
    }
}

void print_version(void) {
    printf("distro-dep-name version %s\nhttps://github.com/esselfe/distro-dep-name/\n", VERSION);
}

int main(int argc, char *argv[]) {
    config.all_distros = 1; // Default to all distros

    int distro_capacity = 10;
    config.distros = malloc(distro_capacity * sizeof(char*));
    if (!config.distros) {
        fprintf(stderr, "ddn:main(): Memory allocation failed\n");
        return ENOMEM;
    }

    int opt;
    while ((opt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
        switch (opt) {
            case 'D':
                config.debug = 1;
                break;
            case 'd':
                if (config.distro_count >= distro_capacity) {
                    distro_capacity *= 2;
                    config.distros = realloc(config.distros, distro_capacity * sizeof(char*));
                    if (!config.distros) {
                        fprintf(stderr, "Memory allocation failed\n");
                        return 1;
                    }
                }
                config.distros[config.distro_count++] = strdup(optarg);
                config.all_distros = 0;
                break;
            case 'a':
                config.all_distros = 1;
                break;
            case 'l':
                printf("Supported distros:\n");
                for (int i = 0; i < get_distro_count(); i++) {
                    printf("  %s\n", get_distro_name(i));
                }
                free(config.distros);
                return 0;
            case 'h':
                print_usage(argv[0]);
                free(config.distros);
                return 0;
            case 'V':
                print_version();
                free(config.distros);
                return 0;
            default:
                print_usage(argv[0]);
                free(config.distros);
                return 1;
        }
    }

    // Get source path
    if (optind >= argc) {
        fprintf(stderr, "Error: source path required\n\n");
        print_usage(argv[0]);
        free(config.distros);
        return 1;
    }
    config.source_path = argv[optind];

    // Parse source code
    if (config.debug)
        printf("Analyzing source code at: %s\n", config.source_path);

    dependency_list_t *deps = parse_dependencies(config.source_path);
    if (!deps) {
        fprintf(stderr, "Failed to parse dependencies\n");
        free(config.distros);
        return 1;
    }

    printf("Found %d dependencies\n\n", deps->count);

    // Query VMs for each distro
    distro_packages_t *results = NULL;
    int result_count = 0;

    if (config.all_distros) {
        result_count = get_distro_count();
        results = malloc(result_count * sizeof(distro_packages_t));
        for (int i = 0; i < result_count; i++) {
            results[i].distro_name = get_distro_name(i);
            results[i].packages = query_distro_packages(get_distro_name(i), deps);
        }
    } else {
        result_count = config.distro_count;
        results = malloc(result_count * sizeof(distro_packages_t));
        for (int i = 0; i < result_count; i++) {
            results[i].distro_name = config.distros[i];
            results[i].packages = query_distro_packages(config.distros[i], deps);
        }
    }

    // Generate output
    generate_install_commands(results, result_count);

    // Cleanup
    free_dependency_list(deps);
    for (int i = 0; i < result_count; i++) {
        free_package_list(results[i].packages);
    }
    free(results);

    for (int i = 0; i < config.distro_count; i++) {
        free(config.distros[i]);
    }
    free(config.distros);

    return 0;
}
