
#include <stdio.h>

void my_strcpy(char* dest, const char* src) {
    while ((*dest++ = *src++));
}

void vuln(char* source) {
    char buffer[8];
    my_strcpy(buffer, source);
}

int main() {
    char buffer[16];
    for (int i = 0; i < 15; ++i) {
        buffer[i] = 'A';
    }
    buffer[15] = '\0';
    vuln(buffer);
}
