#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>

#define MAX_PATH 1024
#define MAX_STEPS_DOWN 4
#define MAX_STEPS_UP 4
#define MAX_RESULTS 20
#define MAX_EXCLUSIONS 20
#define MAX_VISITED 10000
#define HASH_TABLE_SIZE 10007
#define MAX_LINE 256
#define MAX_TOP_LEVEL_DIRS 5

typedef struct Node {
    char *path;
    struct Node *next;
} Node;

typedef struct {
    Node *buckets[HASH_TABLE_SIZE];
} HashTable;

char* top_level_dirs[MAX_TOP_LEVEL_DIRS] = {NULL};
int top_level_dir_count = 0;
char* exclusions[MAX_EXCLUSIONS] = {NULL};
int exclusion_count = 0;

unsigned long hash(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash % HASH_TABLE_SIZE;
}

void insert_hash(HashTable *table, const char *path) {
    unsigned long index = hash(path);
    Node *new_node = malloc(sizeof(Node));
    new_node->path = strdup(path);
    new_node->next = table->buckets[index];
    table->buckets[index] = new_node;
}

int is_visited(HashTable *table, const char *path) {
    unsigned long index = hash(path);
    Node *current = table->buckets[index];
    while (current) {
        if (strcmp(current->path, path) == 0)
            return 1;
        current = current->next;
    }
    return 0;
}

void free_hash_table(HashTable *table) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        Node *current = table->buckets[i];
        while (current) {
            Node *temp = current;
            current = current->next;
            free(temp->path);
            free(temp);
        }
    }
}

int is_excluded(const char *name) {
    for (int i = 0; i < exclusion_count; i++) {
        if (strstr(name, exclusions[i]) != NULL) {
            return 1;
        }
    }
    return 0;
}

void read_config() {
    char *config_home = getenv("XDG_CONFIG_HOME");
    char config_path[MAX_PATH];
    
    if (config_home) {
        snprintf(config_path, sizeof(config_path), "%s/gt/config", config_home);
    } else {
        snprintf(config_path, sizeof(config_path), "%s/.config/gt/config", getenv("HOME"));
    }

    FILE *file = fopen(config_path, "r");
    if (file == NULL) {
        // If config file doesn't exist, use home directory as default top-level directory
        top_level_dirs[0] = strdup(getenv("HOME"));
        top_level_dir_count = 1;
        exclusions[0] = strdup("node_modules");
        exclusion_count = 1;
        return;
    }

    char line[MAX_LINE];
    char *key, *value, *token;
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;  // Remove newline
        if (line[0] == '#' || line[0] == '\0') continue;  // Skip comments and empty lines
        
        key = strtok(line, "=");
        value = strtok(NULL, "=");
        
        if (key && value) {
            if (strcmp(key, "top_level_dir") == 0 && top_level_dir_count < MAX_TOP_LEVEL_DIRS) {
                top_level_dirs[top_level_dir_count++] = strdup(value);
            } else if (strcmp(key, "exclusions") == 0) {
                token = strtok(value, ",");
                while (token != NULL && exclusion_count < MAX_EXCLUSIONS) {
                    exclusions[exclusion_count++] = strdup(token);
                    token = strtok(NULL, ",");
                }
            }
        }
    }

    fclose(file);

    // If no top-level directory was specified, use home directory as default
    if (top_level_dir_count == 0) {
        top_level_dirs[0] = strdup(getenv("HOME"));
        top_level_dir_count = 1;
    }

    // If no exclusions were specified, add a default one
    if (exclusion_count == 0) {
        exclusions[0] = strdup("node_modules");
        exclusion_count = 1;
    }
}

int is_within_top_level_dirs(const char *path) {
    char real_path[PATH_MAX];
    if (realpath(path, real_path) == NULL) {
        return 0;  // If we can't resolve the real path, assume it's not within top-level dirs
    }

    for (int i = 0; i < top_level_dir_count; i++) {
        if (strncmp(real_path, top_level_dirs[i], strlen(top_level_dirs[i])) == 0) {
            return 1;
        }
    }
    return 0;
}

