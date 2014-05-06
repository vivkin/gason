#include "gason.h"
#include <ctype.h>
#include <stdlib.h>

#define JSON_ZONE_SIZE 4096
#define JSON_STACK_SIZE 32

JsonAllocator::~JsonAllocator() {
	while (head) {
		Zone *next = head->next;
		free(head);
		head = next;
	}
}

static inline void *alignPointer(void *p, size_t alignment) {
	return (void *)(((uintptr_t)p + (alignment - 1)) & ~(alignment - 1));
}

void *JsonAllocator::allocate(size_t size, size_t alignment) {
	if (head) {
		char *p = (char *)alignPointer(head->end, alignment);
		if (p + size <= (char *)head + JSON_ZONE_SIZE) {
			head->end = p + size;
			return p;
		}
	}
	size_t zoneSize = sizeof(Zone) + (size + (alignment - 1) & ~(alignment - 1));
	Zone *z = (Zone *)malloc(zoneSize <= JSON_ZONE_SIZE ? JSON_ZONE_SIZE : zoneSize);
	char *p = (char *)alignPointer(z + 1, alignment);
	z->end = p + size;
	if (zoneSize <= JSON_ZONE_SIZE || head == nullptr) {
		z->next = head;
		head = z;
	} else {
		z->next = head->next;
		head->next = z;
	}
	return p;
}

static inline bool isdelim(char c) {
	return isspace(c) || c == ',' || c == ':' || c == ']' || c == '}' || c == '\0';
}

static inline int char2int(char c) {
	if (c >= 'a')
		return c - 'a' + 10;
	if (c >= 'A')
		return c - 'A' + 10;
	return c - '0';
}

static double string2double(char *s, char **endptr) {
	char ch = *s;
	if (ch == '+' || ch == '-')
		++s;

	double result = 0;
	while (isdigit(*s))
		result = (result * 10) + (*s++ - '0');

	if (*s == '.') {
		++s;

		double fraction = 1;
		while (isdigit(*s))
			fraction *= 0.1, result += (*s++ - '0') * fraction;
	}

	if (*s == 'e' || *s == 'E') {
		++s;

		double base = 10;
		if (*s == '+')
			++s;
		else if (*s == '-') {
			++s;
			base = 0.1;
		}

		int exponent = 0;
		while (isdigit(*s))
			exponent = (exponent * 10) + (*s++ - '0');

		double power = 1;
		for (; exponent; exponent >>= 1, base *= base)
			if (exponent & 1)
				power *= base;

		result *= power;
	}

	*endptr = s;
	return ch == '-' ? -result : result;
}

static inline JsonNode *insertAfter(JsonNode *tail, JsonNode *node) {
	if (!tail)
		return node->next = node;
	node->next = tail->next;
	tail->next = node;
	return node;
}

static inline JsonValue listToValue(JsonTag tag, JsonNode *tail) {
	if (tail) {
		auto head = tail->next;
		tail->next = nullptr;
		return JsonValue(tag, head);
	}
	return JsonValue(tag, nullptr);
}

