/*
 * ASyncCouchbase.cpp
 *
 *  Created on: Aug 22, 2013
 *      Author: jim
 */
#include <stdio.h>
#include <ASyncCouchbase.h>
#include "CBWorkQueueThread.h"

#define CONGESTIONCOUNTER (500)

#ifdef LOG_OUT
#undef LOG_OUT
#endif

#define LOG_OUT(lv, fmt, args...)   {   \
    if(_log__ && _logLv__ && lv <= *_logLv__)     {   \
        _log__->Log_Msg("%s:%d[%d %u %0x](%s): " fmt, __FILE__, __LINE__, getpid(), Tools::gettid(), pthread_self(), __FUNCTION__ , ## args);    \
    }   \
}

ASyncCouchbase::ASyncCouchbase():_vbc(NULL), _logLv__(NULL), _log__(NULL)
{
	bzero(&_io_opts,sizeof(_io_opts));
	_io_opts.version = 0;
	_io_opts.v.v0.type = LCB_IO_OPS_DEFAULT;
	_io_opts.v.v0.cookie = NULL;

	_instance = NULL;

	_isTickwait = false;
}

ASyncCouchbase::~ASyncCouchbase()
{
	destroyCB();
}

void ASyncCouchbase::destroyCB()
{
	if (_instance)
	{
		lcb_destroy(_instance);
		_instance = NULL;
		lcb_destroy_io_ops(_create_options.v.v0.io);	
	}
}

bool ASyncCouchbase::disconnect()
{
	destroyCB();
}

//unit is mill-second
bool ASyncCouchbase::connect(unsigned int timeout)
{
	lcb_error_t error;
	//trans to usecond!
	lcb_U32 utimeout = timeout * 1000;
    lcb_set_cookie(_instance, (const void *)&_instanceCookie);
	error = lcb_cntl(_instance, LCB_CNTL_SET, LCB_CNTL_OP_TIMEOUT, &utimeout);
	if(LCB_SUCCESS != error )
	{
        LOG_OUT(LOG_PRIORITY_FATAL,"Failed to set CB timeout, error[%d:%s]"
                , error, lcb_strerror(_instance, error));
        destroyCB();
	    return false;
	}

    //set errorThresh to more than congestioncounter*4
    //it means that the error count won't reach to the throttle if less than 4 nodes being down in one period
    int errorThresh = CONGESTIONCOUNTER*4 + 1;
    error = lcb_cntl(_instance, LCB_CNTL_SET,  LCB_CNTL_CONFERRTHRESH, &errorThresh);
    if(LCB_SUCCESS != error )
    {
        LOG_OUT(LOG_PRIORITY_FATAL,"Failed to set CB LCB_CNTL_CONFERRTHRESH, error[%d:%s]"
                , error, lcb_strerror(_instance, error));
        destroyCB();
        return false;
    }

	error = lcb_connect(_instance);
	if (error == LCB_SUCCESS)
	{
		lcb_wait(_instance);
		error = lcb_get_bootstrap_status(_instance);
		if (error == LCB_SUCCESS)
		{
            lcb_cntl(_instance, LCB_CNTL_GET, LCB_CNTL_VBCONFIG, &_vbc);
			initCongestionCounter();
			return true;
		}
	}

    LOG_OUT(LOG_PRIORITY_FATAL,"Failed to connect to couchbase, hosts[%s] bucket[%s] error[%d:%s]\n",
			_hosts.c_str(), _bucket.c_str(), error,
			lcb_strerror(_instance, error));

	destroyCB();

	return false;
}

bool ASyncCouchbase::init(std::string hosts, std::string user, std::string pwd,
		std::string bucket)
{
	_hosts = hosts;
	_user = user;
	_pwd = pwd;
	_bucket = bucket;

	lcb_error_t error;
	memset(&_create_options, 0, sizeof(_create_options));
	error = lcb_create_io_ops(&_create_options.v.v0.io, &_io_opts);
	if (error == LCB_SUCCESS)
	{
		_create_options.v.v0.host = _hosts.c_str();
		_create_options.v.v0.user = _user.c_str();
		_create_options.v.v0.passwd = _pwd.c_str();
		_create_options.v.v0.bucket = _bucket.c_str();

		error = lcb_create(&_instance, &_create_options);
		if (error == LCB_SUCCESS)
		{
			return true;
		}
		else
		{
			std::cout<<"error:"<<lcb_strerror(NULL, error)<<"\n";
		}
	}

	destroyCB();

    LOG_OUT(LOG_PRIORITY_RUN,"Failed to init instance of couchbase, hosts[%s] bucket[%s] error[%d:%s]\n",
			hosts.c_str(), bucket.c_str(), error, lcb_strerror(NULL, error));
	return false;
}

