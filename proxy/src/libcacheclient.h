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

		//��� server
		int addServer(const char* addr, 
			int port);
		
		//���KEY�Ƿ����
		int exist(const char* key);

		//д
		int set(const char* key, 
			const char* value,
			int value_length,
			time_t expiration = 0/*Ĭ��������Ч*/);

		//д
		int set_cas(const char* key, 
			const char* value,
			int value_length,
			uint64_t cas,
			time_t expiration = 0/*Ĭ��������Ч*/);

		//д
		int set_group(const char* group, 
			const char* key, 
			const char* value,
			int value_length,
			uint64_t cas,
			time_t expiration = 0/*Ĭ��������Ч*/);

		//д
		int get_group(const char* group, 
			const char* key,
			std::string& value,
			uint64_t* cas);

		//��
		int get(const char* key,
			std::string& value);
		//��
		int get_cas(const char* key,
			std::string& value,
			uint64_t* cas);

		//�����
		int gets(const key_set_t& keys,
			result_set_t& values);

		//����
		int replace(const char* key, 
			const char* value,
			int value_length,
			time_t expiration = 0/*Ĭ��������Ч*/);

		//������ֵԪ�ص�ֵ
		int increment(const char* key,
			int offset = 1);

		//��С��ֵԪ�ص�ֵ
		int decrement(const char* key,
			int offset = 1);

		//ɾ��ָ��Key����
		int remove(const char* key);

		//ɾ���������������
		int clear();

        //��ȡ���е�item
        int get_all_items();

        //��ȡ�ص�����ָ��
        void SetCallBack(Func fn){ m_pFn = fn; }

	protected:
        Func m_pFn;
        void *memc_;
	};
};

#endif //ROILAND_BASE_MEMCACHED_H
