#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>

#define BUF_SIZE 256

// Structure to store process information
typedef struct {
    int pid;
    char name[BUF_SIZE];
    double cpu_usage;
    unsigned long memory_usage;  // Memory usage in kilobytes
    int num_threads;
} ProcessInfo;

void calculate_process_info(int pid, double *cpu_usage, unsigned long *memory_usage, char *name, int *num_threads) {
char stat_path[BUF_SIZE];
   char status_path[BUF_SIZE];
    long unsigned utime, stime;
    long int cutime,cstime, starttime;
    sprintf(stat_path, "/proc/%d/stat", pid);

    FILE *stat_file = fopen(stat_path, "r");
    if (stat_file == NULL) {
        perror("Error opening /proc/[PID]/stat");
        exit(EXIT_FAILURE);
    }

    // Read required values from /proc/[PID]/stat

fscanf(stat_file, "%*d %s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu"
                "%lu %ld %ld %*d %*d %*d %*d %ld %*lu %*ld",
                name,&utime, &stime,&cutime,&cstime,&starttime);

    printf ("Utime is %lu\n", utime);
    printf ("stime is %lu\n", stime);
    printf ("cuttime is %ld\n", cutime);
    printf ("cstime is %lu\n", cstime);
    printf ("starttime is %lu\n", starttime);
    fclose(stat_file);

    // Open /proc/uptime file
    FILE *uptime_file = fopen("/proc/uptime", "r");
    if (uptime_file == NULL) {
        perror("Error opening /proc/uptime");
        exit(EXIT_FAILURE);
    }

    // Read uptime value
    double uptime;
    fscanf(uptime_file, "%lf", &uptime);

    fclose(uptime_file);

    // Get Hertz value
    long hertz = sysconf(_SC_CLK_TCK);

    // Calculate total time and seconds
    long total_time = utime + stime;
    //total_time = total_time + cutime + cstime;
    total_time = total_time + cstime;

    double seconds = uptime - ((double)starttime / hertz);

    // Calculate CPU usage percentage
    *cpu_usage = 100 * ((double)total_time / hertz) / seconds;

    sprintf(status_path, "/proc/%d/status", pid);

    FILE *status_file = fopen(status_path, "r");
    if (status_file == NULL) {
        perror("Error opening /proc/[PID]/status");
        exit(EXIT_FAILURE);
    }

    char line[BUF_SIZE];
    while (fgets(line, sizeof(line), status_file) != NULL) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line, "VmRSS: %lu", memory_usage);
	}else if (strncmp(line, "Threads:", 8) == 0) {
            sscanf(line, "Threads: %d", num_threads);
            
        }
    }




    fclose(status_file);
}

// Comparison function for sorting processes based on CPU usage
int compare_processes_cpu(const void *a, const void *b) {
    double cpu_usage_a = ((ProcessInfo *)a)->cpu_usage;
    double cpu_usage_b = ((ProcessInfo *)b)->cpu_usage;

    if (cpu_usage_a < cpu_usage_b) return 1;
    if (cpu_usage_a > cpu_usage_b) return -1;
    return 0;
}



int main() {
    DIR *proc_dir;
    struct dirent *entry;

    // Open the /proc directory
    proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        perror("Error opening /proc directory");
        exit(EXIT_FAILURE);
    }

    // Create an array to store process information
    ProcessInfo processes[1000];  // Adjust the size as needed

    int process_count = 0;

    // Loop through all entries in /proc
    while ((entry = readdir(proc_dir)) != NULL && process_count < 1000) {
        // Check if the entry is a directory and represents a process ID
        if (entry->d_type == DT_DIR && atoi(entry->d_name) != 0) {
            int pid = atoi(entry->d_name);

            // Calculate CPU usage, memory usage, and process name for the current process
            double cpu_usage;
            unsigned long memory_usage;
            char name[BUF_SIZE];
	    int num_threads;
            calculate_process_info(pid, &cpu_usage, &memory_usage, name,&num_threads);

            // Store process information in the array
            processes[process_count].pid = pid;
            strcpy(processes[process_count].name, name);
            processes[process_count].cpu_usage = cpu_usage;
            processes[process_count].memory_usage = memory_usage;
	    processes[process_count].num_threads = num_threads;
            process_count++;
        }
    }

    // Close the /proc directory
    closedir(proc_dir);

    // Sort the array based on CPU usage
    qsort(processes, process_count, sizeof(ProcessInfo), compare_processes_cpu);

    // Print the sorted results including CPU, memory usage, and process name
    for (int i = 0; i < 10; i++) {
        printf("PID: %d, Name: %s, CPU usage: %.2f%%, Memory usage: %lu KB, Threads: %d\n",
               processes[i].pid, processes[i].name, processes[i].cpu_usage, processes[i].memory_usage,processes[i].num_threads);
    }

    return 0;
}