void ASyncCouchbase::setCookie(const void* cookie)
{
    _instanceCookie._ASyncCouchbaseInstance = this;
    _instanceCookie._cookie = (void*) cookie;
}

const void* ASyncCouchbase::getCookie(lcb_t instance)
{
    ASyncCBInstanceCookie* instanceCookie =  (ASyncCBInstanceCookie*)lcb_get_cookie(instance);
    return instanceCookie->_cookie;
}


int ASyncCouchbase::getCurWeight(unsigned int ip)
{
    int ret = 0x7fffffff;
	if(_isUpdating)
		return ret;
	if(_congestionCounter.find(ip) != _congestionCounter.end())
		return _congestionCounter[ip];
    return ret;
}

int ASyncCouchbase::getMaxWeight()
{
    return CONGESTIONCOUNTER;
}

bool ASyncCouchbase::get(char* key, const void *cookie)
{
	if (key)
	{
        ASyncCBGetCookie* cookieWrap = NULL;
        if(_isTickwait)
        {
            if(readyToRequest(key, strlen(key), cookieWrap, cookie))
                return dispatchGet(key, strlen(key), cookieWrap);
        }
        else
            return dispatchGet(key, strlen(key), cookie);
    }

	return false;
}

bool ASyncCouchbase::get(char* key, int len, const void *cookie)
{
    if (key)
    {
        ASyncCBGetCookie* cookieWrap = NULL;
        if(_isTickwait)
        {
            if(readyToRequest(key, len, cookieWrap, cookie))
                return dispatchGet(key, len, cookieWrap);
        }
        else
            return dispatchGet(key, len, cookie);
    }

    return false;
}

bool ASyncCouchbase::readyToRequest(const char* key, int len, ASyncCBGetCookie*& cookieWrap, const void* cookie)
{
    bool ret = false;
    if (key)
    {
        unsigned int ip = 0;
        if(hostForKey(key, len, ip))
        {
            if(!isTargetHostInCongestion(ip))
            {
                cookieWrap = new ASyncCBGetCookie;
                cookieWrap->hostIp = ip;
                cookieWrap->cookie = cookie;
                IncreaseCongestionCounter(ip, 1);
                ret = true;
            }
            else
                LOG_OUT(LOG_PRIORITY_DEBUG,"key:%s server:%d is in congestion", key, ip);
        }
    }
    return ret;
}

bool ASyncCouchbase::dispatchGet(char* key, int len, const void* cookie)
{
	if (key && len > 0)
	{
		lcb_error_t error;
		lcb_get_cmd_t cmd;

		const lcb_get_cmd_t *commands[] =
		{ &cmd };

		memset(&cmd, 0, sizeof(cmd));
		cmd.v.v0.key = key;
		cmd.v.v0.nkey = len;
        error = lcb_get(_instance, (const void*)cookie, sizeof(commands)
				/ sizeof(commands[0]), commands);
		if (error == LCB_SUCCESS)
		{
			return true;
		}
        LOG_OUT(LOG_PRIORITY_FATAL,"lcb_get, error[%s]\n", lcb_strerror(_instance, error));
	}

	return false;
}

bool ASyncCouchbase::dispatchIncr(const char* key, int len, time_t exptime, const void *cookie
                                  , int64_t delta, uint64_t initial)
{
    if (key && len > 0)
    {
        lcb_error_t error;
        lcb_arithmetic_cmd_t cmd(key, len, delta, 1, initial, exptime);
        lcb_arithmetic_cmd_t* commands[] =
        { &cmd };
        error = lcb_arithmetic(_instance, cookie, sizeof(commands)
                / sizeof(commands[0]), commands);
        if (error == LCB_SUCCESS)
        {
            return true;
        }

        LOG_OUT(LOG_PRIORITY_FATAL,"incr key[%s] error[%s], errno[%d]\n", key, lcb_strerror(
                _instance, error), error);
    }

    return false;
}

