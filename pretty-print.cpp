#include <stdlib.h>
#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <sys/time.h>
#include "gason.h"

#define INDENT 4

void print_json(JsonValue o, int indent = 0)
{
	switch (o.getTag())
	{
		case JSON_TAG_NUMBER:
			fprintf(stdout, "%.2f", o.toNumber());
			break;
		case JSON_TAG_BOOL:
			fprintf(stdout, o.toBool() ? "true" : "false");
			break;
		case JSON_TAG_STRING:
			fprintf(stdout, "\"%s\"", o.toString());
			break;
		case JSON_TAG_ARRAY:
			if (!o.toElement())
			{
				fprintf(stdout, "[]");
				break;
			}
			fprintf(stdout, "[\n");
			for (auto i = o.toElement(); i; i = i->next)
			{
				fprintf(stdout, "%*s", indent + INDENT, "");
				print_json(i->value, indent + INDENT);
				fprintf(stdout, i->next ? ",\n" : "\n");
			}
			fprintf(stdout, "%*s]", indent, "");
			break;
		case JSON_TAG_OBJECT:
			if (!o.toPair())
			{
				fprintf(stdout, "{}");
				break;
			}
			fprintf(stdout, "{\n");
			for (auto i = o.toPair(); i; i = i->next)
			{
				fprintf(stdout, "%*s" "\"%s\": ", indent + INDENT, "", i->key);
				print_json(i->value, indent + INDENT);
				fprintf(stdout, i->next ? ",\n" : "\n");
			}
			fprintf(stdout, "%*s}", indent, "");
			break;
		case JSON_TAG_NULL:
			printf("null");
			break;
		default:
			fprintf(stderr, "error: unknown value tag %d\n", o.getTag());
			exit(EXIT_FAILURE);
	}
}

void print_error(const char *filename, JsonParseStatus status, char *endptr, char *source, size_t size)
{
	char *first = endptr;
	while (first != source && *first != '\n') --first;
	if (first != source) ++first;
	char *last = endptr;
	while (last != source + size && *last != '\n') ++last;
	int line = 0;
	for (char *i = first; i >= source; --i)
	{
		if (*i == '\n')
		{
			++line;
		}
	}
	int column = (int)(endptr - first);
	fprintf(stderr, "%s:%d:%d: error %d\n", filename, line + 1, column + 1, (int)status);
	for (char *i = first; i != last; ++i)
	{
		int c = *i;
		switch (c)
		{
			case '\b': fprintf(stderr, "\\b"); column += 1; break;
			case '\f': fprintf(stderr, "\\f"); column += 1; break;
			case '\n': fprintf(stderr, "\\n"); column += 1; break;
			case '\r': fprintf(stderr, "\\r"); column += 1; break;
			case '\t': fprintf(stderr, "%*s", INDENT, ""); column += INDENT - 1; break;
			case 0: fprintf(stderr, "\""); break;
			default: fprintf(stderr, "%c", c);
		}
	}
	fprintf(stderr, "\n%*s\n", column + 1, "^");
}

uint64_t now()
{
	timeval tv;
	gettimeofday(&tv, nullptr);
	return tv.tv_sec * 1000ull + tv.tv_usec / 1000;
}

int main(int argc, char **argv)
{
#ifndef NDEBUG
	auto abort_handler = [](int)
	{
		void *callstack[16];
		int size = backtrace(callstack, sizeof(callstack) / sizeof(callstack[0]));
		char **names = backtrace_symbols(callstack, size);
		for (int i = 0; i < size; ++i)
		{
			fprintf(stderr, "%s\n", names[i]);
		}
		free(names);
		exit(EXIT_FAILURE);
	};
	signal(SIGABRT, abort_handler);
#endif

	FILE *fp = nullptr;
	assert(fp);
	JsonAllocator allocator;
	char *source;
	size_t size;
	if (argc > 1)
	{
		if (argv[1][0] == '-' && argv[1][1] == 0)
		{
			fp = stdin;
		}
		else
		{
			fp = fopen(argv[1], "rb");
		}
	}
	if (fp)
	{
		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		source = (char *)allocator.allocate(size + 1);
		source[size] = 0;
		fread(source, 1, size, fp);
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "error: can't open file: %s\nusage: json-pretty-print [fileanme]\n\"-\" as filename for use stdin\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	JsonValue value;
	char *endptr;
	uint64_t t = now();
	JsonParseStatus status = json_parse(source, &endptr, &value, allocator);
	if (status != JSON_PARSE_OK)
	{
		print_error(argv[1], status, endptr, source, size);
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "%lld ms\n", now() - t);
	print_json(value);
	fprintf(stdout, "\n");
	return 0;
}
