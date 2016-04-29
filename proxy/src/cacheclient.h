#ifndef ROILAND_BASE_MEMCACHED_CLIENT_H
#define ROILAND_BASE_MEMCACHED_CLIENT_H

#ifndef WIN32
#define ROILAND_DLL_API
#else
#ifndef ROILAND_DLL_API 
#define ROILAND_DLL_API _declspec(dllimport)
#else
#define ROILAND_DLL_API _declspec(dllexport)
#endif
#endif // WIN32
#include <stdint.h> 
#include <string>
#include <vector>
#include <map>
#include <boost/function.hpp>

namespace roiland
{
	/*
		cache子系统接口说明
		usage:
			MemcacheClient* cl = MemcacheClient::create();
			cl.addServer(ip,port);
			……
			……
			MemcacheClient::destroy(cl);
	*/
    typedef boost::function<void(const std::string&)> Func;

	class ROILAND_DLL_API MemcacheClient
	{
	public: 
		enum
		{
			CACHE_SUCCESS = 998997,
			CACHE_NOTFOUND,
			CACHE_FAILURE,
			CACHE_CONNECTION_FAILURE,
			CACHE_OTHER_FAILURE,
			CACHED_CAS_ERROR,
		};

		static MemcacheClient* create();
		static void destoy(MemcacheClient* cl);
	public: 
		MemcacheClient(){};
		virtual ~MemcacheClient(){};

		//添加 server
		virtual int addServer(const char* addr, 
			int port)=0;

		//检测KEY是否存在
		virtual int exist(const char* key)=0;

		//写
		virtual int set(const char* key, 
			const char* value,
			int value_length,
			time_t expiration = 0/*默认永久有效*/)=0;

		//读
		virtual int get(const char* key,
			std::string& value)=0;

		//写
		virtual int set_cas(const char* key, 
			const char* value,
			int value_length,
			uint64_t cas,
			time_t expiration = 0/*默认永久有效*/)=0;
		//读
		virtual int get_cas(const char* key,
			std::string& value,
			uint64_t* cas)=0;

		//写
		virtual int set_group(const char* group, 
			const char* key, 
			const char* value,
			int value_length,
			uint64_t cas,
			time_t expiration = 0)=0;

		//读
		virtual int get_group(const char* group, 
			const char* key,
			std::string& value,
			uint64_t* cas)=0;

		/*
			gets接口返回的结果集,每一项对应一个key-value对,结果集为map容器:
			key -	cache中的KEY
			value - cache中对应的VALUE
		*/
		typedef std::vector<std::string> key_set_t;
		typedef std::map<std::string,std::string> result_set_t;

		//读多个
		virtual int gets(const key_set_t& keys,
			result_set_t& values)=0;

		//更新
		virtual int replace(const char* key, 
			const char* value,
			int value_length,
			time_t expiration = 0/*默认永久有效*/)=0;

		//增加数值元素的值
		virtual int increment(const char* key,
			int offset = 1)=0;

		//减小数值元素的值
		virtual int decrement(const char* key,
			int offset = 1)=0;

		//删除指定Key数据
		virtual int remove(const char* key)=0;

		//删除服务端所有数据
		virtual int clear()=0;

        //获取所有的item
        virtual int get_all_items() = 0;

        //设置回调函数
        virtual void SetCallBack(Func fn) = 0;

	};

};

#endif //ROILAND_BASE_MEMCACHED_CLIENT_H
