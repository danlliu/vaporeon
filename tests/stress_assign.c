
#include <stdio.h>

void print_first_char(char* p) {
    char c = p[0];
    putchar(c);
}

int main() {
    char buffer[16];
    buffer[0] = '!';
    char* a;
    char* b;
    char* c;
    a = buffer;
    for (int i = 0; i < 100000000; ++i) {
        b = a;
        c = b;
        a = c;
    }
    a[16] = 'a';
    print_first_char(a);
}
