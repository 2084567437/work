/*
 * ASyncCouchbase.h
 *
 *  Created on: Aug 22, 2013
 *      Author: jim
 */

#ifndef ASYNCCOUCHBASE_H_
#define ASYNCCOUCHBASE_H_

#include <libcouchbase/couchbase.h>
#include <libcouchbase/vbucket.h>
#include <arpa/inet.h>
#include <string>
#include <map>
#include "jlog.h"

using namespace std;

class ASyncCouchbase;

struct ASyncCBInstanceCookie
{
    ASyncCouchbase* _ASyncCouchbaseInstance;
    const void *_cookie;
};

struct ASyncCBGetCookie
{
    unsigned int hostIp;
    const void *cookie;
};



class ASyncCouchbase
{
public:
	typedef void (*GetCallbackFunc)(lcb_t instance, const void *cookie,
			lcb_error_t error, const lcb_get_resp_t *item);
	typedef void (*SetCallbackFunc)(lcb_t instance, const void *cookie,
			lcb_storage_t operation, lcb_error_t error,
			const lcb_store_resp_t *item);
	typedef void (*IncrCallbackFunc)(lcb_t instance, const void * cookie,
			lcb_error_t error, const lcb_arithmetic_resp_t *resp);
	typedef void (*RemoveCallbackFunc)(lcb_t instance, const void *cookie,
			lcb_error_t error, const lcb_remove_resp_t *resp);
	typedef void (*TouchCallbackFunc)(lcb_t instance, const void *cookie,
	        lcb_error_t error, const lcb_touch_resp_t *item);
	typedef void (*ErrorCallbackFunc)(lcb_t instance, lcb_error_t error);
public:
	ASyncCouchbase();
	virtual ~ASyncCouchbase();

    void initLog(int* logLv, MyLog* log)
    {
        _logLv__ = logLv;
        _log__ = log;
    }

	void setCookie(const void* cookie);
    static const void *getCookie(lcb_t instance);
	bool init(std::string hosts, std::string user, std::string pwd,
			std::string bucket);
	bool connect(unsigned int timeout = 10000);
	bool disconnect();

    int getCurWeight(unsigned int ip);
    static int getMaxWeight();

	// Get
	bool get(char* key, const void *cookie = NULL);
	bool get(char* key, int len, const void *cookie = NULL);
	void setGetCallback(GetCallbackFunc callback);

	// Set
	bool set(char* key, char* val, time_t exptime = 0, const void *cookie =
			NULL, bool json = false, lcb_storage_t operation = LCB_SET);
	bool set(char* key, int klen, char* val, int vlen, time_t exptime = 0,
			const void *cookie = NULL, bool json = false,
			lcb_storage_t operation = LCB_SET);
	void setSetCallback(SetCallbackFunc callback);

	// Incr
	bool incr(const char* key, time_t exptime = 0, const void* cookie = NULL,
			int64_t delta = 1, uint64_t initial = 0);
	bool incr(const char* key, int len, time_t exptime = 0, const void* cookie =
			NULL, int64_t delta = 1, uint64_t initial = 0);
	void setIncrCallback(IncrCallbackFunc callback);

	// Remove
	bool remove(const char* key, const void* cookie = NULL);
	bool remove(const char* key, int len, const void* cookie = NULL);
	void setRemoveCallback(RemoveCallbackFunc callback);

	//update ttl
	bool touch(char* key, time_t exptime = 0, const void *cookie = NULL);
	bool touch(char* key, int klen, time_t exptime = 0, const void *cookie = NULL);
	void setTouchCallback(TouchCallbackFunc callback);

	// Error callback
	void setErrorCallback(ErrorCallbackFunc callback);

	// Wait
	bool wait();
	void setTickWait(bool isTickwait);
	bool getTickWait();
    bool hostForKey(const void *key, size_t nkey, unsigned int &ip);

private:
	void destroyCB();
    void initCongestionCounter();
    void updateCongestionCounter();
    void IncreaseCongestionCounter(unsigned int hostIp, int step);
    bool isTargetHostInCongestion(unsigned int hostIp);

    static void OnGetCallbackWrap(lcb_t instance, const void *cookie,
            lcb_error_t error, const lcb_get_resp_t *item);
    void OnGetCallback(lcb_t instance, const void *cookie,
            lcb_error_t error, const lcb_get_resp_t *item);
    static void OnIncrCallbackWrap(lcb_t instance, const void *cookie,
            lcb_error_t error, const lcb_arithmetic_resp_t *item);
    void OnIncrCallback(lcb_t instance, const void *cookie,
            lcb_error_t error, const lcb_arithmetic_resp_t *item);
    static void OnSetCallbackWrap(lcb_t instance, const void *cookie, lcb_storage_t operation,
            lcb_error_t error, const lcb_store_resp_t *item);
    void OnSetCallback(lcb_t instance, const void *cookie,lcb_storage_t operation,
            lcb_error_t error, const lcb_store_resp_t *item);
    bool readyToRequest(const char *key, int len, ASyncCBGetCookie*& cookieWrap, const void *cookie);
    bool dispatchGet(char* key, int len, const void *cookie);
    bool dispatchIncr(const char* key, int len, time_t exptime = 0, const void* cookie =
            NULL, int64_t delta = 1, uint64_t initial = 0);
    bool dispatchSet(char* key, int len, char* val, int vlen, time_t exptime = 0,
            const void *cookie = NULL, bool json = false,
            lcb_storage_t operation = LCB_SET);

    GetCallbackFunc _getCallBack;
    SetCallbackFunc _setCallBack;
    IncrCallbackFunc _incrCallBack;
    std::map<unsigned int, int> _congestionCounter;

	lcb_t _instance;
	struct lcb_create_st _create_options;
	struct lcb_create_io_ops_st _io_opts;
    ASyncCBInstanceCookie _instanceCookie;

	std::string _hosts;
	std::string _user;
	std::string _pwd;
	std::string _bucket;

	bool _isTickwait;
    lcbvb_CONFIG *_vbc;
	bool _isUpdating;
    int*_logLv__;
    MyLog* _log__;
};

#endif /* ASYNCCOUCHBASE_H_ */
