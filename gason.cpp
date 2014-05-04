#include <stdlib.h>
#include <ctype.h>
#include "gason.h"

inline bool isdelim(char c) { return isspace(c) || c == ',' || c == ':' || c == ']' || c == '}' || c == '\0'; }

inline int char2int(char c)
{
	if (c >= 'a') return c - 'a' + 10;
	if (c >= 'A') return c - 'A' + 10;
	return c - '0';
}

static double str2float(const char *str, char **endptr)
{
	char sign = *str;
	if (sign == '+' || sign == '-')
		++str;
	double result = 0;
	while (isdigit(*str)) result = (result * 10) + (*str++ - '0');
	if (*str == '.')
	{
		++str;
		double fraction = 1;
		while (isdigit(*str)) fraction *= 0.1, result += (*str++ - '0') * fraction;
	}
	if (*str == 'e' || *str == 'E')
	{
		++str;
		double base = 10;
		if (*str == '+')
			++str;
		else if (*str == '-') {
			++str;
			base = 0.1;
		}
		int exponent = 0;
		while (isdigit(*str)) exponent = (exponent * 10) + (*str++ - '0');
		double power = 1;
		for (; exponent; exponent >>= 1, base *= base) if (exponent & 1) power *= base;
		result *= power;
	}
	*endptr = (char *)str;
	return sign == '-' ? -result : result;
}

JsonAllocator::~JsonAllocator()
{
	while (head)
	{
		Zone *temp = head->next;
		free(head);
		head = temp;
	}
}

inline void *align_pointer(void *x, size_t align) { return (void *)(((uintptr_t)x + (align - 1)) & ~(align - 1)); }

void *JsonAllocator::allocate(size_t n, size_t align)
{
	if (head)
	{
		char *p = (char *)align_pointer(head->end, align);
		if (p + n <= (char *)head + JSON_ZONE_SIZE)
		{
			head->end = p + n;
			return p;
		}
	}
	size_t zone_size = sizeof(Zone) + n + align;
	Zone *z = (Zone *)malloc(zone_size <= JSON_ZONE_SIZE ? JSON_ZONE_SIZE : zone_size);
	char *p = (char *)align_pointer(z + 1, align);
	z->end = p + n;
	if (zone_size <= JSON_ZONE_SIZE || head == nullptr)
	{
		z->next = head;
		head = z;
	}
	else
	{
		z->next = head->next;
		head->next = z;
	}
	return p;
}

struct JsonList
{
	JsonTag tag;
	JsonValue node;
	char *key;

	void grow_the_tail(JsonNode *p)
	{
		JsonNode *tail = (JsonNode *)node.getPayload();
		if (tail)
		{
			p->next = tail->next;
			tail->next = p;
		}
		else
		{
			p->next = p;
		}
		node = JsonValue(tag, p);
	}

	JsonValue cut_the_head()
	{
		JsonNode *tail = (JsonNode *)node.getPayload();
		if (tail)
		{
			JsonNode *head = tail->next;
			tail->next = nullptr;
			return JsonValue(tag, head);
		}
		return node;
	}
};

