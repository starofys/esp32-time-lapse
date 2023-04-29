//
// Created by micro on 2023/4/15.
//
#include <unity.h>
#include <stdio.h>
void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}


int main(int argc, char **argv) {

    printf("start test\n");
    
    //int a = 1/0;
    UNITY_BEGIN();
    UNITY_END();
    return 0;
}
