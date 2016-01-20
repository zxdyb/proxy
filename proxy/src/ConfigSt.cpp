#include "ConfigSt.h"


ConfigSt::ConfigSt(const std::string &strIniFile)
{
    m_strFileName = strIniFile;

    m_blInitSuccess = Load();
}

bool ConfigSt::Load()
{
    try
    {
        boost::property_tree::ini_parser::read_ini(m_strFileName, m_pt);
    }
    catch (...)
    {
        return false;
    }
    return true;
}


ConfigSt::~ConfigSt(void)
{
}

std::string ConfigSt::GetItem(const std::string &strItemPath)
{
    if (!m_blInitSuccess)
    {
        return "";
    }
    else
    {
        try
        {
            return m_pt.get<std::string>(strItemPath);
        }
        catch(...)
        {
            return "";
        }
    }

    //return !m_blInitSuccess ? "" : m_pt.get<std::string>(strItemPath);
}

void ConfigSt::SetItem(const std::string &strItemPath, const std::string &strItemValue, const bool isNeedSave)
{
    if (!m_blInitSuccess)
    {

        return;
    }

    m_pt.put<std::string>(strItemPath, strItemValue);

    if (isNeedSave)
    {
        Save();
    }

}

bool ConfigSt::Save()
{
    if (!m_blInitSuccess)
    {
        return false;
    }

    try
    {
        boost::property_tree::ini_parser::write_ini(m_strFileName, m_pt);
    }
    catch(...)
    {
        return false;
    }   

    return true;
}

bool ConfigSt::Refresh()
{
    return m_blInitSuccess = Load();
}