JsonParseStatus json_parse(char *str, char **endptr, JsonValue *value, JsonAllocator &allocator)
{
	JsonList stack[JSON_STACK_SIZE];
	int top = -1;
	bool separator = true;
	while (*str)
	{
		JsonValue o;
		while (*str && isspace(*str)) ++str;
		*endptr = str++;
		switch (**endptr)
		{
			case '\0':
				continue;
			case '-':
				if (!isdigit(*str) && *str != '.') return *endptr = str, JSON_PARSE_BAD_NUMBER;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				o = JsonValue(str2float(*endptr, &str));
				if (!isdelim(*str)) return *endptr = str, JSON_PARSE_BAD_NUMBER;
				break;
			case '"':
				o = JsonValue(JSON_TAG_STRING, str);
				for (char *s = str; *str; ++s, ++str)
				{
					int c = *s = *str;
					if (c == '\\')
					{
						c = *++str;
						switch (c)
						{
							case '\\':
							case '"':
							case '/': *s = c; break;
							case 'b': *s = '\b'; break;
							case 'f': *s = '\f'; break;
							case 'n': *s = '\n'; break;
							case 'r': *s = '\r'; break;
							case 't': *s = '\t'; break;
							case 'u':
								c = 0;
								for (int i = 0; i < 4; ++i)
								{
									if (!isxdigit(*++str)) return *endptr = str, JSON_PARSE_BAD_STRING;
									c = c * 16 + char2int(*str);
								}
								if (c < 0x80)
								{
									*s = c;
								}
								else if (c < 0x800)
								{
									*s++ = 0xC0 | (c >> 6);
									*s = 0x80 | (c & 0x3F);
								}
								else
								{
									*s++ = 0xE0 | (c >> 12);
									*s++ = 0x80 | ((c >> 6) & 0x3F);
									*s = 0x80 | (c & 0x3F);
								}
								break;
							default:
								return *endptr = str, JSON_PARSE_BAD_STRING;
						}
					}
					else if (c == '"')
					{
						*s = 0;
						++str;
						break;
					}
				}
				if (!isdelim(*str)) return *endptr = str, JSON_PARSE_BAD_STRING;
				break;
			case 't':
				for (const char *s = "rue"; *s; ++s, ++str)
				{
					if (*s != *str) return JSON_PARSE_BAD_IDENTIFIER;
				}
				if (!isdelim(*str)) return JSON_PARSE_BAD_IDENTIFIER;
				o = JsonValue(JSON_TAG_BOOL, (void *)true);
				break;
			case 'f':
				for (const char *s = "alse"; *s; ++s, ++str)
				{
					if (*s != *str) return JSON_PARSE_BAD_IDENTIFIER;
				}
				if (!isdelim(*str)) return JSON_PARSE_BAD_IDENTIFIER;
				o = JsonValue(JSON_TAG_BOOL, (void *)false);
				break;
			case 'n':
				for (const char *s = "ull"; *s; ++s, ++str)
				{
					if (*s != *str) return JSON_PARSE_BAD_IDENTIFIER;
				}
				if (!isdelim(*str)) return JSON_PARSE_BAD_IDENTIFIER;
				break;
			case ']':
				if (top == -1) return JSON_PARSE_STACK_UNDERFLOW;
				if (stack[top].tag != JSON_TAG_ARRAY) return JSON_PARSE_MISMATCH_BRACKET;
				o = stack[top--].cut_the_head();
				break;
			case '}':
				if (top == -1) return JSON_PARSE_STACK_UNDERFLOW;
				if (stack[top].tag != JSON_TAG_OBJECT) return JSON_PARSE_MISMATCH_BRACKET;
				o = stack[top--].cut_the_head();
				break;
			case '[':
				if (++top == JSON_STACK_SIZE) return JSON_PARSE_STACK_OVERFLOW;
				stack[top] = {JSON_TAG_ARRAY, JsonValue(JSON_TAG_ARRAY, nullptr), nullptr};
				continue;
			case '{':
				if (++top == JSON_STACK_SIZE) return JSON_PARSE_STACK_OVERFLOW;
				stack[top] = {JSON_TAG_OBJECT, JsonValue(JSON_TAG_OBJECT, nullptr), nullptr};
				continue;
			case ':':
				if (separator || stack[top].key == nullptr) return JSON_PARSE_UNEXPECTED_CHARACTER;
				separator = true;
				continue;
			case ',':
				if (separator || stack[top].key != nullptr) return JSON_PARSE_UNEXPECTED_CHARACTER;
				separator = true;
				continue;
			default:
				return JSON_PARSE_UNEXPECTED_CHARACTER;
		}

		separator = false;

		if (top == -1)
		{
			*endptr = str;
			*value = o;
			return JSON_PARSE_OK;
		}

		if (stack[top].tag == JSON_TAG_OBJECT)
		{
			if (!stack[top].key)
			{
				if (o.getTag() != JSON_TAG_STRING) return JSON_PARSE_UNQUOTED_KEY;
				stack[top].key = o.toString();
				continue;
			}
			JsonNode *p = (JsonNode *)allocator.allocate(sizeof(JsonNode));
			p->value = o;
			p->key = stack[top].key;
			stack[top].key = nullptr;
			stack[top].grow_the_tail((JsonNode *)p);
			continue;
		}

		JsonNode *p = (JsonNode *)allocator.allocate(sizeof(JsonNode) - sizeof(char *));
		p->value = o;
		stack[top].grow_the_tail(p);
	}
	return JSON_PARSE_BREAKING_BAD;
}
