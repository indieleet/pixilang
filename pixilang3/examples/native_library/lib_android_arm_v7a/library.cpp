#include <stdio.h>
#include <android/log.h>

char temp_buf[ 1024 ];

extern "C" float test_function1(
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
    sprintf( temp_buf, "%d\n%f\n%d\n%f\n%d\n%f\n%d\n%f\n%d\n%d\n", p1, p2, p3, p4, p5, p6, p7, p8, p9, p10 );
    __android_log_print( ANDROID_LOG_INFO, "native-activity", temp_buf );
    sprintf( temp_buf, "%d\n%f\n%d\n%f\n%d\n%f\n", p11, p12, p13, p14, p15, p16 );
    __android_log_print( ANDROID_LOG_INFO, "native-activity", temp_buf );
    return 3.3;
}

extern "C" float test_function2( int p1, float p2 )
{
    return p1 + p2;
}

extern "C" const char* test_function3( int* ptr )
{
    sprintf( temp_buf, "ptr[ 0 ] = %d\n", ptr[ 0 ] );
    __android_log_print( ANDROID_LOG_INFO, "native-activity", temp_buf );
    sprintf( temp_buf, "ptr[ 1 ] = %d\n", ptr[ 1 ] );
    __android_log_print( ANDROID_LOG_INFO, "native-activity", temp_buf );
    return "Some text from native library";
}
