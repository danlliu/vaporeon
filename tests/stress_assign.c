
int main() {
    char buffer[16];
    char* a;
    char* b;
    char* c;
    a = buffer;
    for (int i = 0; i < 1000; ++i) {
        b = a;
        c = b;
        a = c;
    }
    a[16] = 'a';
}
