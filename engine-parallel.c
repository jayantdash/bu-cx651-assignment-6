#include "engine.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/stat.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 256
#define NUM_THREADS 5

struct worker_args {
    char *filename;
    char *target;

    long start; //start of the chunk to process
    long end; //end of the chunk to process
};

// This function checks if the current file position is at the beginning of a line.
// If not, it moves the file pointer to the beginning of the next line.
// so we can have the complete line for processing and add it to our results.
void checkAndMoveToNextLine(FILE *file) {
    if (file == NULL) {
        return;
    }
    int c;
    while ((c = fgetc(file)) != '\n' && c != EOF) {
    }

    long pos = ftell(file);

    // If we are at the beginning of the file, no need to move to the next line
    if (pos == 0) {
        return;
    }

    // check the character immediately before the current position
    if (fseek(file, -1L, SEEK_CUR) != 0) {
        return;
    }

    int c = fgetc(file);
    if (c != '\n' && c != EOF) {
        // Move to the beginning of the next line
        while ((c = fgetc(file)) != '\n' && c != EOF) {}
    } else {
        // We are already at the beginning of a line, no need to move
        fseek(file, pos, SEEK_SET);
    }
}

void *instance_worker(void *arg) {
    struct worker_args *args = arg;
    
    struct count_result result = {0, NULL};
    
    size_t target_len = strlen(args->target);

	if (args->filename == NULL || args->target == NULL || target_len == 0) {
		return NULL;
	}

	FILE *file = fopen(args->filename, "r");
	if (file == NULL) {
		return NULL;
	}

    // Move the file pointer to the start of the assigned chunk
    fseek(file, args->start, SEEK_SET);
    
    // Ensure we start processing from the beginning of the next line if we are not at the start of a line
    checkAndMoveToNextLine(file);

	char line[MAX_LINE_LENGTH] = {0};

	while (ftell(file) < args->end && fgets(line, sizeof(line), file) != NULL) {
        char *match = line;
        match[strcspn(match, "\t\r\n")] = '\0';
        while ((match = strstr(match, args->target)) != NULL) {
            // Allocate or reallocate memory for storing instances of matches
            if (result.instances == NULL) {
                result.instances = malloc(sizeof(char *));
                if (result.instances == NULL) {
                    fclose(file);
                    return NULL;
                }
            } else {
                // Reallocate memory to accommodate the new instance
                char **new_instances = realloc(result.instances, (result.count + 1) * sizeof(char *));
                if (new_instances == NULL) {
                    fclose(file);
                    return NULL;
                }
                result.instances = new_instances;
            }
            result.count++;
            result.instances[result.count - 1] = strdup(match);
            match += target_len;
        }        
    }

    fclose(file);
    return &result;
}

void *count_worker(void *arg) {
    struct worker_args *args = arg;

    int count = 0;

    size_t target_len = strlen(args->target);

	if (args->filename == NULL || args->target == NULL || target_len == 0) {
		return NULL;
	}

	FILE *file = fopen(args->filename, "r");
	if (file == NULL) {
		return NULL;
	}

    // Move the file pointer to the start of the assigned chunk
    fseek(file, args->start, SEEK_SET);
    
    // Ensure we start processing from the beginning of the next line if we are not at the start of a line
    checkAndMoveToNextLine(file);

	char line[MAX_LINE_LENGTH] = {0};

    while (ftell(file) < args->end && fgets(line, sizeof(line), file) != NULL) {
        char *match = line;
        match[strcspn(match, "\t\r\n")] = '\0';
        while ((match = strstr(match, args->target)) != NULL) {
            count++;
            match += target_len;
        }
    }

    fclose(file);
    return (void *)(intptr_t)count;
}


int search_count(char *filename, char *target) {
 
}

struct count_result search_instance(char *filename,char *target){
    
}