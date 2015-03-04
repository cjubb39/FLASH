/*
 * flash-mmap.c
 * Sowftware and hardware-accelerated FLASH.
 *
 */
#include <sys/mman.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <stdbool.h>
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <math.h>

#include "flash.h"
#include "wami_params.h"
#include "wami_utils.h"
#include "wami_flash.h"

static const char *devname = "/dev/flash.0";

#define PRIME 7

///////////////////////////////////////////////////////////////////////////////
//#define NO_HARDWARE
//#define NO_SOFTWARE
///////////////////////////////////////////////////////////////////////////////

static void print_array(rgb_pixel arr[WAMI_FLASH_IMG_NUM_ROWS-2*PAD][WAMI_FLASH_IMG_NUM_COLS-2*PAD])
{
	int i, j;
	for (i = 0; i < WAMI_FLASH_IMG_NUM_ROWS-2*PAD; i++) {
		for (j = 0; j < WAMI_FLASH_IMG_NUM_COLS-2*PAD; j++) {
			printf("[%04x,%04x,%04x]", arr[i][j].r, arr[i][j].g, arr[i][j].b);
		}
		printf("\n");
	}
}

static long hash(rgb_pixel arr[WAMI_FLASH_IMG_NUM_ROWS-2*PAD][WAMI_FLASH_IMG_NUM_COLS-2*PAD])
{
	long long int hash = 0;
	int row, col;
	for (row = 0; row < WAMI_FLASH_IMG_NUM_ROWS-2*PAD; row++)
		for (col = 0; col < WAMI_FLASH_IMG_NUM_COLS-2*PAD; col++) {
			hash = PRIME * hash + ((arr[row][col].r * PRIME) + arr[row][col].g) * PRIME + arr[row][col].b;
		}
	return hash;
}

int main(int argc, char *argv[])
{
	unsigned flag;
	void *buf;
	int fd;
	size_t buf_size;
	size_t sample_size;

	struct flash_access desc;
	int rc, memcmp_ret;

	// Load the image
	u16 bayer[WAMI_FLASH_IMG_NUM_ROWS][WAMI_FLASH_IMG_NUM_COLS];
	rgb_pixel software_flash[WAMI_FLASH_IMG_NUM_ROWS-2*PAD][WAMI_FLASH_IMG_NUM_COLS-2*PAD];
	rgb_pixel hardware_flash[WAMI_FLASH_IMG_NUM_ROWS-2*PAD][WAMI_FLASH_IMG_NUM_COLS-2*PAD];
	rgb_pixel golden_flash[WAMI_FLASH_IMG_NUM_ROWS-2*PAD][WAMI_FLASH_IMG_NUM_COLS-2*PAD];

	// Read the image file
	read_image_file((void *) bayer, sizeof(u16), "input.bin", "inout", sizeof(u16) * WAMI_FLASH_IMG_NUM_ROWS * WAMI_FLASH_IMG_NUM_COLS);
	// Read the golden file
	read_image_file((void *) golden_flash, sizeof(rgb_pixel), "golden_output.bin", "inout", sizeof(rgb_pixel) * (WAMI_FLASH_IMG_NUM_ROWS-2*PAD) * (WAMI_FLASH_IMG_NUM_COLS-2*PAD));
	printf("Golden flash hash: %ld\n", hash(golden_flash));
	/* *********************** SOFTWARE ****************************************/
#ifndef NO_SOFTWARE
	printf("Start: FLASH as SOFTWARE.\n");
	wami_flash(software_flash, bayer);
	// print_array(software_flash);
	printf("Software flash hash: %ld\n", hash(software_flash));
	memcmp_ret = memcmp(software_flash, golden_flash, sizeof(golden_flash));
	printf("software_flash and golden_flash memcmp: %d\n", memcmp_ret);
	if (!memcmp_ret)
		printf("Software and Golden match!\n");
#endif
	/* *********************** HARDWARE ****************************************/
#ifndef NO_HARDWARE
	printf("Open: %s\n", devname);
	fd = open(devname, O_RDWR, 0);
	if (fd < 0)
	{
		perror("open");
		exit(1);
	}

	buf_size = FLASH_BUF_SIZE;
	sample_size = FLASH_OUTPUT_SIZE;

	/* main memory */
	buf = mmap(NULL, buf_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (buf == MAP_FAILED)
	{
		perror("mmap");
		exit(1);
	}

	/* load data - copy data from input data structures*/
	// for (i = 0; i < num; i++) {
	memcpy(buf, bayer, sizeof(bayer));
	printf("Size of bayer: %lu\n", sizeof(bayer));
	// 	memcpy(buf + ((2 * i + 1) * sz), coeff + (i * sz), sample_size);
	// }

	// printf("\nStart: FLASH as HARDWARE.\n");
	desc.size = sizeof(software_flash);
	// // desc.num_samples = num;

	// Configure the device and run it
	rc = ioctl(fd, FLASH_IOC_ACCESS, &desc);
	if (rc < 0)
	{
		perror("ioctl");
		exit(1);
	}
	// Retrieve the computation
	memcpy(hardware_flash, ((char *) buf) + FLASH_INPUT_SIZE, sizeof(hardware_flash));
	printf("Hardware flash hash: %ld\n", hash(hardware_flash));

	// Check equality
	memcmp_ret = memcmp(hardware_flash, golden_flash, sizeof(golden_flash));
	printf("hardware_flash and golden_flash memcmp: %d\n", memcmp_ret);

	if (!memcmp_ret)
		printf("Hardware and Golden match!\n");

	if (munmap(buf, buf_size)) {
		perror("munmap");
		exit(1);
	}
#endif
	close(fd);

	return 0;
}
