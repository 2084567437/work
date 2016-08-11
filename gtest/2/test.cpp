#include <gtest/gtest.h>

class FooCalcTest:public testing::Test
{
protected:

    virtual void SetUp()

    {

        m_foo.Init();

    }

    virtual void TearDown()

    {

        m_foo.Finalize();

    }
	FooCalcTest  m_foo;

};



TEST_F(FooCalcTest, HandleNoneZeroInput)

{

    EXPECT_EQ(4, m_foo.Calc(12, 16));

}



TEST_F(FooCalcTest, HandleNoneZeroInput_Error)

{

    EXPECT_EQ(5, m_foo.Calc(12, 16));

}
