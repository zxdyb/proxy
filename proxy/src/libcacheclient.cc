#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <boost/shared_ptr.hpp>
#include <libmemcached/memcached.h>
#include "libcacheclient.h"

using namespace roiland;

MemcacheClient* MemcacheClient::create()
{
	return new MemcacheClientImpl();
}

void MemcacheClient::destoy(MemcacheClient* cl)
{
	delete cl;
}

MemcacheClientImpl::MemcacheClientImpl()
{
	memc_ = (void*)memcached_create(NULL);
}

MemcacheClientImpl::~MemcacheClientImpl()
{
	memcached_free((memcached_st*)memc_);
}

int MemcacheClientImpl::addServer(const char* ip,
	int port)
{
	if (ip == NULL )
		return CACHE_FAILURE;

	memcached_return rc;                                                                   
	memcached_server_st *server = NULL;

	server =memcached_server_list_append(server, ip, port, &rc);
	rc=memcached_server_push((memcached_st*)memc_, server);

	if(MEMCACHED_SUCCESS != rc)
	{
		memcached_server_list_free(server);
		fprintf(stderr,"[MemcacheClient]couldn't add server: %s\n", memcached_strerror((memcached_st*)memc_, rc));
		return CACHE_FAILURE;
	}
	
	//使用一致性算法,保证dead server的自动隔离和自动连接
	memcached_behavior_set((memcached_st*)memc_, MEMCACHED_BEHAVIOR_DISTRIBUTION,MEMCACHED_DISTRIBUTION_CONSISTENT);
	//失败多少次就视为 dead server
	memcached_behavior_set((memcached_st*)memc_, MEMCACHED_BEHAVIOR_SERVER_FAILURE_LIMIT, 3);
	//dead server 重连时间,单位秒
	memcached_behavior_set((memcached_st*)memc_, MEMCACHED_BEHAVIOR_RETRY_TIMEOUT, 1);
	//是否删除 dead server
	memcached_behavior_set((memcached_st*)memc_, MEMCACHED_BEHAVIOR_REMOVE_FAILED_SERVERS ,0);
	//异步
	memcached_behavior_set((memcached_st*)memc_, MEMCACHED_BEHAVIOR_NO_BLOCK, 1);
	//使用2进制协议(启用cas则不能使用,否则cas机制失效)
	memcached_behavior_set((memcached_st*)memc_, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 0);
	//不使用DELAY
	memcached_behavior_set((memcached_st*)memc_, MEMCACHED_BEHAVIOR_TCP_NODELAY, 1);
	//使用cas
	memcached_behavior_set((memcached_st*)memc_, MEMCACHED_BEHAVIOR_SUPPORT_CAS, true);
	//连接超时ms
	memcached_behavior_set((memcached_st*)memc_, MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, 300);
	//读超时us
	memcached_behavior_set((memcached_st*)memc_, MEMCACHED_BEHAVIOR_RCV_TIMEOUT, 300*1000);
	//写超时us
	memcached_behavior_set((memcached_st*)memc_, MEMCACHED_BEHAVIOR_SND_TIMEOUT, 300*1000);
	
	memcached_behavior_set((memcached_st*)memc_, MEMCACHED_BEHAVIOR_POLL_TIMEOUT, 300);
	
	//自动隔离dead server
	//memcached_behavior_set((memcached_st*)memc_, MEMCACHED_BEHAVIOR_AUTO_EJECT_HOSTS, 0);  

	return CACHE_SUCCESS;
}

int MemcacheClientImpl::exist(const char* key)
{
	//uint64_t value = 0;
	memcached_return rc;
	rc = memcached_exist((memcached_st*)memc_, key, strlen(key));

	if (rc == MEMCACHED_NOTFOUND){
		return rc;
	}
	else if (MEMCACHED_SUCCESS  != rc){
		fprintf(stderr,"[MemcacheClient]memcached_exist failed: %s\n", memcached_strerror((memcached_st*)memc_, rc));
		if(MEMCACHED_CONNECTION_FAILURE == rc || MEMCACHED_SERVER_MARKED_DEAD == rc || MEMCACHED_SERVER_TEMPORARILY_DISABLED == rc) return CACHE_CONNECTION_FAILURE;
		return rc;
	}
	return CACHE_SUCCESS;
}

