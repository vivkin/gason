#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#if !defined(_WIN32) && !defined(NDEBUG)
#include <execinfo.h>
#include <signal.h>
#endif
#include "gason.h"

const int SHIFT_WIDTH = 4;

void dumpString(const char *s) {
	fputc('"', stdout);
	while (*s) {
		int c = *s++;
		switch (c) {
		case '\b':
			fprintf(stdout, "\\b");
			break;
		case '\f':
			fprintf(stdout, "\\f");
			break;
		case '\n':
			fprintf(stdout, "\\n");
			break;
		case '\r':
			fprintf(stdout, "\\r");
			break;
		case '\t':
			fprintf(stdout, "\\t");
			break;
		case '\\':
			fprintf(stdout, "\\\\");
			break;
		case '"':
			fprintf(stdout, "\\\"");
			break;
		default:
			fputc(c, stdout);
		}
	}
	fprintf(stdout, "%s\"", s);
}

void dumpValue(JsonValue o, int indent = 0) {
	switch (o.getTag()) {
	case JSON_TAG_NUMBER:
		fprintf(stdout, "%f", o.toNumber());
		break;
	case JSON_TAG_BOOL:
		fprintf(stdout, o.toBool() ? "true" : "false");
		break;
	case JSON_TAG_STRING:
		dumpString(o.toString());
		break;
	case JSON_TAG_ARRAY:
		// It is not necessary to use o.toNode() to check if an array or object
		// is empty before iterating over its members, we do it here to allow
		// nicer pretty printing.
		if (!o.toNode()) {
			fprintf(stdout, "[]");
			break;
		}
		fprintf(stdout, "[\n");
		for (auto i : o) {
			fprintf(stdout, "%*s", indent + SHIFT_WIDTH, "");
			dumpValue(i->value, indent + SHIFT_WIDTH);
			fprintf(stdout, i->next ? ",\n" : "\n");
		}
		fprintf(stdout, "%*s]", indent, "");
		break;
	case JSON_TAG_OBJECT:
		if (!o.toNode()) {
			fprintf(stdout, "{}");
			break;
		}
		fprintf(stdout, "{\n");
		for (auto i : o) {
			fprintf(stdout, "%*s", indent + SHIFT_WIDTH, "");
			dumpString(i->key);
			fprintf(stdout, ": ");
			dumpValue(i->value, indent + SHIFT_WIDTH);
			fprintf(stdout, i->next ? ",\n" : "\n");
		}
		fprintf(stdout, "%*s}", indent, "");
		break;
	case JSON_TAG_NULL:
		fprintf(stdout, "null");
		break;
	}
}

void printError(const char *filename, JsonParseStatus status, char *endptr, char *source, size_t size) {
	static const char *status2string[] = {
		"JSON_PARSE_OK",
		"JSON_PARSE_BAD_NUMBER",
		"JSON_PARSE_BAD_STRING",
		"JSON_PARSE_BAD_IDENTIFIER",
		"JSON_PARSE_STACK_OVERFLOW",
		"JSON_PARSE_STACK_UNDERFLOW",
		"JSON_PARSE_MISMATCH_BRACKET",
		"JSON_PARSE_UNEXPECTED_CHARACTER",
		"JSON_PARSE_UNQUOTED_KEY",
		"JSON_PARSE_BREAKING_BAD"};

	char *s = endptr;
	while (s != source && *s != '\n')
		--s;
	if (s != endptr && s != source)
		++s;

	int lineno = 0;
	for (char *it = s; it != source; --it) {
		if (*it == '\n') {
			++lineno;
		}
	}

	int column = (int)(endptr - s);

	fprintf(stderr, "%s:%d:%d: error %s\n", filename, lineno + 1, column + 1, status2string[status]);

	while (s != source + size && *s != '\n') {
		int c = *s++;
		switch (c) {
		case '\b':
			fprintf(stderr, "\\b");
			column += 1;
			break;
		case '\f':
			fprintf(stderr, "\\f");
			column += 1;
			break;
		case '\n':
			fprintf(stderr, "\\n");
			column += 1;
			break;
		case '\r':
			fprintf(stderr, "\\r");
			column += 1;
			break;
		case '\t':
			fprintf(stderr, "%*s", SHIFT_WIDTH, "");
			column += SHIFT_WIDTH - 1;
			break;
		case '\0':
			fprintf(stderr, "\"");
			break;
		default:
			fputc(c, stderr);
		}
	}

	fprintf(stderr, "\n%*s\n", column + 1, "^");
}

int main(int argc, char **argv) {
#if !defined(_WIN32) && !defined(NDEBUG)
	signal(SIGABRT, [](int) {
		void *callstack[64];
		int size = backtrace(callstack, arraySize(callstack));
		char **strings = backtrace_symbols(callstack, size);
		for (int i = 0; i < size; ++i)
			fprintf(stderr, "%s\n", strings[i]);
		free(strings);
		exit(EXIT_FAILURE);
	});
#endif

	FILE *fp = (argc > 1 && strcmp(argv[1], "-")) ? fopen(argv[1], "rb") : stdin;
	if (!fp) {
		fprintf(stderr, "%s: %s: %s\n", argv[0], argv[1], strerror(errno));
		exit(EXIT_FAILURE);
	}
	char *source = nullptr;
	size_t sourceSize = 0;
	size_t bufferSize = 0;
	while (!feof(fp)) {
		if (sourceSize + 1 >= bufferSize) {
			bufferSize = bufferSize < BUFSIZ ? BUFSIZ : bufferSize * 2;
			source = (char *)realloc(source, bufferSize);
		}
		sourceSize += fread(source + sourceSize, 1, bufferSize - sourceSize - 1, fp);
	}
	fclose(fp);
	source[sourceSize] = 0;

	char *endptr;
	JsonValue value;
	JsonAllocator allocator;
	JsonParseStatus status = jsonParse(source, &endptr, &value, allocator);
	if (status != JSON_PARSE_OK) {
		printError((argc > 1 && strcmp(argv[1], "-")) ? argv[1] : "-stdin-", status, endptr, source, sourceSize);
		exit(EXIT_FAILURE);
	}
	dumpValue(value);
	fprintf(stdout, "\n");

	return 0;
}
