#include "engine.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 256

int search_count(char *filename, char *target) {

    size_t target_len = strlen(target);

	if (filename == NULL || target == NULL || target_len == 0) {
		return 0;
	}

	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		return 0;
	}

	char line[MAX_LINE_LENGTH] = {0};
	
	int count = 0;

	while (fgets(line, sizeof(line), file) != NULL) {
        char *match = line;
        match[strcspn(match, "\t\r\n")] = '\0';
        while ((match = strstr(match, target)) != NULL) {
            count++;
            match += target_len;
        }
	}

	fclose(file);
	return count;
}

struct count_result search_instance(char *filename,char *target){

    size_t target_len = strlen(target);    
    struct count_result result = {0, NULL};
    result.count = 0;
    result.instances = NULL;



	if (filename == NULL || target == NULL || target_len == 0) {
		return result;
	}

	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		return result;
	}

	char line[MAX_LINE_LENGTH] = {0};

	while (fgets(line, sizeof(line), file) != NULL) {
        char *match = line;
        match[strcspn(match, "\t\r\n")] = '\0';
        while ((match = strstr(match, target)) != NULL) {
            if (result.instances == NULL) {
                result.instances = malloc(sizeof(char *));
                if (result.instances == NULL) {
                    fclose(file);
                    return result;
                }
            } else {
                char **new_instances = realloc(result.instances, (result.count + 1) * sizeof(char *));
                if (new_instances == NULL) {
                    fclose(file);
                    return result;
                }
                result.instances = new_instances;
            }
            result.count++;
            result.instances[result.count - 1] = strdup(line);
            match += target_len;
        }        
    }
	fclose(file);
	return result;
}