int MemcacheClientImpl::set(const char* key, 
	const char* value,
	int value_length,
	time_t expiration )
{
	if (NULL == key || value == NULL || value_length < 1 )
		return CACHE_FAILURE;

	uint32_t flags = 0;
	memcached_return rc;

	rc = memcached_set((memcached_st*)memc_, key, 
		strlen(key),
		value, 
		value_length, 
		expiration, 
		flags);
	if (MEMCACHED_SUCCESS != rc){
		fprintf(stderr,"[MemcacheClient]memcached_set failed: %s\n", memcached_strerror((memcached_st*)memc_, rc));
		if(MEMCACHED_CONNECTION_FAILURE == rc || MEMCACHED_SERVER_MARKED_DEAD == rc || MEMCACHED_SERVER_TEMPORARILY_DISABLED == rc) return CACHE_CONNECTION_FAILURE;
		return rc;
	}
	return CACHE_SUCCESS;
}

int MemcacheClientImpl::set_cas(const char* key, 
	const char* value,
	int value_length,
	uint64_t cas,
	time_t expiration )
{
	if (NULL == key || value == NULL || value_length < 1 )
		return CACHE_FAILURE;

	uint32_t flags = 0;
	memcached_return rc;

	rc = memcached_cas((memcached_st*)memc_, key, 
		strlen(key),
		value, 
		value_length, 
		expiration, 
		flags,
		cas);
	if (MEMCACHED_SUCCESS != rc && MEMCACHED_END != rc){
		fprintf(stderr,"[MemcacheClient]memcached_set failed: %s\n", memcached_strerror((memcached_st*)memc_, rc));
		if(MEMCACHED_CONNECTION_FAILURE == rc || MEMCACHED_SERVER_MARKED_DEAD == rc || MEMCACHED_SERVER_TEMPORARILY_DISABLED == rc) return CACHE_CONNECTION_FAILURE;
		if(MEMCACHED_DATA_EXISTS == rc)  return CACHED_CAS_ERROR; 
		return rc;
	}
	return CACHE_SUCCESS;
}

int  MemcacheClientImpl::get(const char* key,
	std::string& value)
{
	if (NULL == key )
		return CACHE_FAILURE;
	uint32_t flags = 0;
	memcached_return rc;
	char* data = NULL;
	size_t length = 0;
	data = memcached_get((memcached_st*)memc_, 
		key, 
		strlen(key), 
		&length, 
		&flags, 
		&rc);

	if (MEMCACHED_SUCCESS != rc){
		if(MEMCACHED_NOTFOUND == rc) return CACHE_NOTFOUND;
		fprintf(stderr,"[MemcacheClient]memcached_get failed: %s\n", memcached_strerror((memcached_st*)memc_, rc));
		if(MEMCACHED_CONNECTION_FAILURE == rc || MEMCACHED_SERVER_MARKED_DEAD == rc || MEMCACHED_SERVER_TEMPORARILY_DISABLED == rc) return CACHE_CONNECTION_FAILURE;
		return rc;
	}
	if (!data) return CACHE_FAILURE;
	value.assign(data,length);
	free(data);
	return CACHE_SUCCESS;
}

