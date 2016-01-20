#ifndef _CONFIGST_
#define _CONFIGST_

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 创建日期：2014-11-21
// 作    者：尹宾
// 修改日期：
// 修 改 者：
// 修改说明：
// 类 摘 要：配置模块
// 详细说明：GetItem和SetItem中，参数strItemPath格式是"Session.Key"，参数strItemValue就是"Key"对应的值
// 附加说明：
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

