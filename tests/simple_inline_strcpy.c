
void vuln(char* arg) {
    char buffer[16];
    char* p = buffer;
    while (*arg) {
        *p++ = *arg++;
    }
}

int main(int argc, char* argv[]) {
    vuln(argv[1]);
}
