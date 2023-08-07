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


int aaaa(int argc, char **argv) {

    printf("start test aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n");
    
    //int a = 1/0;
    UNITY_BEGIN();
    printf("start test aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n");
    UNITY_END();
    return 0;
}