bool ASyncCouchbase::dispatchSet(char* key, int len, char* val, int vlen, time_t exptime,
        const void *cookie, bool json,
        lcb_storage_t operation)
{
    if (key && val && (len > 0) && (vlen > 0))
    {
        lcb_store_cmd_t cmd;
        memset(&cmd, 0, sizeof(cmd));

        const lcb_store_cmd_t *commands[] =
        { &cmd };
        cmd.v.v0.operation = operation;
        cmd.v.v0.key = key;
        cmd.v.v0.nkey = len;
        cmd.v.v0.bytes = val;
        cmd.v.v0.nbytes = vlen;
        cmd.v.v0.exptime = exptime;

        if (json)
            cmd.v.v0.datatype = 1;

        lcb_error_t error;
        error = lcb_store(_instance, cookie, sizeof(commands)
                / sizeof(commands[0]), commands);
        if (error == LCB_SUCCESS)
        {
            return true;
        }
        LOG_OUT(LOG_PRIORITY_FATAL,"lcb_store, error[%s]\n", lcb_strerror(_instance, error));
    }

    return false;

}

void ASyncCouchbase::setGetCallback(GetCallbackFunc callback)
{
    _getCallBack = callback;
    lcb_set_get_callback(_instance, OnGetCallbackWrap);
}

bool ASyncCouchbase::set(char* key, char* val, time_t exptime,
		const void* cookie, bool json, lcb_storage_t operation)
{

    if (key && val)
    {
        ASyncCBGetCookie* cookieWrap = NULL;
        if(_isTickwait)
        {
            if(readyToRequest(key, strlen(key), cookieWrap, cookie))
                return dispatchSet(key, strlen(key), val, strlen(val), exptime, cookieWrap, json,
                        operation);
        }
        else
            return dispatchSet(key, strlen(key), val, strlen(val), exptime, cookie, json,
                    operation);

    }
	return false;
}

bool ASyncCouchbase::set(char* key, int klen, char* val, int vlen,
		time_t exptime, const void* cookie, bool json, lcb_storage_t operation)
{
    if (key && val)
    {
        ASyncCBGetCookie* cookieWrap = NULL;
        if(_isTickwait)
        {
            if(readyToRequest(key, strlen(key), cookieWrap, cookie))
                return dispatchSet(key, klen, val, vlen, exptime, cookieWrap, json,
                        operation);
        }
        else
            return dispatchSet(key, klen, val, vlen, exptime, cookie, json,
                    operation);
    }
	return false;
}

void ASyncCouchbase::setSetCallback(SetCallbackFunc callback)
{
    _setCallBack = callback;
    lcb_set_store_callback(_instance, OnSetCallbackWrap);
}

bool ASyncCouchbase::incr(const char* key, int len, time_t exptime,
		const void* cookie, int64_t delta, uint64_t initial)
{
    if (key)
    {
        ASyncCBGetCookie* cookieWrap = NULL;
        if(_isTickwait)
        {
            if(readyToRequest(key, strlen(key), cookieWrap, cookie))
                return dispatchIncr(key, len, exptime, cookieWrap, delta, initial);
        }
        else
            return dispatchIncr(key, len, exptime, cookie, delta, initial);
    }
	return false;
}

bool ASyncCouchbase::incr(const char* key, time_t exptime, const void* cookie,
		int64_t delta, uint64_t initial)
{
    if (key)
    {
        ASyncCBGetCookie* cookieWrap = NULL;
        if(_isTickwait)
        {
            if(readyToRequest(key, strlen(key), cookieWrap, cookie))
                return dispatchIncr(key, strlen(key), exptime, cookieWrap, delta, initial);
        }
        else
            return dispatchIncr(key, strlen(key), exptime, cookie, delta, initial);

    }
	return false;
}

void ASyncCouchbase::setIncrCallback(IncrCallbackFunc callback)
{
    _incrCallBack = callback;
    lcb_set_arithmetic_callback(_instance, OnIncrCallbackWrap);
}

bool ASyncCouchbase::remove(const char* key, const void* cookie)
{
	if (key)
	{
		return remove(key, strlen(key), cookie);
	}

	return false;
}

bool ASyncCouchbase::remove(const char* key, int len, const void* cookie)
{
	if (key && len > 0)
	{
		lcb_error_t error;
		lcb_remove_cmd_t cmd(key, len);
		lcb_remove_cmd_t* commands[] =
		{ &cmd };
		error = lcb_remove(_instance, cookie, sizeof(commands)
				/ sizeof(commands[0]), commands);
		if (error == LCB_SUCCESS)
		{
			return true;
		}

        LOG_OUT(LOG_PRIORITY_FATAL,"remove key[%s] error[%s], errno[%d]\n", key, lcb_strerror(
				_instance, error), error);
	}

	return false;
}

void ASyncCouchbase::setRemoveCallback(RemoveCallbackFunc callback)
{
	lcb_set_remove_callback(_instance, callback);
}

