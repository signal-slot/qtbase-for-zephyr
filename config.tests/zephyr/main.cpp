// Qt Zephyr configuration test
// This file tests if the Zephyr SDK and toolchain are properly configured

// Test basic C++ compilation
extern "C" {
    // Minimal entry point for freestanding environment
    void _start() {
        // Empty function - just test if we can compile
    }
}

// Test C++ features
class TestClass {
public:
    TestClass() : m_value(42) {}
    int getValue() const { return m_value; }
private:
    int m_value;
};

// Test basic template
template<typename T>
T add(T a, T b) {
    return a + b;
}

// Test if compiler supports basic C++11 features
static_assert(sizeof(int) == 4, "int must be 4 bytes");

// Global object to test static initialization
TestClass g_test;

// Test function
int testFunction() {
    TestClass obj;
    return obj.getValue() + add(1, 2);
}