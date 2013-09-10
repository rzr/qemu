#include "maru_common.h"

int base64_decode(const char *text, unsigned char *dst, int numBytes);
int base64_encode(const char *text, int numBytes, char **encodedText);
