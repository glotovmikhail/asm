#include <iostream>
#include <cstddef>
#include <tmmintrin.h>
#include <xmmintrin.h>

using namespace std;

int classic_asm_word_count(const char* str, size_t size) {
    int ans = 0;
    bool is_space = true;

    for (size_t i = 0; i < size; i++) {
        if (is_space && str[i] != ' ') {
            ans++;
        }
        is_space = (str[i] == ' ');
    }

    return ans;
}

int asm_word_count(const char* str, size_t size) {

    if (size < (sizeof(__m128i) * 2) + 1) {
        return classic_asm_word_count(str, size);
    }

    bool is_space = false;
    size_t curr = 0;
    size_t ans = 0;

    while ((size_t)(str + curr) % 16 != 0) {
        if (is_space && (*(str + curr) != ' ')) {
            ans++;
        }
        is_space = (*(str + curr) == ' ');
        curr++;
    }

    if (*str != ' ') {
        ans++;
    }
    if (is_space && *(str + curr) != ' ' && curr != 0){
        ans++;
    }

    size_t aligned_end = size - (size - curr) % 16 - 16;
    __m128i storage = _mm_set1_epi8(0);
    __m128i spaces = _mm_cmpeq_epi8(_mm_load_si128((__m128i *) (str + curr)), _mm_set1_epi8(' ')); 

    for (size_t i = curr; i < aligned_end; i += sizeof(__m128i)) {
        __m128i curr_spaces_mask = spaces;
        spaces = _mm_cmpeq_epi8(_mm_load_si128((__m128i *) (str + i + 16)), _mm_set1_epi8(' '));

        __m128i shifted_spaces = _mm_alignr_epi8(spaces, curr_spaces_mask, 1);
        __m128i wordsbegings = _mm_andnot_si128(shifted_spaces, curr_spaces_mask);
        storage = _mm_adds_epu8(_mm_and_si128(_mm_set1_epi8(1), wordsbegings), storage);

        if (_mm_movemask_epi8(storage) != 0 || i + 16 >= aligned_end) {
            storage = _mm_sad_epu8(_mm_set1_epi8(0), storage);
            ans += _mm_cvtsi128_si32(storage);
            storage = _mm_srli_si128(storage, 8);
            ans += _mm_cvtsi128_si32(storage);
            storage = _mm_set1_epi8(0);
        }
    }

    curr = aligned_end;

    if ((*(str + curr - 1) == ' ') && (*(str + curr) != ' ')) {
        ans--;
    }

    is_space = *(str + curr - 1) == ' ';
    for (size_t i = curr; i < size; i++){
        char cur = *(str + i);
        if (is_space && cur != ' '){
            ans++;
        }
        is_space = (cur == ' ');
    }

    return ans;

}

int main() {

    const char* test_string = "";
    int length = 40;
    cout << asm_word_count(test_string, length);
    return 0;
}