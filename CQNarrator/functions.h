#pragma once

#include <stdint.h>

size_t base64enc(const char *str, size_t strLen, char *base64str);
size_t base64dec(const char *base64str, char *str);
void getStatsuStr(const char *str1, const char *str2, uint8_t color, char *retstr);