void ASyncCouchbase::setErrorCallback(ErrorCallbackFunc callback)
{
    lcb_set_bootstrap_callback(_instance, callback);
}

bool ASyncCouchbase::touch(char* key, time_t exptime, const void *cookie )
{
    if (NULL != key)
    {
        return touch(key, strlen(key), exptime, cookie);
    }

    return false;
}

bool ASyncCouchbase::touch(char* key, int klen, time_t exptime, const void *cookie )
{
    if ((NULL != key) && klen > 0)
    {
        lcb_touch_cmd_t touch ;
        memset(&touch, 0, sizeof(touch));
        const lcb_touch_cmd_t *commands[] = {&touch};
        touch.v.v0.key = key;
        touch.v.v0.nkey = klen;
        touch.v.v0.exptime = exptime;
        lcb_error_t error = lcb_touch(_instance, cookie, sizeof(commands)
                /sizeof(commands[0]), commands);
        if (error == LCB_SUCCESS)
        {
            return true;
        }
        LOG_OUT(LOG_PRIORITY_FATAL,"lcb_touch key[%s], error[%s]\n", key, lcb_strerror(_instance, error));
    }

    return false;
}

void ASyncCouchbase::setTouchCallback(TouchCallbackFunc callback)
{
    lcb_set_touch_callback(_instance, callback);
}

bool ASyncCouchbase::wait()
{
	lcb_error_t error;
	if(_isTickwait)
	{
		error = lcb_tick_nowait(_instance);
	}
	else
	{
		error = lcb_wait(_instance);
	}

	if (error == LCB_SUCCESS)
	{
		return true;
	}

    LOG_OUT(LOG_PRIORITY_FATAL,"Failed to do lcb_wait, error[%s], errno[%d]\n", lcb_strerror(
			_instance, error), error);
	return false;
}

void ASyncCouchbase::setTickWait(bool isTickwait)
{
	_isTickwait = isTickwait;
}

bool ASyncCouchbase::getTickWait()
{
	return _isTickwait;
}

bool ASyncCouchbase::hostForKey(const void *key, size_t nkey, unsigned int& ip)
{
    int vbid, server_index;
	lcb_cntl(_instance, LCB_CNTL_GET, LCB_CNTL_VBCONFIG, &_vbc);
    if(!_vbc)
        return false;

    lcbvb_map_key(_vbc, key, nkey, &vbid, &server_index);

    if (server_index < 0) {
        return false;
    }

    // Find the server:
    const char *server = lcb_get_node(_instance, LCB_NODE_DATA, server_index);
	if(server == NULL || strcmp(server,LCB_GETNODE_UNAVAILABLE) == 0)
    {
        LOG_OUT(LOG_PRIORITY_FATAL,"mapping key %s error invalid host", key);
        return false;
    }
    const char *endp = strchr(server, ':');
    if(endp == NULL)
    {
        LOG_OUT(LOG_PRIORITY_FATAL,"mapping key %s  error %s", key, server);
        return false;
    }
    std::string host(server, endp);
    in_addr_t addr = inet_addr(host.c_str());
    ip = addr;
    return true;
}

void ASyncCouchbase::initCongestionCounter()
{
	_isUpdating = true;
    int serverNum = lcb_get_num_nodes(_instance);
    for(int ii=0; ii<serverNum; ii++)
    {
        const char* server = lcb_get_node(_instance, LCB_NODE_DATA, ii);
		if(server == NULL || strcmp(server,LCB_GETNODE_UNAVAILABLE) == 0)
		{
            LOG_OUT(LOG_PRIORITY_FATAL,"init get node error ");
            continue;
		}
		const char *endp = strchr(server, ':');
        if(endp == NULL)
        {
            LOG_OUT(LOG_PRIORITY_FATAL,"get node format error %s", server);
            continue;
        }
        std::string host(server, endp);
		in_addr_t addr = inet_addr(host.c_str());
        LOG_OUT(LOG_PRIORITY_RUN,"get cluster server index:%d host:%s", ii, host.c_str());
        _congestionCounter[addr] = 0;
    }
	_isUpdating = false;
}

