#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>

#include <asm/unistd.h>

#include <sys/ioctl.h>

static long perf_event_open(struct perf_event_attr *hw_event,
	pid_t pid, int cpu, int group_fd, unsigned long flags)
{
	int ret;
	ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,	group_fd, flags);
	return ret;
}

int main(int argc, char **argv)
{
	struct perf_event_attr pe;
	long long count;
	int fd, fd2;

	memset(&pe, 0, sizeof(struct perf_event_attr));
	pe.type = PERF_TYPE_HARDWARE;
	pe.size = sizeof(struct perf_event_attr);
	pe.config = PERF_COUNT_HW_REF_CPU_CYCLES;
	pe.disabled = 1;
	pe.exclude_kernel = 1;
	pe.exclude_hv = 1;

	fd = perf_event_open(&pe, 0, -1, -1, 0);
	if (fd == -1) {
		fprintf(stderr, "Error opening leader %llx\n", pe.config);
		exit(EXIT_FAILURE);
	}

	ioctl(fd, PERF_EVENT_IOC_RESET, 0);
	ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

	int itr = 1<<30;
	while (itr--);

	fd2 = perf_event_open(&pe, 0, -1, -1, 0);
	if (fd2 == -1) {
		fprintf(stderr, "Error opening leader2 %llx\n", pe.config);
		exit(EXIT_FAILURE);
	}

	ioctl(fd2, PERF_EVENT_IOC_RESET, 0);
	ioctl(fd2, PERF_EVENT_IOC_ENABLE, 0);

	ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
	read(fd, &count, sizeof(long long));

	printf("Used %lld cycles\n", count);

	ioctl(fd2, PERF_EVENT_IOC_DISABLE, 0);
	read(fd2, &count, sizeof(long long));

	printf("Used2 %lld cycles\n", count);

	close(fd);
	return 0;
}