#include "maru_common.h"

int base64_decode(char *text, unsigned char *dst, int numBytes);
int base64_encode(char *text, int numBytes, char **encodedText);
