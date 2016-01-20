#ifndef _CONFIGST_
#define _CONFIGST_

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// �������ڣ�2014-11-21
// ��    �ߣ�����
// �޸����ڣ�
// �� �� �ߣ�
// �޸�˵����
// �� ժ Ҫ������ģ��
// ��ϸ˵����GetItem��SetItem�У�����strItemPath��ʽ��"Session.Key"������strItemValue����"Key"��Ӧ��ֵ
// ����˵����
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include <boost/cstdint.hpp>
#include <boost/noncopyable.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <string>


class ConfigSt : public boost::noncopyable
{
public:
    ConfigSt(const std::string &strIniFile);
    ~ConfigSt(void);

public:
    std::string GetItem(const std::string &strItemPath);

    void SetItem(const std::string &strItemPath, const std::string &strItemValue, const bool isNeedSave = true);

    bool Save();

    bool Refresh();
    
private:
    bool Load();

private:
    bool m_blInitSuccess;
    boost::property_tree::ptree m_pt;
    std::string m_strFileName;
};
#endif

