#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
#ifndef NDEBUG
#include <execinfo.h>
#include <signal.h>
#endif
#include <sys/time.h>
#endif
#include "gason.h"

#define INDENT 4

void print_json(JsonValue o, int indent = 0)
{
	switch (o.getTag())
	{
		case JSON_TAG_NUMBER:
			fprintf(stdout, "%f", o.toNumber());
			break;
		case JSON_TAG_BOOL:
			fprintf(stdout, o.toBool() ? "true" : "false");
			break;
		case JSON_TAG_STRING:
			fprintf(stdout, "\"%s\"", o.toString());
			break;
		case JSON_TAG_ARRAY:
			if (!o.toNode())
			{
				fprintf(stdout, "[]");
				break;
			}
			fprintf(stdout, "[\n");
			for (auto i : o)
			{
				fprintf(stdout, "%*s", indent + INDENT, "");
				print_json(i->value, indent + INDENT);
				fprintf(stdout, i->next ? ",\n" : "\n");
			}
			fprintf(stdout, "%*s]", indent, "");
			break;
		case JSON_TAG_OBJECT:
			if (!o.toNode())
			{
				fprintf(stdout, "{}");
				break;
			}
			fprintf(stdout, "{\n");
			for (auto i : o)
			{
				fprintf(stdout, "%*s" "\"%s\": ", indent + INDENT, "", i->key);
				print_json(i->value, indent + INDENT);
				fprintf(stdout, i->next ? ",\n" : "\n");
			}
			fprintf(stdout, "%*s}", indent, "");
			break;
		case JSON_TAG_NULL:
			fprintf(stdout, "null");
			break;
		default:
			fprintf(stderr, "error: unknown value tag %d\n", o.getTag());
			exit(EXIT_FAILURE);
	}
}

void print_error(const char *filename, JsonParseStatus status, char *endptr, char *source, size_t size)
{
	const char *status2str[] = {
		"JSON_PARSE_OK",
		"JSON_PARSE_BAD_NUMBER",
		"JSON_PARSE_BAD_STRING",
		"JSON_PARSE_BAD_IDENTIFIER",
		"JSON_PARSE_STACK_OVERFLOW",
		"JSON_PARSE_STACK_UNDERFLOW",
		"JSON_PARSE_MISMATCH_BRACKET",
		"JSON_PARSE_UNEXPECTED_CHARACTER",
		"JSON_PARSE_UNQUOTED_KEY",
		"JSON_PARSE_BREAKING_BAD"
	};
	char *s = endptr;
	while (s != source && *s != '\n') --s;
	if (s != endptr && s != source) ++s;
	int line = 0;
	for (char *i = s; i != source; --i)
	{
		if (*i == '\n')
		{
			++line;
		}
	}
	int column = (int)(endptr - s);
	fprintf(stderr, "%s:%d:%d: error %s\n", filename, line + 1, column + 1, status2str[status]);
	while (s != source + size && *s != '\n')
	{
		int c = *s++;
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

unsigned long long now()
{
#ifndef _WIN32
	timeval tv;
	gettimeofday(&tv, nullptr);
	return tv.tv_sec * 1000000ull + tv.tv_usec;
#else
	return 0;
#endif
}

int main(int argc, char **argv)
{
#if !defined(_WIN32) && !defined(NDEBUG)
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

	const char *filename = "<stdin>";
	FILE *fp = stdin;
	char *source = nullptr;
	size_t source_size = 0;
	size_t buffer_size = 0;
	if (argc > 1 && !(argv[1][0] == '-' && argv[1][1] == 0))
	{
		filename = argv[1];
		fp = fopen(filename, "rb");
		if (!fp)
		{
			fprintf(stderr, "error: %s: no such file\nusage: json-pretty-print [fileanme]\n", filename);
			exit(EXIT_FAILURE);
		}
		fseek(fp, 0, SEEK_END);
		buffer_size = ftell(fp) + 1;
		fseek(fp, 0, SEEK_SET);
		source = (char *)malloc(buffer_size);
	}
	while (!feof(fp))
	{
		if (source_size + 1 >= buffer_size)
		{
			buffer_size = buffer_size < BUFSIZ ? BUFSIZ : buffer_size * 2;
			source = (char *)realloc(source, buffer_size);
		}
		source_size += fread(source + source_size, 1, buffer_size - source_size - 1, fp);
	}
	fclose(fp);
	source[source_size] = 0;

	char *endptr;
	JsonValue value;
	JsonAllocator allocator;
	unsigned long long t = now();
	JsonParseStatus status = json_parse(source, &endptr, &value, allocator);
	if (status != JSON_PARSE_OK)
	{
		print_error(filename, status, endptr, source, source_size);
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "%s: buffer size %zd, length %zd, parsed %zd bytes for %lluus\n", filename, buffer_size, source_size, endptr - source, now() - t);

	print_json(value);
	fprintf(stdout, "\n");

	return 0;
}
