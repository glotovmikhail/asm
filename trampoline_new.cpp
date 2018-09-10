#include <sys/mman.h>
#include <iostream>
#include <vector>


using namespace std;

template<typename ... Args>
struct args;

template<typename T>
struct trampoline;

template<typename R, typename... Args>
void swap(trampoline<R(Args...)> &a, trampoline<R(Args...)> &b);

template<typename T, typename ... Args>
struct trampoline<T(Args ...)> {
private:
    static const int BYTE = 8;
    
    struct __attribute__((packed)) NotAlignedStorage {
        char c;
        void* funcPtr;
        void* empty;
    };

    void add(char *&p, const char *command) {
        for (const char *i = command; *i; i++)
            *(p++) = *i;
    }

    void add_p(char *&p, void *c) {
        *(void **) p = c;
        p += BYTE;
    }

    void *address = nullptr;
    const size_t PAGE_SIZE = 4096;
    const size_t SIZE_PER_FUNC = 255;

    void* allocate_page() {
        char* page = static_cast<char*>(mmap(nullptr, PAGE_SIZE, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
        if (page == MAP_FAILED)
            return nullptr;
        *page = 0;
        page += sizeof(int);
        for (size_t i = 0; i < PAGE_SIZE - sizeof(int); i += SIZE_PER_FUNC)
            *reinterpret_cast<char**>(page + i - (i == 0 ? 0 : SIZE_PER_FUNC)) = page + i;
        return page;
    }

    bool counter_add(void *page, int delta) {
        void *start_of_page = (void*)(((size_t)page >> 12) << 12);
        *static_cast<int*>(start_of_page) += delta;
        if (*static_cast<int*>(start_of_page) == 0) {
            munmap(start_of_page, PAGE_SIZE);
            return false;
        }
        return true;
    }

    void* my_malloc() {
        if (!address) {
            address = allocate_page();
            if (!address) {
                return nullptr;
            }
        }
        counter_add(address, 1);
        void *ret = address;
        address = *reinterpret_cast<void**>(address);
        return ret;
    }



    template<typename Function>
    static T caller(NotAlignedStorage storage, Args... args) {
        return (*static_cast<Function *>(storage.funcPtr))(std::forward<Args>(args)...);
    }

    template<typename Function>
    static void del(void *func) {
        delete static_cast<Function *>(func);
    }

    void *func;
    void *code;
    void (*deleter)(void *);
    T (*caller_)(NotAlignedStorage, Args...);


public:
    template<typename Function>
    trampoline(Function f) : func(new Function(std::move(f))), caller_(caller<Function>), deleter(del<Function>) {
        code = my_malloc();
        char *pointer_to_code = (char *) code;

        // sub rsp,16
        add (pointer_to_code, "\x48\x83\xEC\x10");
        //mov r11,byte
        add (pointer_to_code, "\x49\xBB");
        //setting obj adress to last command
        add_p (pointer_to_code, func);
        //mov [rsp + 1],r11
        add (pointer_to_code, "\x4C\x89\x5C\x24\x01");
        //mov r11,byte
        add (pointer_to_code, "\x49\xBB");
        //setting caller adress to last command
        add_p (pointer_to_code, (void *) caller_);
        //call r11
        add (pointer_to_code, "\x41\xFF\xD3");
        //add rsp,16
        add (pointer_to_code, "\x48\x83\xC4\x10");
        //ret
        add (pointer_to_code, "\xC3");
             
    }

    trampoline(trampoline &&other) : code(other.code), caller_(other.caller_), deleter(other.deleter), func(other.func) {
        other.func = nullptr;
    }

    trampoline(const trampoline &) = delete;

    template<class TRAMPOLINE>
    trampoline &operator=(TRAMPOLINE &&function) {
        trampoline tmp(std::move(function));
        swap(*this, tmp);
        return *this;
    }

    T (*get() const )(Args ... args) {
        return (T(*)(Args ... args))code;
    }

    void swap(trampoline &other) {
        ::swap(*this, other);
    }

    friend void::swap<>(trampoline &a, trampoline &b);

    ~trampoline() {
        if (func)
            deleter(func);
        *(void **) code = address;
        address = (void **) code;
    }
};

template<typename R, typename... Args>
void swap(trampoline<R(Args...)> &a, trampoline<R(Args...)> &b) {
    std::swap(a.func, b.func);
    std::swap(a.code, b.code);
    std::swap(a.deleter, b.deleter);
}

struct functional_object {
    int operator()(int c1, double c2, float c3, long c4, int c5) {
        cout << "in functional_object()\n";
        return 2345619;
    }
    ~functional_object() {
        cout << "functional_object deleted\n";
    }
};

void test() {
    int b = 42;
    functional_object fo;

    trampoline<int (int, double, float, long, int)> tr(fo);
    auto p = tr.get();
    p(1, 2.0, 3.0, 4, 5);
    b = 43;

    int res = p(2, 3.0, 4.0, 5, 6);
    cout << res << "\n";
    cout << b << "\n";
}


int main() {
    test();
    return 0;
}