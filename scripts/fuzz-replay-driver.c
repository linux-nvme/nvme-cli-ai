// SPDX-License-Identifier: Apache-2.0
/* replay FILE...: run each fuzz-device input once, output not redirected */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int LLVMFuzzerInitialize(int *argc, char ***argv);
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

int main(int argc, char **argv)
{
	static uint8_t buf[1 << 16];
	int i;

	LLVMFuzzerInitialize(&argc, &argv);
	for (i = 1; i < argc; i++) {
		FILE *f = fopen(argv[i], "rb");
		size_t n;

		if (!f)
			continue;
		n = fread(buf, 1, sizeof(buf), f);
		fclose(f);
		printf("##### %s\n", argv[i]);
		fflush(stdout);
		fprintf(stderr, "##### %s\n", argv[i]);
		LLVMFuzzerTestOneInput(buf, n);
		fflush(stdout);
		fflush(stderr);
	}
	return 0;
}
