
int main() {
    char buffer[1024];

    for (int i = 0; i < 1000000000; ++i) {
        buffer[i % 1024] = 'a';
    }
}
