#include <iostream>
#include <emmintrin.h>

void asm_memcpy(char* dst, const char* src, size_t N) {

    size_t pos = 0;

    char symb;

    while ((size_t) (pos + dst) % 16 != 0 && pos < N) {
        symb = *(src + pos);
        *(dst + pos) = symb;
        pos++;
    }

    size_t chars_left = (N - pos) % 16;

    for (size_t i = pos; i < N - chars_left; i += 16) {
        __m128i temp;

        __asm__ volatile(
        "movdqu\t (%1), %0\n"
        "movntdq\t %0, (%2)\n"
        :"=x"(temp)
        :"r"(src + i), "r"(dst + i)
        );
    }

    for (size_t i = N - chars_left; i < N; i++) {
        *(dst + i) = *(src + i);
    }
}

int main() {

    char* a = new char[42];

    for (int i = 0; i < 13; ++i) {
        a[i] = 'x';
    }
    for (int i = 13; i < 20; ++i) {
        a[i] = ' ';
    }
    for (int i = 20; i < 42; ++i) {
        a[i] = '/';
    }

    char* b = new char[100];

    asm_memcpy(b, a, 42);

    bool is_correct = true;

    for (int i = 0; i < 42; ++i) {
        is_correct = is_correct && (a[i] == b[i]);
    }
    std::cout << b << '\n';
    std::cout << is_correct;
    return 0;
}