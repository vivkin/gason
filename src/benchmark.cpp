#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>

#if defined(__linux__)
#include <time.h>
#elif defined(__MACH__)
#include <mach/mach_time.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

uint64_t nanotime() {
#if defined(__linux__)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * UINT64_C(1000000000) + ts.tv_nsec;
#elif defined(__MACH__)
    static mach_timebase_info_data_t info;
    if (info.denom == 0)
        mach_timebase_info(&info);
    return mach_absolute_time() * info.numer / info.denom;
#elif defined(_WIN32)
    static LARGE_INTEGER frequency;
    if (frequency.QuadPart == 0)
        QueryPerformanceFrequency(&frequency);
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter.QuadPart * UINT64_C(1000000000) / frequency.QuadPart;
#endif
}

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "gason.h"

struct Stat {
    const char *parserName;
    size_t sourceSize;
    size_t objectCount;
    size_t arrayCount;
    size_t numberCount;
    size_t stringCount;
    size_t trueCount;
    size_t falseCount;
    size_t nullCount;
    size_t memberCount;
    size_t elementCount;
    size_t stringLength;
    uint64_t parseTime;
    uint64_t updateTime;
};

struct Rapid {
    rapidjson::Document doc;

    bool parse(const std::vector<char> &buffer) {
        doc.Parse(buffer.data());
        return doc.HasParseError();
    }
    const char *strError() {
        return rapidjson::GetParseError_En(doc.GetParseError());
    }
    void update(Stat &stat) {
        genStat(stat, doc);
    }
    static void genStat(Stat &stat, const rapidjson::Value &v) {
        using namespace rapidjson;
        switch (v.GetType()) {
        case kNullType:
            stat.nullCount++;
            break;
        case kFalseType:
            stat.falseCount++;
            break;
        case kTrueType:
            stat.trueCount++;
            break;
        case kObjectType:
            for (Value::ConstMemberIterator m = v.MemberBegin(); m != v.MemberEnd(); ++m) {
                stat.stringLength += m->name.GetStringLength();
                genStat(stat, m->value);
            }
            stat.objectCount++;
            stat.memberCount += (v.MemberEnd() - v.MemberBegin());
            stat.stringCount += (v.MemberEnd() - v.MemberBegin());
            break;
        case kArrayType:
            for (Value::ConstValueIterator i = v.Begin(); i != v.End(); ++i)
                genStat(stat, *i);
            stat.arrayCount++;
            stat.elementCount += v.Size();
            break;
        case kStringType:
            stat.stringCount++;
            stat.stringLength += v.GetStringLength();
            break;
        case kNumberType:
            stat.numberCount++;
            break;
        }
    }
    static const char *name() {
        return "rapid normal";
    }
};

struct RapidInsitu : Rapid {
    std::vector<char> source;

    bool parse(const std::vector<char> &buffer) {
        source = buffer;
        doc.ParseInsitu(source.data());
        return doc.HasParseError();
    }
    static const char *name() {
        return "rapid insitu";
    }
};

struct Gason {
    std::vector<char> source;
    JsonAllocator allocator;
    JsonValue value;
    char *endptr;
    int result;

    bool parse(const std::vector<char> &buffer) {
        source = buffer;
        return (result = jsonParse(source.data(), &endptr, &value, allocator)) == JSON_OK;
    }
    const char *strError() {
        return jsonStrError(result);
    }
    void update(Stat &stat) {
        genStat(stat, value);
    }
    static void genStat(Stat &stat, JsonValue v) {
        switch (v.getTag()) {
        case JSON_ARRAY:
            for (auto i : v) {
                genStat(stat, i->value);
                stat.elementCount++;
            }
            stat.arrayCount++;
            break;
        case JSON_OBJECT:
            for (auto i : v) {
                genStat(stat, i->value);
                stat.memberCount++;
                stat.stringLength += strlen(i->key);
                stat.stringCount++;
            }
            stat.objectCount++;
            break;
        case JSON_STRING:
            stat.stringCount++;
            stat.stringLength += strlen(v.toString());
            break;
        case JSON_NUMBER:
            stat.numberCount++;
            break;
        case JSON_TRUE:
            stat.trueCount++;
            break;
        case JSON_FALSE:
            stat.falseCount++;
            break;
        case JSON_NULL:
            stat.nullCount++;
            break;
        }
    }
    static const char *name() {
        return "gason";
    }
};

template <typename T>
static Stat run(size_t iterations, const std::vector<char> &buffer) {
    Stat stat;
    memset(&stat, 0, sizeof(stat));
    stat.parserName = T::name();
    stat.sourceSize = buffer.size() * iterations;

    std::vector<T> docs(iterations);

    auto t = nanotime();
    for (auto &i : docs) {
        i.parse(buffer);
    }
    stat.parseTime += nanotime() - t;

    t = nanotime();
    for (auto &i : docs)
        i.update(stat);
    stat.updateTime += nanotime() - t;

    return stat;
}

static void print(const Stat &stat) {
    printf("%8zd %8zd %8zd %8zd %8zd %8zd %8zd %8zd %8zd %8zd %8zd %11.2f %11.2f %11.2f %s\n",
           stat.objectCount,
           stat.arrayCount,
           stat.numberCount,
           stat.stringCount,
           stat.trueCount,
           stat.falseCount,
           stat.nullCount,
           stat.memberCount,
           stat.elementCount,
           stat.stringLength,
           stat.sourceSize,
           stat.updateTime / 1e6,
           stat.parseTime / 1e6,
           stat.sourceSize / (stat.parseTime / 1e9) / (1 << 20),
           stat.parserName);
}

#if defined(__clang__)
#define COMPILER "Clang " __clang_version__
#elif defined(__GNUC__)
#define COMPILER "GCC " __VERSION__
#endif

#ifndef __x86_64__
#define __x86_64__ 0
#endif

#ifndef NDEBUG
#define NDEBUG 0
#endif

int main(int argc, const char **argv) {
    printf("gason benchmark, %s, x86_64 %d, SIZEOF_POINTER %d, NDEBUG %d\n", COMPILER, __x86_64__, __SIZEOF_POINTER__, NDEBUG);

    size_t iterations = 10;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp("-n", argv[i])) {
            iterations = strtol(argv[++i], NULL, 0);
            continue;
        }

        FILE *fp = fopen(argv[i], "r");
        if (!fp) {
            perror(argv[i]);
            exit(EXIT_FAILURE);
        }
        fseek(fp, 0, SEEK_END);
        size_t size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        std::vector<char> buffer;
        buffer.resize(size + 1);
        fread(buffer.data(), 1, size, fp);
        fclose(fp);

        putchar('\n');
        printf("%s, %zdB x %zd:\n", argv[i], size, iterations);
        printf("%8s %8s %8s %8s %8s %8s %8s %8s %8s %8s %8s %11s %11s %11s\n",
               "Object",
               "Array",
               "Number",
               "String",
               "True",
               "False",
               "Null",
               "Member",
               "Element",
               "StrLen",
               "Size",
               "Update(ms)",
               "Parse(ms)",
               "Speed(Mb/s)");
        print(run<Rapid>(iterations, buffer));
        print(run<RapidInsitu>(iterations, buffer));
        print(run<Gason>(iterations, buffer));
    }
    return 0;
}
