
char* copy(char* d, char* s) {
    while (*s) {
        *d++ = *s++;
    }
    return d;
}

void vuln(char* arg) {
    char buffer[16];
    copy(buffer, arg);
}

int main(int argc, char* argv[]) {
    char str[32];
    for (int i = 0; i < 18; ++i) {
        str[i] = 'a';
    }
    str[18] = '\0';
    vuln(str);
}