int  MemcacheClientImpl::get_cas(const char* key,
	std::string& value,
	uint64_t* cas)
{
	if (key  == NULL)
		return CACHE_FAILURE;
	memcached_return rc;
	memcached_result_st * results_t;
	const char* keys[2] = { key, NULL };
	size_t keylengths[2] = { strlen(key), 0 };

	rc = memcached_mget((memcached_st*)memc_,keys,keylengths,1);
	if (MEMCACHED_SUCCESS != rc){
		if(MEMCACHED_NOTFOUND == rc) return CACHE_NOTFOUND;
		fprintf(stderr,"[MemcacheClient]memcached_get failed: %s\n", memcached_strerror((memcached_st*)memc_, rc));
		if(MEMCACHED_CONNECTION_FAILURE == rc || MEMCACHED_SERVER_MARKED_DEAD == rc || MEMCACHED_SERVER_TEMPORARILY_DISABLED == rc) return CACHE_CONNECTION_FAILURE;
		return rc;
	}

	if ((results_t = memcached_fetch_result((memcached_st*)memc_,NULL,&rc))!=NULL)
	{
		const char* key = memcached_result_key_value(results_t);
		const char* value_ = memcached_result_value(results_t);
		int value_length = memcached_result_length(results_t);
		*cas = memcached_result_cas(results_t);
		if (key == NULL || value_ == NULL || value_length == 0)
			return CACHE_NOTFOUND;
		value = std::string(value_,value_length);
		memcached_result_free(results_t);
	}

	if (MEMCACHED_SUCCESS != rc && MEMCACHED_END != rc){
		fprintf(stderr,"[MemcacheClient]memcached_fetch_result failed: %s\n", memcached_strerror((memcached_st*)memc_, rc));
		if(MEMCACHED_CONNECTION_FAILURE == rc || MEMCACHED_SERVER_MARKED_DEAD == rc || MEMCACHED_SERVER_TEMPORARILY_DISABLED == rc) return CACHE_CONNECTION_FAILURE;
		return rc;
	}
	return CACHE_SUCCESS;
}

int MemcacheClientImpl::gets(const key_set_t& keys,
	result_set_t& results)
{
	if (keys.size()==0)
		return CACHE_FAILURE;

	char * *keys_ = new char *[keys.size()];
	size_t* size_ = new size_t[keys.size()];
	size_t number_of_keys = keys.size();
    int nsize = keys.size();
	for(int n = 0;n < nsize;n++)
	{
		size_[n] = 	keys[n].length();
		keys_[n] = (char*)keys[n].c_str();
	}
	memcached_return rc;
	memcached_result_st * results_t;

	rc = memcached_mget((memcached_st*)memc_,keys_,size_,number_of_keys);
	if (MEMCACHED_SUCCESS != rc){
		delete []size_;
		delete []keys_;
		fprintf(stderr,"[MemcacheClient]memcached_get failed: %s\n", memcached_strerror((memcached_st*)memc_, rc));
		if(MEMCACHED_CONNECTION_FAILURE == rc || MEMCACHED_SERVER_MARKED_DEAD == rc || MEMCACHED_SERVER_TEMPORARILY_DISABLED == rc) return CACHE_CONNECTION_FAILURE;
		return rc;
	}
	delete []size_;
	delete []keys_;
	while ((results_t = memcached_fetch_result((memcached_st*)memc_,NULL,&rc))!=NULL)
	{
		const char* key = memcached_result_key_value(results_t);
		int key_length = memcached_result_key_length(results_t);
		const char* value = memcached_result_value(results_t);
		int value_length = memcached_result_length(results_t);
		if (key == NULL || value == NULL || value_length == 0)
			continue;
		results[std::string(key,key_length)] = std::string(value,value_length);
		memcached_result_free(results_t);
	}

	if (MEMCACHED_SUCCESS != rc && MEMCACHED_END != rc && MEMCACHED_NOTFOUND != rc){
		fprintf(stderr,"[MemcacheClient]memcached_fetch_result failed: %s\n", memcached_strerror((memcached_st*)memc_, rc));
		if(MEMCACHED_CONNECTION_FAILURE == rc || MEMCACHED_SERVER_MARKED_DEAD == rc || MEMCACHED_SERVER_TEMPORARILY_DISABLED == rc) return CACHE_CONNECTION_FAILURE;
		return rc;
	}
	return CACHE_SUCCESS;
}

//写
int MemcacheClientImpl::set_group(const char* group, 
	const char* key, 
	const char* value,
	int value_length,
	uint64_t cas,
	time_t expiration)
{
	if (NULL == group || NULL == key || value == NULL || value_length < 1 )
		return CACHE_FAILURE;

	uint32_t flags = 0;
	memcached_return rc;

	rc = memcached_cas_by_key((memcached_st*)memc_, 
		group, 
		strlen(group),
		key, 
		strlen(key),
		value, 
		value_length, 
		expiration, 
		flags,
		cas);
	if (MEMCACHED_SUCCESS != rc && MEMCACHED_END != rc){
		fprintf(stderr,"[MemcacheClient]memcached_set failed: %s\n", memcached_strerror((memcached_st*)memc_, rc));
		if(MEMCACHED_CONNECTION_FAILURE == rc || MEMCACHED_SERVER_MARKED_DEAD == rc || MEMCACHED_SERVER_TEMPORARILY_DISABLED == rc) return CACHE_CONNECTION_FAILURE;
		if(MEMCACHED_DATA_EXISTS == rc)  return CACHED_CAS_ERROR; 
		return rc;
	}
	return CACHE_SUCCESS;
}

