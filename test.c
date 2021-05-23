#include <stdio.h>
#include "libag.h"

int main(int argc, char **argv)
{
	if (argc < 3)
	{
		fprintf(stderr, "Usage: %s \"regex\" [paths]\n", argv[0]);
		return (1);
	}

	ag_init();

	printf("ag_search: %d\n", ag_search(argv[1], argc - 2, argv + 2));

	ag_stop_workers();
}
