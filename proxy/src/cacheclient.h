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
		cache��ϵͳ�ӿ�˵��
		usage:
			MemcacheClient* cl = MemcacheClient::create();
			cl.addServer(ip,port);
			����
			����
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

		//��� server
		virtual int addServer(const char* addr, 
			int port)=0;

		//���KEY�Ƿ����
		virtual int exist(const char* key)=0;

		//д
		virtual int set(const char* key, 
			const char* value,
			int value_length,
			time_t expiration = 0/*Ĭ��������Ч*/)=0;

		//��
		virtual int get(const char* key,
			std::string& value)=0;

		//д
		virtual int set_cas(const char* key, 
			const char* value,
			int value_length,
			uint64_t cas,
			time_t expiration = 0/*Ĭ��������Ч*/)=0;
		//��
		virtual int get_cas(const char* key,
			std::string& value,
			uint64_t* cas)=0;

		//д
		virtual int set_group(const char* group, 
			const char* key, 
			const char* value,
			int value_length,
			uint64_t cas,
			time_t expiration = 0)=0;

		//��
		virtual int get_group(const char* group, 
			const char* key,
			std::string& value,
			uint64_t* cas)=0;

		/*
			gets�ӿڷ��صĽ����,ÿһ���Ӧһ��key-value��,�����Ϊmap����:
			key -	cache�е�KEY
			value - cache�ж�Ӧ��VALUE
		*/
		typedef std::vector<std::string> key_set_t;
		typedef std::map<std::string,std::string> result_set_t;

		//�����
		virtual int gets(const key_set_t& keys,
			result_set_t& values)=0;

		//����
		virtual int replace(const char* key, 
			const char* value,
			int value_length,
			time_t expiration = 0/*Ĭ��������Ч*/)=0;

		//������ֵԪ�ص�ֵ
		virtual int increment(const char* key,
			int offset = 1)=0;

		//��С��ֵԪ�ص�ֵ
		virtual int decrement(const char* key,
			int offset = 1)=0;

		//ɾ��ָ��Key����
		virtual int remove(const char* key)=0;

		//ɾ���������������
		virtual int clear()=0;

        //��ȡ���е�item
        virtual int get_all_items() = 0;

        //���ûص�����
        virtual void SetCallBack(Func fn) = 0;

	};

};

#endif //ROILAND_BASE_MEMCACHED_CLIENT_H