JsonParseStatus gasonParse(char *s, char **endptr, JsonValue *value, JsonAllocator &allocator) {
	struct {
		char *key;
		JsonNode *tail;
		JsonTag tag;
	} stack[JSON_STACK_SIZE];
	int top = -1;

	bool separator = true;

	while (*s) {
		JsonValue o;

		while (*s && isspace(*s))
			++s;

		*endptr = s++;
		switch (**endptr) {
		case '\0':
			continue;
		case '-':
			if (!isdigit(*s) && *s != '.') {
				*endptr = s;
				return JSON_PARSE_BAD_NUMBER;
			}
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
			o = JsonValue(string2double(*endptr, &s));
			if (!isdelim(*s)) {
				*endptr = s;
				return JSON_PARSE_BAD_NUMBER;
			}
			break;
		case '"':
			o = JsonValue(JSON_TAG_STRING, s);
			for (char *it = s; *s; ++it, ++s) {
				int c = *it = *s;
				if (c == '\\') {
					c = *++s;
					switch (c) {
					case '\\':
					case '"':
					case '/':
						*it = c;
						break;
					case 'b':
						*it = '\b';
						break;
					case 'f':
						*it = '\f';
						break;
					case 'n':
						*it = '\n';
						break;
					case 'r':
						*it = '\r';
						break;
					case 't':
						*it = '\t';
						break;
					case 'u':
						c = 0;
						for (int i = 0; i < 4; ++i) {
							if (!isxdigit(*++s)) {
								*endptr = s;
								return JSON_PARSE_BAD_STRING;
							}
							c = c * 16 + char2int(*s);
						}
						if (c < 0x80) {
							*it = c;
						} else if (c < 0x800) {
							*it++ = 0xC0 | (c >> 6);
							*it = 0x80 | (c & 0x3F);
						} else {
							*it++ = 0xE0 | (c >> 12);
							*it++ = 0x80 | ((c >> 6) & 0x3F);
							*it = 0x80 | (c & 0x3F);
						}
						break;
					default:
						*endptr = s;
						return JSON_PARSE_BAD_STRING;
					}
				} else if (c == '"') {
					*it = 0;
					++s;
					break;
				}
			}
			if (!isdelim(*s)) {
				*endptr = s;
				return JSON_PARSE_BAD_STRING;
			}
			break;
		case 't':
			for (const char *it = "rue"; *it; ++it, ++s) {
				if (*it != *s)
					return JSON_PARSE_BAD_IDENTIFIER;
			}
			if (!isdelim(*s))
				return JSON_PARSE_BAD_IDENTIFIER;
			o = JsonValue(JSON_TAG_BOOL, (void *)true);
			break;
		case 'f':
			for (const char *it = "alse"; *it; ++it, ++s) {
				if (*it != *s)
					return JSON_PARSE_BAD_IDENTIFIER;
			}
			if (!isdelim(*s))
				return JSON_PARSE_BAD_IDENTIFIER;
			o = JsonValue(JSON_TAG_BOOL, (void *)false);
			break;
		case 'n':
			for (const char *it = "ull"; *it; ++it, ++s) {
				if (*it != *s)
					return JSON_PARSE_BAD_IDENTIFIER;
			}
			if (!isdelim(*s))
				return JSON_PARSE_BAD_IDENTIFIER;
			break;
		case ']':
			if (top == -1)
				return JSON_PARSE_STACK_UNDERFLOW;
			if (stack[top].tag != JSON_TAG_ARRAY)
				return JSON_PARSE_MISMATCH_BRACKET;
			o = listToValue(JSON_TAG_ARRAY, stack[top--].tail);
			break;
		case '}':
			if (top == -1)
				return JSON_PARSE_STACK_UNDERFLOW;
			if (stack[top].tag != JSON_TAG_OBJECT)
				return JSON_PARSE_MISMATCH_BRACKET;
			o = listToValue(JSON_TAG_OBJECT, stack[top--].tail);
			break;
		case '[':
			if (++top == JSON_STACK_SIZE)
				return JSON_PARSE_STACK_OVERFLOW;
			stack[top] = {nullptr, nullptr, JSON_TAG_ARRAY};
			continue;
		case '{':
			if (++top == JSON_STACK_SIZE)
				return JSON_PARSE_STACK_OVERFLOW;
			stack[top] = {nullptr, nullptr, JSON_TAG_OBJECT};
			continue;
		case ':':
			if (separator || stack[top].key == nullptr)
				return JSON_PARSE_UNEXPECTED_CHARACTER;
			separator = true;
			continue;
		case ',':
			if (separator || stack[top].key != nullptr)
				return JSON_PARSE_UNEXPECTED_CHARACTER;
			separator = true;
			continue;
		default:
			return JSON_PARSE_UNEXPECTED_CHARACTER;
		}

		separator = false;

		if (top == -1) {
			*endptr = s;
			*value = o;
			return JSON_PARSE_OK;
		}

		if (stack[top].tag == JSON_TAG_OBJECT) {
			if (!stack[top].key) {
				if (o.getTag() != JSON_TAG_STRING)
					return JSON_PARSE_UNQUOTED_KEY;
				stack[top].key = o.toString();
				continue;
			}
			JsonNode *p = (JsonNode *)allocator.allocate(sizeof(JsonNode));
			p->value = o;
			p->key = stack[top].key;
			stack[top].key = nullptr;
			stack[top].tail = insertAfter(stack[top].tail, p);
			continue;
		}

		JsonNode *p = (JsonNode *)allocator.allocate(sizeof(JsonNode) - sizeof(char *));
		p->value = o;
		stack[top].tail = insertAfter(stack[top].tail, p);
	}
	return JSON_PARSE_BREAKING_BAD;
}
