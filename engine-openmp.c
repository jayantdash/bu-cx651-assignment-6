#include "engine.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <omp.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 256
#define NUM_THREADS 100

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
	long position = ftell(file);
	if (position <= 0) {
		return;
	}

	if (fseek(file, position - 1, SEEK_SET) != 0) {
		return;
	}

	if (fgetc(file) == '\n') {
		fseek(file, position, SEEK_SET);
		return;
	}

	int character;
	while ((character = fgetc(file)) != '\n' && character != EOF) {
	}
}

struct count_result instance_worker(struct worker_args *args) {
	struct count_result result = {0, NULL};
	size_t target_len = strlen(args->target);
	FILE *file = fopen(args->filename, "r");
	if (file == NULL || fseek(file, args->start, SEEK_SET) != 0) {
		if (file != NULL) {
			fclose(file);
		}
		return result;
	}

	checkAndMoveToNextLine(file);

	char line[MAX_LINE_LENGTH] = {0};
	while (ftell(file) < args->end && fgets(line, sizeof(line), file) != NULL) {
		char *match = line;
		match[strcspn(match, "\t\r\n")] = '\0';
		while ((match = strstr(match, args->target)) != NULL) {
			char *instance = strdup(line);
			if (instance == NULL) {
				for (int i = 0; i < result.count; i++) {
					free(result.instances[i]);
				}
				free(result.instances);
				result = (struct count_result){0, NULL};
				fclose(file);
				return result;
			}

			char **instances = realloc(result.instances,
									   (result.count + 1) * sizeof(char *));
			if (instances == NULL) {
				free(instance);
				for (int i = 0; i < result.count; i++) {
					free(result.instances[i]);
				}
				free(result.instances);
				result = (struct count_result){0, NULL};
				fclose(file);
				return result;
			}

			result.instances = instances;
			result.instances[result.count++] = instance;
			match += target_len;
		}
	}

	fclose(file);
	return result;
}

int count_worker(struct worker_args *args) {
	size_t target_len = strlen(args->target);
	FILE *file = fopen(args->filename, "r");
	if (file == NULL || fseek(file, args->start, SEEK_SET) != 0) {
		if (file != NULL) {
			fclose(file);
		}
		return 0;
	}

	checkAndMoveToNextLine(file);

	char line[MAX_LINE_LENGTH] = {0};
	int count = 0;
	while (ftell(file) < args->end && fgets(line, sizeof(line), file) != NULL) {
		char *match = line;
		match[strcspn(match, "\t\r\n")] = '\0';
		while ((match = strstr(match, args->target)) != NULL) {
			count++;
			match += target_len;
		}
	}

	fclose(file);
	return count;
}

int search_count(char *filename, char *target) {
	if (filename == NULL || target == NULL || target[0] == '\0') {
		return 0;
	}

	struct stat file_info;
	if (stat(filename, &file_info) != 0 || file_info.st_size <= 0) {
		return 0;
	}

	long file_size = (long)file_info.st_size;
	struct worker_args args[NUM_THREADS];
	int partial_counts[NUM_THREADS] = {0};

	for (int i = 0; i < NUM_THREADS; i++) {
		args[i].filename = filename;
		args[i].target = target;
		args[i].start = file_size * i / NUM_THREADS;
		args[i].end = file_size * (i + 1) / NUM_THREADS;
	}

	#pragma omp parallel for num_threads(NUM_THREADS)
	for (int i = 0; i < NUM_THREADS; i++) {
		partial_counts[i] = count_worker(&args[i]);
	}

	int total = 0;
	for (int i = 0; i < NUM_THREADS; i++) {
		total += partial_counts[i];
	}
	return total;
}

struct count_result search_instance(char *filename,char *target){
	struct count_result result = {0, NULL};
	if (filename == NULL || target == NULL || target[0] == '\0') {
		return result;
	}

	struct stat file_info;
	if (stat(filename, &file_info) != 0 || file_info.st_size <= 0) {
		return result;
	}

	long file_size = (long)file_info.st_size;
	struct worker_args args[NUM_THREADS];
	struct count_result partial_results[NUM_THREADS] = {{0, NULL}};

	for (int i = 0; i < NUM_THREADS; i++) {
		args[i].filename = filename;
		args[i].target = target;
		args[i].start = file_size * i / NUM_THREADS;
		args[i].end = file_size * (i + 1) / NUM_THREADS;
	}

	#pragma omp parallel for num_threads(NUM_THREADS)
	for (int i = 0; i < NUM_THREADS; i++) {
		partial_results[i] = instance_worker(&args[i]);
	}

	for (int i = 0; i < NUM_THREADS; i++) {
		for (int j = 0; j < partial_results[i].count; j++) {
			char **instances = realloc(result.instances,
									   (result.count + 1) * sizeof(char *));
			if (instances == NULL) {
				for (int k = 0; k < result.count; k++) {
					free(result.instances[k]);
				}
				free(result.instances);
				result = (struct count_result){0, NULL};
				for (int k = i; k < NUM_THREADS; k++) {
					for (int n = 0; n < partial_results[k].count; n++) {
						free(partial_results[k].instances[n]);
					}
					free(partial_results[k].instances);
				}
				return result;
			}
			result.instances = instances;
			result.instances[result.count++] = partial_results[i].instances[j];
			partial_results[i].instances[j] = NULL;
		}
		free(partial_results[i].instances);
	}

	return result;
}