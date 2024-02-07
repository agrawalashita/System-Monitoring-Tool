/**
 * Tony Givargis
 * Copyright (C), 2023
 * University of California, Irvine
 *
 * CS 238P - Operating Systems
 * main.c
 */

#include <signal.h>
#include "system.h"

/**
 * Needs:
 *   signal()
 */
#define MAX_NAME_LENGTH 20

const char *PROC_STAT = "/proc/stat";
const char *PROC_MEMINFO = "/proc/meminfo";
const char *PROC_NET_DEV = "/proc/net/dev";
const char *PROC_DISK_STATS = "/proc/diskstats";

struct NetworkInterface {
    char name[20];
    unsigned long packetsSent;
    unsigned long packetsReceived;
};

struct BlockDevice {
    char name[MAX_NAME_LENGTH];
    unsigned long blocksRead;
    unsigned long blocksWritten;
};

static volatile int done;

static void
_signal_(int signum)
{
	assert( SIGINT == signum );

	done = 1;
}

double
cpu_util()
{
	static unsigned sum_, vector_[7];
	unsigned sum, vector[7];
	const char *p;
	double util;
	uint64_t i;
    FILE *file;
    char line[1024];

	/*
	  user
	  nice
	  system
	  idle
	  iowait
	  irq
	  softirq
	*/

    if (!(file = fopen(PROC_STAT, "r"))) {
        TRACE("fopen()");
        return -1;
    }
    if (fgets(line, sizeof (line), file) == NULL) {
        TRACE("fgets while reading CPU\n");
        exit(1);
    }

	if (!(p = strstr(line, " ")) ||
	    (7 != sscanf(p,
			 "%u %u %u %u %u %u %u",
			 &vector[0],
			 &vector[1],
			 &vector[2],
			 &vector[3],
			 &vector[4],
			 &vector[5],
			 &vector[6]))) {
		return 0;
	}
	sum = 0.0;
	for (i=0; i<ARRAY_SIZE(vector); ++i) {
		sum += vector[i];
	}
	util = (1.0 - (vector[3] - vector_[3]) / (double)(sum - sum_)) * 100.0;
	sum_ = sum;
	for (i=0; i<ARRAY_SIZE(vector); ++i) {
		vector_[i] = vector[i];
	}

    fclose(file);

	return util;
}

char* mem_info() {
    char line[1024];
    char *mem_info_buffer = NULL;
    size_t bufferSize = 0;
    FILE *file;

    if (!(file = fopen(PROC_MEMINFO, "r"))) {
        perror("fopen");
        TRACE("mem info");
        return NULL;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        size_t lineLength = strlen(line);
        mem_info_buffer = realloc(mem_info_buffer, bufferSize + lineLength + 1); 

        if (mem_info_buffer == NULL) {
            perror("Error allocating memory");
            TRACE("mem info");
            fclose(file);
            return NULL; 
        }

        strcpy(mem_info_buffer + bufferSize, line);
        bufferSize += lineLength;
    }

    fclose(file);

    return mem_info_buffer;
}

double mem_util()
{
    unsigned long total, free_memory, buffers, cached;
    const char *p;
	double util;
    char *s = NULL;

    s = mem_info();

    p = strstr(s, "MemTotal:");
    if (!p) return 0;
    sscanf(p, "MemTotal:        %lu kB", &total);

    p = strstr(s, "MemFree:");
    if (!p) return 0;
    sscanf(p, "MemFree:          %lu kB", &free_memory);

    p = strstr(s, "Buffers:");
    if (!p) return 0;
    sscanf(p, "Buffers:            %lu kB", &buffers);

    p = strstr(s, "Cached:");
    if (!p) return 0;
    sscanf(p, "Cached:           %lu kB", &cached);

    FREE(s);

    util = 100.0 * (1.0 - (((double)free_memory + buffers + cached) / ((double)total)));

    return util;
}

void read_network_stats(struct NetworkInterface *interface)
{
    char line[256];
    FILE *file = fopen(PROC_NET_DEV, "r");
    int skip_lines = 3, skip_counter = 0;
    
    if (file == NULL) {
        TRACE("Error opening /proc/net/dev");
        exit(1);
    }

    while (skip_counter < skip_lines) {
        if (fgets(line, sizeof(line), file) == NULL) {
            TRACE("read file for net stats");
            exit(1);
        }
        skip_counter++;
    }
    
    if (fgets(line, sizeof(line), file) != NULL) {
        sscanf(line, "%s%*c%*u%lu%*u%*u%*u%*u%*u%*u%*u%lu", interface->name, &interface->packetsReceived, &interface->packetsSent);
    }

    fclose(file);
}

void read_block_stats(struct BlockDevice *device) {
    char line[256];
    FILE *file = fopen(PROC_DISK_STATS, "r");
    int skip_devices = 13, skip_device_counter = 0;

    if (file == NULL) {
        perror("Error opening /proc/diskstats");
        exit(1);
    }

    while (skip_device_counter < skip_devices) {
        if (fgets(line, sizeof(line), file) == NULL) {
            TRACE("read file for net stats");
            exit(1);
        }
        skip_device_counter++;
    }
    
    if (fgets(line, sizeof(line), file) != NULL) {
        sscanf(line, "%*u %*u %s %lu %*u %*u %*u %lu", device->name, &device->blocksRead, &device->blocksWritten);
    }

    fclose(file);
}

int
main(int argc, char *argv[])
{
    struct NetworkInterface *interface;
    struct BlockDevice *device;

	UNUSED(argc);
	UNUSED(argv);

	if (SIG_ERR == signal(SIGINT, _signal_)) {
		TRACE("signal()");
		return -1;
	}

    if ((interface = malloc(sizeof(struct NetworkInterface))) == NULL) {
        TRACE("malloc network interface");
        exit(1);
    }

    if ((device = malloc(sizeof(struct BlockDevice))) == NULL) {
        TRACE("malloc block device");
        exit(1);
    }

	while (!done) {
        read_network_stats(interface);
        read_block_stats(device);

        printf("\rCPU Util: %2.1f%% Memory Util: %2.1f%% Network Packets Received: %ld Network Packets Sent: %ld Device Blocks Read: %ld Device Blocks Written: %ld", cpu_util(), mem_util(), interface->packetsReceived, interface->packetsSent, device->blocksRead, device->blocksWritten);
        fflush(stdout);

		us_sleep(500000);
	}
	printf("\rDone!   \n");
	return 0;
}