//写
int MemcacheClientImpl::get_group(const char* group, 
	const char* key,
	std::string& value,
	uint64_t* cas)
{
	if (group == NULL || key  == NULL)
		return CACHE_FAILURE;
	memcached_return rc;
	memcached_result_st * results_t;
	const char* keys[2] = { key, NULL };
	size_t keylengths[2] = { strlen(key), 0 };

	rc = memcached_mget_by_key((memcached_st*)memc_,group,strlen(group),keys,keylengths,1);
	if (MEMCACHED_SUCCESS != rc){
		fprintf(stderr,"[MemcacheClient]memcached_get failed: %s\n", memcached_strerror((memcached_st*)memc_, rc));
		if(MEMCACHED_CONNECTION_FAILURE == rc || MEMCACHED_SERVER_MARKED_DEAD == rc || MEMCACHED_SERVER_TEMPORARILY_DISABLED == rc) return CACHE_CONNECTION_FAILURE;
		return rc;
	}

	if ((results_t = memcached_fetch_result((memcached_st*)memc_,NULL,&rc))!=NULL)
	{
		const char* key = memcached_result_key_value(results_t);
		const char* value_ = memcached_result_value(results_t);
		int value_length = memcached_result_length(results_t);
		*cas = memcached_result_cas(results_t);
		if (key == NULL || value_ == NULL || value_length == 0)
			return CACHE_NOTFOUND;
		value = std::string(value_,value_length);
		memcached_result_free(results_t);
	}

	if (MEMCACHED_SUCCESS != rc && MEMCACHED_END != rc){
		fprintf(stderr,"[MemcacheClient]memcached_fetch_result failed: %s\n", memcached_strerror((memcached_st*)memc_, rc));
		if(MEMCACHED_CONNECTION_FAILURE == rc || MEMCACHED_SERVER_MARKED_DEAD == rc || MEMCACHED_SERVER_TEMPORARILY_DISABLED == rc) return CACHE_CONNECTION_FAILURE;
		return rc;
	}
	return CACHE_SUCCESS;
}

int  MemcacheClientImpl::replace(const char* key, 
	const char* value,
	int value_length,
	time_t expiration )
{
	if (NULL == key || value == NULL || value_length < 1 )
		return CACHE_FAILURE;

	uint32_t flags = 0;
	memcached_return rc;

	rc = memcached_replace((memcached_st*)memc_, 
		key, 
		strlen(key), 
		value, 
		value_length,
		expiration,
		flags);

	if (MEMCACHED_SUCCESS != rc){
		fprintf(stderr,"[MemcacheClient]memcached_replace failed: %s\n", memcached_strerror((memcached_st*)memc_, rc));
		if(MEMCACHED_CONNECTION_FAILURE == rc || MEMCACHED_SERVER_MARKED_DEAD == rc || MEMCACHED_SERVER_TEMPORARILY_DISABLED == rc) return CACHE_CONNECTION_FAILURE;
		return rc;
	}
	return CACHE_SUCCESS;
}

int MemcacheClientImpl::increment(const char* key,
	int offset)
{
	if (NULL == key )
		return CACHE_FAILURE;
	uint64_t value = 0;
	memcached_return rc;
	rc = memcached_increment((memcached_st*)memc_, key, strlen(key), offset, &value);

	if (MEMCACHED_SUCCESS != rc){
		fprintf(stderr,"[MemcacheClient]memcached_increment failed: %s\n", memcached_strerror((memcached_st*)memc_, rc));
		if(MEMCACHED_CONNECTION_FAILURE == rc || MEMCACHED_SERVER_MARKED_DEAD == rc || MEMCACHED_SERVER_TEMPORARILY_DISABLED == rc) return CACHE_CONNECTION_FAILURE;
		return rc;
	}
	return CACHE_SUCCESS;
}

