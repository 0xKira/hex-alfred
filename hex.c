#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

uint8_t  x8[64];
uint16_t x12[64];
uint16_t x16[32];
uint32_t x32[16];

#define min(a,b) ((a) < (b) ? (a) : (b))
#define printx(n,f) do { \
	int i, skip; \
	for (skip = 1, i = 0; i < sizeof(x ## n) / sizeof(x ## n[0]); i++) { \
		if (x ## n[i] == 0 && skip) { continue; } \
		skip = 0; printf(f " ", x ## n[i]); \
	} \
} while (0)
#define printc(n) do { \
	int i, skip; \
	for (skip = 1, i = 0; i < sizeof(x ## n) / sizeof(x ## n[0]); i++) { \
		if (x ## n[i] == 0 && skip) { continue; } \
		skip = 0; printf("%c ", isprint(x ## n[i]) ? x ## n[i] : '.'); \
	} \
} while (0)

int main(int argc, char **argv)
{
	int skip, i, j;
	int is_str, has_ws, b8, b10, b16;
	long long n;

	printf("<items>\n");

	b8 = b10 = b16 = 0;
	if (argv[1][0] == '0') {
		b8 = 1;
	}

	for (i = 0; i < strlen(argv[1]); i++) {
		if (argv[1][i] >= '0' && argv[1][i] <= '7') {
			continue;
		}
		if (argv[1][i] >= '8' && argv[1][i] <= '9') {
			b10 = 1;
			continue;
		}
		if ((argv[1][i] >= 'a' && argv[1][i] <= 'f') || argv[1][i] == 'x') {
			b16 = 1;
			continue;
		}
		if (isspace(argv[1][i])) {
			has_ws = 1;
			continue;
		}
		is_str = 1;
	}

	if (has_ws && !is_str) {
		char *arg, *end;

		i = 0;
		arg = argv[1];
		while (*arg && i < 64) {
			errno = 0;
			n = strtoll(arg, &end, 16);
			if (errno != 0) {
				goto efail;
			}
			if (n > 0xff || n < 0) {
				goto nfail;
			}

			x8[i++] = n;

			arg = end;
			while (isspace(*arg)) {
				arg++;
			}
		}

		i--;
		for (j = 63; i >= 0; i--, j--) {
			x8[j] = x8[i];
		}
		for (j--; j >= 0; j--) {
			x8[j] = 0;
		}

	} else if (is_str) {
		int offset = 64 - (min(64, strlen(argv[1]) + 1) & ~0x1);
		for (i = 0; i < offset; i++) {
			x8[i + offset] = argv[1][i];
			if (argv[1][i] == '\0') {
				break;
			}
		}

	} else {
		int base = 10;

		if (b8 && !b10 && !b16) {
			base = 8;
		} else if (b16) {
			base = 16;
		}

		errno = 0;
		n = strtoll(argv[1], NULL, base);
		if (errno != 0) {
			goto efail;
		}
		if (n < 0) {
			goto nfail;
		}
		for (i = 63; i >= 0 && n > 0; i--) {
			x8[i] = n & 0xff;
			n = n >> 8;
		}
	}

	for (i = 0; i < 63; ) {
		x16[i/2] = x8[i] << 8 | x8[i+1];
		i += 2;
	}
	for (i = 0; i < 63; ) {
		x32[i/4] = x8[i] << 24 | x8[i|1] << 16 | x8[i|2] << 8 | x8[i|3];
		i += 4;
	}
	skip = 4; /* right skip */
	for (i = 63, j = 63; i >= 1 && j >= 0; ){
		switch (skip) {
		case 4: /* 0000 1111 1111 1111 */
			x12[j]  =  x8[i--];
			x12[j] |= (x8[i] & 0x0f) << 8;
			j--;
			skip = 8;
			break;

		case 8: /* 1111 1111 1111 xxxx */
			x12[j]  = (x8[i--] & 0xf0) >> 4;
			x12[j] |=  x8[i--] << 4;
			j--;
			skip = 4;
			break;
		}
	}

#define ATTRS " valid=\"yes\" auto=\"\""
	printf("<item uuid=\"hex\"" ATTRS "><arg>");
	printx(8, "%02x");
	printf("</arg><title>");
	printx(8, "%02x");
	printf("</title><subtitle>hex</subtitle></item>\n");

	printf("<item uuid=\"oct\"" ATTRS "><arg>");
	printx(12, "%04o");
	printf("</arg><title>");
	printx(12, "%04o");
	printf("</title><subtitle>octal</subtitle></item>\n");

	printf("<item uuid=\"ascii\"" ATTRS "><arg>");
	printc(8);
	printf("</arg><title>");
	printc(8);
	printf("</title><subtitle>ascii</subtitle></item>\n");

	printf("<item uuid=\"dec8\"" ATTRS "><arg>");
	printx(8, "%d");
	printf("</arg><title>");
	printx(8, "%d");
	printf("</title><subtitle>decimal 8</subtitle></item>\n");

	printf("<item uuid=\"dec16\"" ATTRS "><arg>");
	printx(16, "%d");
	printf("</arg><title>");
	printx(16, "%d");
	printf("</title><subtitle>decimal 16</subtitle></item>\n");

	printf("<item uuid=\"dec32\"" ATTRS "><arg>");
	printx(32, "%d");
	printf("</arg><title>");
	printx(32, "%d");
	printf("</title><subtitle>decimal 32</subtitle></item>\n");

	printf("</items>\n");
	return 0;

efail:
	printf("<item uuid=\"errno\"><title>oops... %s</title><subtitle>that's an error code %d</subtitle></item>\n",
		strerror(errno), errno);
	printf("</items>\n");
	return 1;

nfail:
	printf("<item uuid=\"errno\"><title>oops... %s is an odd looking numeric</title></item>\n", argv[1]);
	printf("</items>\n");
	return 2;
}
