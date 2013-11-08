#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "gason.h"
#include "vjson/json.h"
#include "sajson/include/sajson.h"
#include "stix-json/JsonParser.h"

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

double traverse_sajson(const sajson::value &v)
{
	double x = 0;
	switch (v.get_type())
	{
		case sajson::TYPE_DOUBLE:
			x += v.get_double_value();
			break;
		case sajson::TYPE_INTEGER:
			x += v.get_integer_value();
			break;
		case sajson::TYPE_ARRAY:
			for (size_t i = 0; i < v.get_length(); ++i)
			{
				x += traverse_sajson(v.get_array_element(i));
			}
			break;
		case sajson::TYPE_OBJECT:
			for (size_t i = 0; i < v.get_length(); ++i)
			{
				x += traverse_sajson(v.get_object_value(i));
			}
			break;
		default:
			return 0;
	}
	return x;
}

double traverse_stixjson(const stix::JsonValue &v)
{
	double x = 0;
	switch (v.GetType())
	{
		case JSMN_PRIMITIVE:
			x += v.AsNumber();
			break;
		case JSMN_ARRAY:
			for (stix::u32 i = 0; i < v.GetElementsCount(); ++i)
			{
				x += traverse_stixjson(v[i]);
			}
			break;
		case JSMN_OBJECT:
			for (stix::u32 i = 0; i < v.GetElementsCount(); ++i)
			{
				x += traverse_stixjson(v[i].GetValue());
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
			char *source = strdup(buffer);

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
			fprintf(stderr, "%10s %f %10lluus %10lluus\n", "gason", x, parse_time, traverse_time);

			free(source);
		}

		// vjson
		{
			char *source = strdup(buffer);

			char *errorPos = 0;
			const char *errorDesc = 0;
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
			fprintf(stderr, "%10s %f %10lluus %10lluus\n", "vjson", x, parse_time, traverse_time);

			free(source);
		}

		// sajson
		{
			char *source = strdup(buffer);

			t = now();
			const sajson::document& document = parse(sajson::string(source, buffer_size));
			auto parse_time = now() - t;
			if (!document.is_valid())
			{
				fprintf(stderr, "error: sajson: %s\n", document.get_error_message().c_str());
			}
			t = now();
			double x = traverse_sajson(document.get_root());
			auto traverse_time = now() - t;
			fprintf(stderr, "%10s %f %10lluus %10lluus\n", "sajson", x, parse_time, traverse_time);

			free(source);
		}

		// stix-json
		{
			char *source = strdup(buffer);

			stix::JsonParser parser;
			t = now();
			int status = parser.ParseJsonString(source);
			auto parse_time = now() - t;
			if (status != JSMN_SUCCESS)
			{
				fprintf(stderr, "error: stix-json: %d\n", status);
			}
			t = now();
			double x = traverse_stixjson(parser.GetRoot());
			auto traverse_time = now() - t;
			fprintf(stderr, "%10s %f %10lluus %10lluus\n", "stix-json", x, parse_time, traverse_time);

			free(source);
		}
	}
	return 0;
}
