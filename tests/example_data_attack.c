
#include <stdio.h>

int main(int argc, char** argv) {
    char buf[16];
    char buf2[16];
    buf[0] = 1;
    buf2[0] = 3;
    if (argc != 1) buf2[16] = 8;
    if (buf[0] != 1) {
        puts("this is a restricted area!");
    } else {
        puts("normal operation");
    }
}
