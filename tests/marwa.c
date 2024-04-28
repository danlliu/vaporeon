
char* copy(char* d, char* s) {
    while (*s) {
        *d++ = *s++;
    }
    return d;
}

void vuln(char* s, char* s2) {
    char buffer[16];
    copy(buffer, s);
    copy(buffer + 8, s2);
}

int main() {
    char marwa[8];
    marwa[0] = 'm';
    marwa[1] = 'a';
    marwa[2] = 'r';
    marwa[3] = 'w';
    marwa[4] = 'a';
    marwa[5] = 0; 
    char aaaaaaaa[16];
    for (int i = 0; i < 15; ++i) {
        aaaaaaaa[i] = 'a';
    }
    aaaaaaaa[15] = '\0';
    vuln(marwa, aaaaaaaa);
}
