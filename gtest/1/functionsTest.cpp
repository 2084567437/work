//functionsTest.cpp
#include "gtest/gtest.h"
#include "functions.h"

TEST(AddTest,AddTestCase){
        ASSERT_EQ(2,add(1,1));
}
TEST(MinusTest,MinusTestCase){
        ASSERT_EQ(10,myMinus(25,15));
}
TEST(MultiplyTest,MutilplyTestCase){
        ASSERT_EQ(13,multiply(3,4));
}
TEST(DivideTest,DivideTestCase){
        ASSERT_EQ(2,divide(7,3));
}
