#pragma once
#include "Typedef.h"
#include "inifile.h"
#include <fstream>
#include <unordered_set>
#include <map>

class IniManager
{
public:
    IniManager();

    ~IniManager();

    bool Init(inifile::IniFile& ini_parser);

    std::string GetErrString();

    void Show();

private:
    bool readCfgInfos(inifile::IniFile& ini_parser);

    bool checkCfgInfos();

    bool getCoreSegInfo(inifile::IniFile& ini_parser, std::string& section, std::vector<cfgSegInfo>& seg_infos);

    bool checkSingleSegInfos();

    bool checkDualSegInfos();

    int  getSegNums(inifile::IniFile& ini_parser, std::string& section);

    bool GetIntValue(inifile::IniFile& ini_parser, const string &section, const string &key, int& value);

    bool GetStringValue(inifile::IniFile& ini_parser, const string &section, const string &key, std::string& value);

public:
    int                         m_head_type;
    int                         m_flag;
    std::string                 m_input_file_path_core0;
    std::string                 m_input_file_path_core1;
    std::vector<cfgSegInfo>     m_seg_infos_core0;
    std::vector<cfgSegInfo>     m_seg_infos_core1;
    std::string                 m_output_file_name;
    std::string                 m_image_version;
    std::string                 m_inter_version;
    std::string                 m_option_rom_path;
    
private:
    std::string                 err_msg; 
    int                         m_seg_nums_core0;
    int                         m_seg_nums_core1;
};