int MemcacheClientImpl::decrement(const char* key,
	int offset)
{
	if (NULL == key )
		return CACHE_FAILURE;

	uint64_t value = 0;
	memcached_return rc;
	rc = memcached_decrement((memcached_st*)memc_, key, strlen(key), offset, &value);

	if (MEMCACHED_SUCCESS != rc){
		fprintf(stderr,"[MemcacheClient]memcached_decrement failed: %s\n", memcached_strerror((memcached_st*)memc_, rc));
		if(MEMCACHED_CONNECTION_FAILURE == rc || MEMCACHED_SERVER_MARKED_DEAD == rc || MEMCACHED_SERVER_TEMPORARILY_DISABLED == rc) return CACHE_CONNECTION_FAILURE;
		return rc;
	}
	return CACHE_SUCCESS;
}

int  MemcacheClientImpl::remove(const char* key)
{ 
	if (NULL == key || strlen(key)==0)
		return CACHE_FAILURE;
	uint32_t flags = 0;
	memcached_return rc;
	rc = memcached_delete((memcached_st*)memc_, key, strlen(key), flags);

	if (MEMCACHED_SUCCESS != rc){
		fprintf(stderr,"[MemcacheClient]memcached_delete failed: %s\n", memcached_strerror((memcached_st*)memc_, rc));
		if(MEMCACHED_CONNECTION_FAILURE == rc || MEMCACHED_SERVER_MARKED_DEAD == rc || MEMCACHED_SERVER_TEMPORARILY_DISABLED == rc) return CACHE_CONNECTION_FAILURE;
		return rc;
	}
	return CACHE_SUCCESS;
}

int  MemcacheClientImpl::clear()
{
	memcached_return rc;
	rc = memcached_flush((memcached_st*)memc_, (time_t)0);

	if (MEMCACHED_SUCCESS != rc){
		fprintf(stderr,"[MemcacheClient]memcached_flush failed: %s\n", memcached_strerror((memcached_st*)memc_, rc));
		if(MEMCACHED_CONNECTION_FAILURE == rc || MEMCACHED_SERVER_MARKED_DEAD == rc || MEMCACHED_SERVER_TEMPORARILY_DISABLED == rc) return CACHE_CONNECTION_FAILURE;
		return rc;
	}
	return CACHE_SUCCESS;
}

memcached_return_t MemcachedDump(const memcached_st *ptr, const char *key, size_t key_length, void *context)
{
    if (NULL == ptr || NULL == key || NULL == context || key_length == 0) {
        fprintf(stderr, "[MemcacheClient]MemcachedDump Parameter check failed.");
        return MEMCACHED_FAILURE;
    }
    
    Func* fn = static_cast<Func*>(context);
    if (fn == NULL || *fn == NULL) {
        fprintf(stderr, "[MemcacheClient]MemcachedDump cast context is NULL.");
        return MEMCACHED_FAILURE;
    }

    (*fn)(std::string(key, key_length));
    return MEMCACHED_SUCCESS;
}

int MemcacheClientImpl::get_all_items()
{   
    if (m_pFn == NULL)
        return CACHE_FAILURE;

    memcached_return rc;
    memcached_dump_fn callback = &MemcachedDump;
    rc = memcached_dump((memcached_st*)memc_, &callback, (void *)&m_pFn, 1);
    if (rc != MEMCACHED_SUCCESS && rc != MEMCACHED_CLIENT_ERROR) {
        fprintf(stderr, "[MemcacheClient]memcached_dump failed: %s\n", memcached_strerror((memcached_st*)memc_, rc));
        if (MEMCACHED_CONNECTION_FAILURE == rc || MEMCACHED_SERVER_MARKED_DEAD == rc || MEMCACHED_SERVER_TEMPORARILY_DISABLED == rc) return CACHE_CONNECTION_FAILURE;
        return rc;
    }

    return CACHE_SUCCESS;
}
