#include "stdafx.h"
#include "functions.h"

void getStatsuStr(const char *str1, const char *str2, uint8_t color, char *retstr) {
	//数据大小（大头）+文本+数据大小+文本+颜色代码（32bit），base64
	char data[32];
	uint16_t len1 = (uint16_t)strlen(str1), len2 = (uint16_t)strlen(str2);
	int lenall = sprintf(data, "%c%c%s%c%c%s%c%c%c%c", len1 >> 8, len1 & 0xFF, str1, len2 >> 8, len2 & 0xFF, str2, 0, 0, 0, color);
	base64enc(data, lenall, retstr);
}

size_t base64enc(const char *str, size_t len, char *base64str) {
	static char base64pattern[64] =
	{ 'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
		'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/' };
	uint32_t  buffer = 0;
	size_t counter = 0, i;
	for (i = 0; i < len; i++) {
		switch (i % 3) {
		case 0:
			buffer = str[i] << 16;
			break;
		case 1:
			buffer |= str[i] << 8;
			break;
		case 2:
			buffer |= str[i];
			//out
			base64str[counter++] = base64pattern[buffer >> 18];
			base64str[counter++] = base64pattern[(buffer >> 12) & 0x3f];
			base64str[counter++] = base64pattern[(buffer >> 6) & 0x3f];
			base64str[counter++] = base64pattern[buffer & 0x3F];
		}
	}
	//last words
	switch (i % 3) {
	case 1:
		base64str[counter++] = base64pattern[buffer >> 18];
		base64str[counter++] = base64pattern[(buffer >> 12) & 0x3f];
		base64str[counter++] = '=';
		base64str[counter++] = '=';
		break;
	case 2:
		//out
		base64str[counter++] = base64pattern[buffer >> 18];
		base64str[counter++] = base64pattern[(buffer >> 12) & 0x3f];
		base64str[counter++] = base64pattern[(buffer >> 6) & 0x3f];
		base64str[counter++] = '=';
	default:;
	}
	base64str[counter++] = '\0';
	return counter;
}
