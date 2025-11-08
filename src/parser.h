#ifndef PARSER_H
#define PARSER_H

typedef enum {
    DEP_TYPE_HEADER,    // From #include
    DEP_TYPE_LIBRARY    // From -l flag
} dependency_type_t;

typedef struct {
    char *name;
    dependency_type_t type;
} dependency_t;

typedef struct {
    dependency_t *items;
    int count;
    int capacity;
} dependency_list_t;

// Parse dependencies from source directory
dependency_list_t* parse_dependencies(const char *path);

// Add a dependency to the list
void add_dependency(dependency_list_t *list, const char *name, dependency_type_t type);

// Free dependency list
void free_dependency_list(dependency_list_t *list);

#endif // PARSER_H
