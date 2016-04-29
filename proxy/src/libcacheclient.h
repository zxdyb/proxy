#ifndef ROILAND_BASE_MEMCACHED_H
#define ROILAND_BASE_MEMCACHED_H

#include "cacheclient.h"

namespace roiland
{
	#define MAX_MULTI_ITEM_NUM 50
	#define MAX_KEY_LEN 250

	class MemcacheClientImpl : public roiland::MemcacheClient
	{
	public: 
		MemcacheClientImpl();
		~MemcacheClientImpl();

		//添加 server
		int addServer(const char* addr, 
			int port);
		
		//检测KEY是否存在
		int exist(const char* key);

		//写
		int set(const char* key, 
			const char* value,
			int value_length,
			time_t expiration = 0/*默认永久有效*/);

		//写
		int set_cas(const char* key, 
			const char* value,
			int value_length,
			uint64_t cas,
			time_t expiration = 0/*默认永久有效*/);

		//写
		int set_group(const char* group, 
			const char* key, 
			const char* value,
			int value_length,
			uint64_t cas,
			time_t expiration = 0/*默认永久有效*/);

		//写
		int get_group(const char* group, 
			const char* key,
			std::string& value,
			uint64_t* cas);

		//读
		int get(const char* key,
			std::string& value);
		//读
		int get_cas(const char* key,
			std::string& value,
			uint64_t* cas);

		//读多个
		int gets(const key_set_t& keys,
			result_set_t& values);

		//更新
		int replace(const char* key, 
			const char* value,
			int value_length,
			time_t expiration = 0/*默认永久有效*/);

		//增加数值元素的值
		int increment(const char* key,
			int offset = 1);

		//减小数值元素的值
		int decrement(const char* key,
			int offset = 1);

		//删除指定Key数据
		int remove(const char* key);

		//删除服务端所有数据
		int clear();

        //获取所有的item
        int get_all_items();

        //获取回调函数指针
        void SetCallBack(Func fn){ m_pFn = fn; }

	protected:
        Func m_pFn;
        void *memc_;
	};
};

#endif //ROILAND_BASE_MEMCACHED_H
