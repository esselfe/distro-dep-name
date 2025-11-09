#ifndef DDN_CONFIG_H
#define DDN_CONFIG_H 1

typedef struct {
    int debug;
    char **distros;
    int distro_count;
    char *source_path;
    int all_distros;
} config_t;

// From main.c
extern config_t config;

#endif // DDN_CONFIG_H
