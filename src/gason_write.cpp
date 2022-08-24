#include "gason_write.h"
#include <string.h>
#include <stdio.h>

struct Data {
	size_t bufferSize;
	char* buffer;
	size_t bufferOffset;
	size_t indentorLen;
	char* indentor;
	size_t indentLevel;
	bool writeMode;
};

static void jsonWrite_(const JsonValue& val, Data& data)
{
	auto dst = [&data]() { return data.buffer + data.bufferOffset; };

	auto pushChar = [&data](char c) {
		if (data.writeMode) {
			if (data.bufferOffset < data.bufferSize)
				data.buffer[data.bufferOffset] = c;
			else
				data.writeMode = false;
		}
		data.bufferOffset++;
	};

	auto pushStr = [&data, &dst](const char* str, size_t strLen) {
		if (data.writeMode) {
			const size_t remainingSpace = data.bufferSize - data.bufferOffset;
			if (strLen < remainingSpace)
				strncpy(dst(), str, strLen);
			else {
				strncpy(dst(), str, remainingSpace);
				data.writeMode = false;
			}
		}
		data.bufferOffset += strLen;
	};

	auto indent = [&data, &pushStr]() {
		for (size_t i = 0; i < data.indentLevel; i++)
			pushStr(data.indentor, data.indentorLen);
	};

	const JsonTag tag = val.getTag();
	if (tag == JSON_ARRAY || tag == JSON_OBJECT) {
		// open braquet
		pushChar(tag == JSON_ARRAY ? '[' : '{');

		if (val.toNode()) { // not empty array or object
			pushChar('\n');
			if (tag == JSON_ARRAY) {
				data.indentLevel++;
				for (auto it : val) {
					indent();
					jsonWrite_(it->value, data);
					if (it->next)
						pushChar(',');
					pushChar('\n');
				}
				data.indentLevel--;
			}
			else {
				data.indentLevel++;
				for (auto it : val) {
					indent();
					pushChar('\"');
					pushStr(it->key, strlen(it->key));
					pushStr("\": ", 3);
					jsonWrite_(it->value, data);
					if (it->next)
						pushChar(',');
					pushChar('\n');
				}
				data.indentLevel--;
			}
		}

		// close braquet
		indent();
		pushChar(tag == JSON_ARRAY ? ']' : '}');
	}
	else if (tag == JSON_NUMBER) {
		char str[64];
		size_t strLen = snprintf(str, sizeof(str), "%g", val.toNumber());
		pushStr(str, strLen);
	}
	else if (tag == JSON_STRING) {
		pushChar('\"');
		const char* str = val.toString();
		pushStr(str, strlen(str));
		pushChar('\"');
	}
	else if (tag == JSON_FALSE) {
		pushStr("false", 5);
	}
	else if (tag == JSON_TRUE) {
		pushStr("true", 4);
	}
	else if (tag == JSON_NULL) {
		pushStr("null", 4);
	}
}

size_t jsonWrite(const JsonValue& val, size_t bufferSize, char* buffer, char* indentor)
{
	Data data{ bufferSize, buffer, 0, strlen(indentor), indentor, 0 };
	if (data.buffer == nullptr) {
		data.writeMode = false;
		jsonWrite_(val, data);
		return data.bufferOffset + 1;
	}
	else {
		data.writeMode = true;
		jsonWrite_(val, data);

		// add null terminator
		if (data.bufferOffset < data.bufferSize)
			data.buffer[data.bufferOffset] = '\0';
		else
			data.buffer[data.bufferSize - 1] = '\0';
		data.bufferOffset++;
	}
}