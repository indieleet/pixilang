#include <stdio.h>

extern "C" __declspec(dllexport) __stdcall float test_function1(
    int p1,
    float p2,
    int p3,
    float p4,
    int p5,
    double p6,
    int p7,
    float p8,
    char p9,
    short p10,
    int p11,
    float p12,
    int p13,
    float p14,
    int p15,
    double p16
)
{
    printf( "%d\n%f\n%d\n%f\n%d\n%f\n%d\n%f\n%d\n%d\n", p1, p2, p3, p4, p5, p6, p7, p8, p9, p10 );
    printf( "%d\n%f\n%d\n%f\n%d\n%f\n", p11, p12, p13, p14, p15, p16 );
    return 3.3;
}

extern "C" __declspec(dllexport) __stdcall float test_function2( int p1, float p2 )
{
    return p1 + p2;
}

extern "C" __declspec(dllexport) __stdcall const char* test_function3( int* ptr )
{
    printf( "ptr[ 0 ] = %d\n", ptr[ 0 ] );
    printf( "ptr[ 1 ] = %d\n", ptr[ 1 ] );
    return "Some text from native library";
}