void ASyncCouchbase::updateCongestionCounter()
{
	_isUpdating = true;
    int serverNum = lcb_get_num_nodes(_instance);
    for(int ii=0; ii<serverNum; ii++)
    {
        const char* server = lcb_get_node(_instance, LCB_NODE_DATA, ii);
		if(server == NULL || strcmp(server,LCB_GETNODE_UNAVAILABLE) == 0)
		{
            LOG_OUT(LOG_PRIORITY_FATAL,"update get node error ");
            continue;
		}
		const char *endp = strchr(server, ':');
        if(endp == NULL)
        {
            LOG_OUT(LOG_PRIORITY_FATAL,"get node format error %s", server);
            continue;
        }
        std::string host(server, endp);
		in_addr_t addr = inet_addr(host.c_str());
        LOG_OUT(LOG_PRIORITY_RUN,"update cluster server index:%d ip: %s", ii, host.c_str());
        if(_congestionCounter.find(addr) == _congestionCounter.end())
            _congestionCounter[addr] = 0;
    }
	_isUpdating = false;
}

void ASyncCouchbase::IncreaseCongestionCounter(unsigned int hostIp, int step)
{
    if(_congestionCounter.find(hostIp) == _congestionCounter.end())
        updateCongestionCounter();
    _congestionCounter[hostIp] += step;
}

bool ASyncCouchbase::isTargetHostInCongestion(unsigned int hostIp)
{
    if(_congestionCounter.find(hostIp) == _congestionCounter.end())
        return false;
   return (_congestionCounter[hostIp] > CONGESTIONCOUNTER);
}

void ASyncCouchbase::OnGetCallbackWrap(lcb_t instance, const void *cookie,
        lcb_error_t error, const lcb_get_resp_t *item)
{
    if (instance)
    {
        ASyncCBInstanceCookie* instanceCookie = NULL;
        instanceCookie = (ASyncCBInstanceCookie*) lcb_get_cookie(instance);
        instanceCookie->_ASyncCouchbaseInstance->OnGetCallback(instance, cookie, error, item);
    }
}

void ASyncCouchbase::OnGetCallback(lcb_t instance, const void *cookie,
        lcb_error_t error, const lcb_get_resp_t *item)
{
    const void* busiCookie = cookie;
    if(cookie && _isTickwait)
    {
        ASyncCBGetCookie* curCookie = (ASyncCBGetCookie*)cookie;
        unsigned int ip = curCookie->hostIp;
        busiCookie = curCookie->cookie;
        IncreaseCongestionCounter(ip, -1);
        delete curCookie;
    }

    if(_getCallBack)
        _getCallBack(instance, busiCookie, error, item);
}

void ASyncCouchbase::OnIncrCallbackWrap(lcb_t instance, const void *cookie,
        lcb_error_t error, const lcb_arithmetic_resp_t *item)
{
    if (instance)
    {
        ASyncCBInstanceCookie* instanceCookie = NULL;
        instanceCookie = (ASyncCBInstanceCookie*) lcb_get_cookie(instance);
        instanceCookie->_ASyncCouchbaseInstance->OnIncrCallback(instance, cookie, error, item);
    }

}

void ASyncCouchbase::OnIncrCallback(lcb_t instance, const void *cookie,
        lcb_error_t error, const lcb_arithmetic_resp_t *item)
{
    const void* busiCookie = cookie;
    if(cookie && _isTickwait)
    {
        ASyncCBGetCookie* curCookie = (ASyncCBGetCookie*)cookie;
        unsigned int ip = curCookie->hostIp;
        busiCookie = curCookie->cookie;
        IncreaseCongestionCounter(ip, -1);
        delete curCookie;
    }

    if(_incrCallBack)
        _incrCallBack(instance, busiCookie, error, item);

}


void ASyncCouchbase::OnSetCallbackWrap(lcb_t instance, const void *cookie, lcb_storage_t operation,
        lcb_error_t error, const lcb_store_resp_t *item)
{
    if (instance)
    {
        ASyncCBInstanceCookie* instanceCookie = NULL;
        instanceCookie = (ASyncCBInstanceCookie*) lcb_get_cookie(instance);
        instanceCookie->_ASyncCouchbaseInstance->OnSetCallback(instance, cookie, operation, error, item);
    }

}

void ASyncCouchbase::OnSetCallback(lcb_t instance, const void *cookie, lcb_storage_t operation,
        lcb_error_t error, const lcb_store_resp_t *item)
{
    const void* busiCookie = cookie;
    if(cookie && _isTickwait)
    {
        ASyncCBGetCookie* curCookie = (ASyncCBGetCookie*)cookie;
        unsigned int ip = curCookie->hostIp;
        busiCookie = curCookie->cookie;
        IncreaseCongestionCounter(ip, -1);
        delete curCookie;
    }

    if(_setCallBack)
        _setCallBack(instance, busiCookie, operation, error, item);

}
