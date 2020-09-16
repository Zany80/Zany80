#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <ctype.h>
#include "format.h"

typedef enum {
	S_default = 0,
	S_unsigned = 1,
	S_lower = 2,
	S_prefix = 4
} spec_flags;

typedef enum {
	T_char,
	T_short,
	T_int,
	T_long,
	T_llong,
	T_size,
	T_intptr,
	T_ptrdiff,
	T_intmax
} spec_type;

struct format_spec {
	int base, width;
	spec_flags flags;
	spec_type type;
};

static uintmax_t get_value(struct format_spec spec, uintmax_t (*getarg)(size_t)) {
	if (spec.flags & S_unsigned) {
		switch (spec.type) {
		case T_char:
			return (uintmax_t)(char)getarg(sizeof(int));
		case T_short:
			return (uintmax_t)(short)getarg(sizeof(int));
		case T_int:
			return (uintmax_t)getarg(sizeof(unsigned int));
		case T_long:
			return (uintmax_t)getarg(sizeof(unsigned long));
		case T_llong:
			return (uintmax_t)getarg(sizeof(unsigned long long));
		case T_size:
			return (uintmax_t)getarg(sizeof(size_t));
		case T_intptr:
			return (uintmax_t)getarg(sizeof(uintptr_t));
		case T_ptrdiff:
			return (uintmax_t)getarg(sizeof(ptrdiff_t));
		case T_intmax:
			return getarg(sizeof(uintmax_t));
		}
	} else {
		switch (spec.type) {
		case T_char:
			return (uintmax_t)(unsigned char)getarg(sizeof(int));
		case T_short:
			return (uintmax_t)(unsigned short)getarg(sizeof(int));
		case T_int:
			return (uintmax_t)getarg(sizeof(int));
		case T_long:
			return (uintmax_t)getarg(sizeof(long));
		case T_llong:
			return (uintmax_t)getarg(sizeof(long long));
		case T_size:
			return (uintmax_t)getarg(sizeof(size_t));
		case T_intptr:
			return (uintmax_t)getarg(sizeof(intptr_t));
		case T_ptrdiff:
			return (uintmax_t)getarg(sizeof(ptrdiff_t));
		case T_intmax:
			return getarg(sizeof(uintmax_t));
		}
	}
	return 0;
}

static void putstr(void (*putc)(char), const char *str) {
	while (*str) {
		putc(*str);
		++str;
	}
}

static void handle_spec(void (*putc)(char), uintmax_t (*getarg)(size_t), const char **f) {
	struct format_spec spec = {
		.base = 10,
		.width = 0,
		.flags = S_default,
		.type = T_int
	};

	++*f;

	bool repeat;
	do {
		repeat = false;

		switch (**f) {
		case 'd': /* fallthrough */
		case 'i':
			spec.base = 10;
			break;
		case 'o':
			spec.base = 8;
			spec.flags |= S_unsigned;
			break;
		case 'x':
			spec.base = 16;
			spec.flags |= (S_lower | S_unsigned);
			break;
		case 'X':
			spec.base = 16;
			spec.flags |= S_unsigned;
			break;
		case 'p':
			spec.base = 16;
			spec.flags |= (S_prefix | S_unsigned | S_lower);
			spec.type = T_intptr;
			break;
		case 'c':
		{
			char c = getarg(sizeof(int));
			putc(c);
			return;
		}
		case 's':
		{
			char *s = (char *)getarg(sizeof(char *));
			if (!s) {
				putstr(putc, "(nil)");
			} else {
				putstr(putc, s);
			}
			return;
		}
		case 'z':
			spec.type = T_size;
			repeat = true;
			++*f;
			break;
		case '%':
			putc('%');
			return;
		default:
			if (isdigit(**f)) {
				char *endptr;
				spec.width = strtol(*f, &endptr, 10);
				*f += endptr - *f;
				repeat = true;
			}
			break;
		}
	} while (repeat);

	char buffer[32];
	// TODO: Deal with things that are not integers
	
	uintmax_t val = get_value(spec, getarg);
	
	const char *digits = "0123456789ABCDEF";

	if (spec.base != 0) {
		int i = 0;
		uintmax_t value = val;
		if (!(spec.flags & S_unsigned)) {
			if ((intmax_t)value < 0) {
				value = (uintmax_t)-(intmax_t)value;
			}
		}
		if (spec.type == T_intptr && value == 0) {
			putstr(putc, "(nil)");
			return;
		} else {
			do {
				char d = digits[value % spec.base];
				if (spec.flags & S_lower) {
					d = tolower(d);
				}
				buffer[i++] = d;
				value /= spec.base;
			} while (value);
		}
		if (!(spec.flags & S_unsigned) && (intmax_t)val < 0) {
			putc('-');
		}
		if (spec.flags & S_prefix) {
			if (spec.base == 16 && val != 0) {
				buffer[i++] = 'x';
				buffer[i++] = '0';
			}
			if (spec.base == 8) {
				buffer[i++] = '0';
			}
		}
		if (i < spec.width) {
			for (int j = 0; j < spec.width - i; ++j) {
				putc('0');
			}
		}
		for (--i; i >= 0; --i) {
			putc(buffer[i]);
		}
	}
}

void format(void (*putc)(char), uintmax_t (*getarg)(size_t size), const char *fmt) {
	while (*fmt) {
		if (*fmt == '%') {
			handle_spec(putc, getarg, &fmt);
			++fmt;
		} else {
			putc(*fmt);
			++fmt;
		}
	}
}