void search_down(const char *current_path, const char *target, char **results, int *result_count, 
                 HashTable *visited, int depth, int max_depth) {
    DIR *dir;
    struct dirent *entry;
    char path[MAX_PATH];
    struct stat statbuf;

    if (depth > max_depth || *result_count >= MAX_RESULTS) {
        return;
    }

    dir = opendir(current_path);
    if (dir == NULL) {
        return;
    }

    while ((entry = readdir(dir)) != NULL && *result_count < MAX_RESULTS) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || entry->d_name[0] == '.') {
            continue;
        }

        snprintf(path, sizeof(path), "%s/%s", current_path, entry->d_name);

        if (!is_within_top_level_dirs(path)) {
            continue;  // Skip if not within top-level directories
        }

        if (is_visited(visited, path)) {
            continue;
        }

        insert_hash(visited, path);

        if (stat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
            if (!is_excluded(entry->d_name)) {
                if (strstr(entry->d_name, target) != NULL) {
                    results[*result_count] = strdup(path);
                    (*result_count)++;
                }
                search_down(path, target, results, result_count, visited, depth + 1, max_depth);
            }
        }
    }

    closedir(dir);
}

void search_up_and_down(char *current_path, const char *target, char **results, int *result_count,
                        HashTable *visited, int max_up, int max_down) {
    char path[MAX_PATH];
    int up_depth = 0;

    while (up_depth < max_up && *result_count < MAX_RESULTS) {
        if (!is_within_top_level_dirs(current_path)) {
            break;  // Stop ascending if we've gone beyond top-level directories
        }

        char *dir_name = basename(current_path);
        if (!is_excluded(dir_name) && !is_visited(visited, current_path)) {
            insert_hash(visited, current_path);

            if (strstr(dir_name, target) != NULL) {
                results[*result_count] = strdup(current_path);
                (*result_count)++;
            }
            search_down(current_path, target, results, result_count, visited, 0, max_down);
        }

        char *parent = dirname(current_path);
        if (strcmp(parent, current_path) == 0) {
            break;  // Reached the root directory
        }
        strncpy(path, parent, sizeof(path));
        strncpy(current_path, path, MAX_PATH);
        up_depth++;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> <target_directory>\n", argv[0]);
        fprintf(stderr, "Available commands: search, immediate\n");
        return 1;
    }

    char *command = argv[1];
    char *target = argv[2] == NULL ? "" : argv[2];
    char current_path[MAX_PATH];
    char *results[MAX_RESULTS] = {0};
    int result_count = 0;
    HashTable visited = {0};

    read_config();  // Read configuration including top-level directories and exclusions

    if (strcmp(command, "search") != 0 && strcmp(command, "immediate") != 0) {
        fprintf(stderr, "Unknown command: %s\n", command);
        fprintf(stderr, "Available commands: search, immediate\n");
        return 1;
    }

    if (getcwd(current_path, sizeof(current_path)) == NULL) {
        perror("getcwd() error");
        return 1;
    }

    int max_steps_up = strcmp(command, "immediate") == 0 ? 1 : MAX_STEPS_UP;
    int max_steps_down = strcmp(command, "immediate") == 0 ? 1 : MAX_STEPS_DOWN;

    if (strncmp(target, "../", 3) == 0) {
        // Remove "../" prefix for the actual search
        target += 3;
        search_up_and_down(current_path, target, results, &result_count, &visited, max_steps_up, max_steps_down);
    } else {
        search_down(current_path, target, results, &result_count, &visited, 0, max_steps_down);
    }

    for (int i = 0; i < result_count; i++) {
        printf("%s\n", results[i]);
        free(results[i]);
    }

    free_hash_table(&visited);

    // Clean up top_level_dirs and exclusions
    for (int i = 0; i < top_level_dir_count; i++) {
        free(top_level_dirs[i]);
    }
    for (int i = 0; i < exclusion_count; i++) {
        free(exclusions[i]);
    }

    return (result_count > 0) ? 0 : 1;
}