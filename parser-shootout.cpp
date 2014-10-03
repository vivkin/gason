#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#ifdef ANDROID
#include <android/log.h>
#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "ruberoid", __VA_ARGS__)
#else
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#endif
#include "gason.h"
#include "vjson/json.h"
#include "sajson/include/sajson.h"
#include "rapidjson/include/rapidjson/document.h"

double traverse_gason(JsonValue o)
{
	double x = 0;
	switch (o.getTag())
	{
		case JSON_NUMBER:
			x += o.toNumber();
			break;
		case JSON_ARRAY:
		case JSON_OBJECT:
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

double traverse_rapidjson(const rapidjson::Value &v)
{
	double x = 0;
	if (v.IsObject())
	{
		for (auto i = v.MemberBegin(); i != v.MemberEnd(); ++i)
		{
			x += traverse_rapidjson(i->value);
		}
	}
	else if (v.IsArray())
	{
		for (auto i = v.Begin(); i != v.End(); ++i)
		{
			x += traverse_rapidjson(*i);
		}
	}
	else if (v.IsNumber())
	{
		x = v.GetDouble();
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
			LOG("error: %s: no such file\nusage: json-pretty-print [fileanme]\n", filename);
			exit(EXIT_FAILURE);
		}
		fseek(fp, 0, SEEK_END);
		size_t buffer_size = ftell(fp) + 1;
		fseek(fp, 0, SEEK_SET);
		char *buffer = (char *)malloc(buffer_size);
		fread(buffer, 1, buffer_size - 1, fp);
		fclose(fp);
		buffer[buffer_size - 1] = 0;

		LOG("%s: length %zd\n", filename, buffer_size);
		unsigned long long t;

		// gason
		{
			char *source = strdup(buffer);

			char *endptr;
			JsonValue value;
			JsonAllocator allocator;
			t = now();
			int status = jsonParse(source, &endptr, &value, allocator);
			auto parse_time = now() - t;
			if (status != JSON_OK)
			{
				LOG("error: gason: %s\n", jsonStrError(status));
			}
			t = now();
			double x = traverse_gason(value);
			auto traverse_time = now() - t;
			LOG("%10s %10lluus %10lluus \t(%f)\n", "gason", parse_time, traverse_time, x);

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
				LOG("error: vjson: %s\n", errorDesc);
			}
			t = now();
			double x = traverse_vjson(root);
			auto traverse_time = now() - t;
			LOG("%10s %10lluus %10lluus \t(%f)\n", "vjson", parse_time, traverse_time, x);

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
				LOG("error: sajson: %s\n", document.get_error_message().c_str());
			}
			t = now();
			double x = traverse_sajson(document.get_root());
			auto traverse_time = now() - t;
			LOG("%10s %10lluus %10lluus \t(%f)\n", "sajson", parse_time, traverse_time, x);

			free(source);
		}

		// rapidjson
		{
			char *source = strdup(buffer);

			t = now();
			rapidjson::Document document;
			if (document.ParseInsitu<0>(source).HasParseError())
			{
				LOG("error: rapidjson: %s\n", document.GetParseError());
			}
			auto parse_time = now() - t;
			t = now();
			double x = traverse_rapidjson(document);
			auto traverse_time = now() - t;
			LOG("%10s %10lluus %10lluus \t(%f)\n", "rapidjson", parse_time, traverse_time, x);

			free(source);
		}
	}
	return 0;
}
