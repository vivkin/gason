#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "gason.h"
#include "vjson/json.h"

double traverse_gason(JsonValue o)
{
	double x = 0;
	switch (o.getTag())
	{
		case JSON_TAG_NUMBER:
			x += o.toNumber();
			break;
		case JSON_TAG_ARRAY:
		case JSON_TAG_OBJECT:
			for (auto i : o)
			{
				x += traverse_gason(i->value);
			}
			break;
		default:
			return 0;
	}
	return x;
}

double traverse_vjson(json_value *value)
{
	double x = 0;
	switch (value->type)
	{
		case JSON_FLOAT:
			x += value->float_value;
			break;
		case JSON_INT:
			x += value->int_value;
			break;
		case JSON_ARRAY:
		case JSON_OBJECT:
			for (json_value *it = value->first_child; it; it = it->next_sibling)
			{
				x += traverse_vjson(it);
			}
			break;
		default:
			return 0;
	}
	return x;
}

unsigned long long now()
{
	timeval tv;
	gettimeofday(&tv, nullptr);
	return tv.tv_sec * 1000000ull + tv.tv_usec;
}

int main(int argc, char **argv)
{
	for (int i = 1; i < argc; ++i)
	{
		const char *filename = argv[i];
		FILE *fp = fopen(filename, "rb");
		if (!fp)
		{
			fprintf(stderr, "error: %s: no such file\nusage: json-pretty-print [fileanme]\n", filename);
			exit(EXIT_FAILURE);
		}
		fseek(fp, 0, SEEK_END);
		size_t buffer_size = ftell(fp) + 1;
		fseek(fp, 0, SEEK_SET);
		char *buffer = (char *)malloc(buffer_size);
		fread(buffer, 1, buffer_size - 1, fp);
		fclose(fp);
		buffer[buffer_size - 1] = 0;

		fprintf(stderr, "%s: length %zd\n", filename, buffer_size);
		unsigned long long t;

		// gason
		{
			char *source = (char *)malloc(buffer_size);
			memcpy(source, buffer, buffer_size);

			char *endptr;
			JsonValue value;
			JsonAllocator allocator;
			t = now();
			JsonParseStatus status = json_parse(source, &endptr, &value, allocator);
			auto parse_time = now() - t;
			if (status != JSON_PARSE_OK)
			{
				fprintf(stderr, "error: gason: %d\n", (int)status);
			}
			t = now();
			double x = traverse_gason(value);
			auto traverse_time = now() - t;
			fprintf(stderr, "%.16s %f %10lluus %10lluus\n", "gason", x, parse_time, traverse_time);

			free(source);
		}

		// vjson
		{
			char *source = (char *)malloc(buffer_size);
			memcpy(source, buffer, buffer_size);

			char *errorPos = 0;
			char *errorDesc = 0;
			int errorLine = 0;
			block_allocator allocator(4096);
			t = now();
			json_value *root = json_parse(source, &errorPos, &errorDesc, &errorLine, &allocator);
			auto parse_time = now() - t;
			if (!root)
			{
				fprintf(stderr, "error: vjson: %s\n", errorDesc);
			}
			t = now();
			double x = traverse_vjson(root);
			auto traverse_time = now() - t;
			fprintf(stderr, "%.16s %f %10lluus %10lluus\n", "vjson", x, parse_time, traverse_time);

			free(source);
		}
	}
	return 0;
}
