#include <gtest/gtest.h>
#include <string>
#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>


class CSingleCouchbaseTest : public testing::Test
{
public:
    static void SetUpTestCase() 
    {
    	using namespace boost::property_tree;
    	ptree pt;
    	read_xml("CSingleCouchbaseTest.conf", pt);

    	std::string hosts = pt.get<std::string>("conf.hosts");

    	std::string user = pt.get<std::string>("conf.user");
    	std::string passwd = pt.get<std::string>("conf.passwd");
    	std::string bucket = pt.get<std::string>("conf.bucket");
    }

    static void TearDownTestCase()
    {
    }

    static int a;
};


TEST_F(CSingleCouchbaseTest, GetValue)
{
	int uid = 0;
	int mid = 0;

	std::string key, val;
    {
	    std::stringstream ss;
	    ss << uid << ":" << mid;
	    key = ss.str();
    }

    {
	    std::stringstream ss;
	    ss << "{\"tuid\":" << uid << "," << "\"mid\":" << mid << "}";
	    val = ss.str();
    }

	char result[256] = {0};
	int ret;
	//ddint ret = _csc.SetValue((char*)key.c_str(), (char*)val.c_str(), 300, true);
	ASSERT_EQ(0, ret) << "SetValue error with ret[" << ret << "].";

	int len = sizeof(result);
	//ret = _csc.GetValue((unsigned char*)key.c_str(), key.size(), (unsigned char*)result, &len);
	ASSERT_EQ(0, ret) << "GetValue error with ret[" << ret << "].";
	ASSERT_GT(len, 0) << "GetValue error with ret len[" << len << "].";
	ASSERT_STREQ(val.c_str(), result) << "Key[" << key << "]";
}

