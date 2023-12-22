#include "../include/IniManager.h"

IniManager::IniManager()
{
}

IniManager::~IniManager()
{
}

bool IniManager::Init(inifile::IniFile& ini_parser)
{
    if (!readCfgInfos(ini_parser))
    {
        return false;
    }
    if (!checkCfgInfos())
    {
        return false;
    }
    return true;
}

std::string IniManager::GetErrString()
{
    return err_msg;
}

void IniManager::Show()
{
    std::cout << "--------------------------------------------------------------------------" << std::endl;
    std::cout << "output_file_name == "      << m_output_file_name << std::endl
              << "input_file_path_core0 == " << m_input_file_path_core0 << std::endl
              << "input_file_path_core1 == " << m_input_file_path_core1 << std::endl
              << "flag == "                  << m_flag << std::endl
              << "image_version == "         << m_image_version << std::endl
              << "inter_version == "         << m_inter_version << std::endl;
    for(auto& it : m_seg_infos_core0)
    {
        std::cout << "-----------------------" << std::endl;
        std::cout << "start_addr == "   << it.start_addr << std::endl
                  << "max_length == " << it.max_length << std::endl
                  << "file_name == "    << it.file_name << std::endl;
    }
    for(auto& it : m_seg_infos_core1)
    {
        std::cout << "-----------------------" << std::endl;
        std::cout << "start_addr == "   << it.start_addr << std::endl
                  << "max_length == " << it.max_length << std::endl
                  << "file_name == "    << it.file_name << std::endl;
    }
    std::cout << "--------------------------------------------------------------------------" << std::endl;
}

bool IniManager::readCfgInfos(inifile::IniFile& ini_parser)
{
    std::string section = "BuildType";
    if (ini_parser.HasSection(section))
    {
        if (!GetIntValue(ini_parser, section, "Head_Type", m_head_type))
        {
            return false;
        }
        if(m_head_type != 0 && m_head_type != 1)
        {
            err_msg = "Error: [BuildType] Head_Type should be 0(build 64 Byte header) or 1(build 1024 Byte header)!";
            return false;
        }
    }
    else
    {
        err_msg = "Error: The cfg have no this section:[" + section + "]!";
        return false;
    }

    section = "DstInfo";
    if (ini_parser.HasSection(section))
    {
        if (!GetStringValue(ini_parser, section, "OutputbinFileName", m_output_file_name))
        {
            return false;
        }
        if (!GetStringValue(ini_parser, section, "InputbinFilePath_core0", m_input_file_path_core0))
        {
            return false;
        }
        if(m_head_type == 0)
        {
            if (!GetIntValue(ini_parser, section, "crc16orsum16_flag", m_flag))
            {
                return false;
            }
            std::string sec = "OPTIONROM";
            if (ini_parser.HasSection(sec))
            {
                GetStringValue(ini_parser, sec, "FileName", m_option_rom_path);
            }
        }
        else
        {
            if (!GetStringValue(ini_parser, section, "InputbinFilePath_core1", m_input_file_path_core1))
            {
                return false;
            }
            if (!GetStringValue(ini_parser, section, "FW_Image_Ver_Int", m_inter_version))
            {
                return false;
            }
            if (!GetStringValue(ini_parser, section, "FW_Image_Ver_Ext", m_image_version))
            {
                return false;
            }
        }
    }
    else
    {
        err_msg = "Error: The cfg have no this section:[" + section + "]!";
        return false;
    }

    section = "CORE0_SEG";
    if(!getCoreSegInfo(ini_parser, section, m_seg_infos_core0))
    {
        return false;
    }
    m_seg_nums_core0 = getSegNums(ini_parser,section);

    if(!m_input_file_path_core1.empty())
    {
        section = "CORE1_SEG";
        if(!getCoreSegInfo(ini_parser, section, m_seg_infos_core1))
        {
            return false;
        }
        m_seg_nums_core1 = getSegNums(ini_parser,section);
    }
    else
    {
        m_seg_nums_core1 = 0;
    }
    
    return true;
}

bool IniManager::checkCfgInfos()
{

    if(m_head_type == 0)
    {
        if(m_flag != 0 && m_flag != 1)
        {
            err_msg = "Error: crc16orsum16_flag error, it should be 0(check crc16) or 1(check sum16)!";
            return false;
        }
        if(m_output_file_name.empty() || m_input_file_path_core0.empty())
        {
            err_msg = "Error: OutputbinFileName or InputbinFilePath_core0 or [OPTIONROM] FileName is empty!";
            return false;
        }
        if(!checkSingleSegInfos())
        {
            return false;
        }
    }
    else
    {
        if(m_output_file_name.empty() || m_input_file_path_core0.empty())
        {
            err_msg = "Error: OutputbinFileName or InputbinFilePath_core0 is empty!";
            return false;
        }
        if(m_inter_version.empty() && !m_image_version.empty())
        {
            m_inter_version = m_image_version;
            std::cout << "Info: Inter version is empty and image version not empty, inter version will be same as image version: " << m_image_version << std::endl;
        }
        if(m_inter_version.size() > 8 || m_image_version.size() > 8)
        {
            err_msg = "Error: FW_Image_Ver_Int or FW_Image_Ver_Ext size is mo than 8!";
            return false;
        }
        if(!checkDualSegInfos())
        {
            return false;
        }
    }
    return true;
}

int IniManager::getSegNums(inifile::IniFile& ini_parser, std::string& section)
{
    vector<string> sections;
    ini_parser.GetSections(&sections);
    int num = 0;
    for(auto& it : sections)
    {
        if(it.find(section) != std::string::npos)
        {
            num++;
        }
    }
    return num;
}

bool IniManager::checkDualSegInfos()
{
    if(m_seg_infos_core0.size() != m_seg_nums_core0 || m_seg_infos_core1.size() != m_seg_nums_core1)
    {
        err_msg = "Error: CORE0 or CORE1 SEG nums check error, The suffix must be a continuous natural number!";
        return false;
    }
    if(m_seg_nums_core0 + m_seg_nums_core1 > FW_IMAGE_SECTION_MAX_NUM)
    {
        err_msg = "Error: SEG nums more than 16!";
        return false;
    }
    std::unordered_set<int> addr;
    for(auto& it : m_seg_infos_core0)
    {
        if(it.file_name.empty())
        {
            err_msg = "Error: SEG's FileName can't be empty!";
            return false;
        }

        if(it.max_length == 0)
        {
            err_msg = "Error: SEG's MaxLength can't be 0!";
            return false;
        }

        if(it.overlay_idx == 0 && addr.count(it.start_addr) != 0)
        {
            err_msg = "Error: SEG's StartAddr can't be same!";
            return false;
        }
        addr.insert(it.start_addr);

        for(auto& iter : m_seg_infos_core0)
        {
            if(it.start_addr > iter.start_addr)
            {
                if(iter.overlay_idx == 0 && iter.start_addr + iter.max_length > it.start_addr)
                {
                    err_msg = "Error: One or some SEG's StartAddr + MaxLength > other SEG's StartAddr, will result in data loss!";
                    return false;
                }
            }
            else if(it.start_addr < iter.start_addr)
            {
                if(it.overlay_idx == 0 && it.start_addr + it.max_length > iter.start_addr)
                {
                    err_msg = "Error: One or some SEG's StartAddr + MaxLength > other SEG's StartAddr, will result in data loss!";
                    return false;
                }
            }
            else
            {
                continue;
            }
        } 
        for(auto& iter : m_seg_infos_core1)
        {
            if(it.start_addr > iter.start_addr)
            {
                if(iter.overlay_idx == 0 && iter.start_addr + iter.max_length > it.start_addr)
                {
                    err_msg = "Error: One or some SEG's StartAddr + MaxLength > other SEG's StartAddr, will result in data loss!";
                    return false;
                }
            }
            else if(it.start_addr < iter.start_addr)
            {
                if(it.overlay_idx == 0 && it.start_addr + it.max_length > iter.start_addr)
                {
                    err_msg = "Error: One or some SEG's StartAddr + MaxLength > other SEG's StartAddr, will result in data loss!";
                    return false;
                }
            }
            else
            {
                continue;
            }
        } 
    }
    for(auto& it : m_seg_infos_core1)
    {
        if(it.file_name.empty())
        {
            err_msg = "Error: SEG's FileName can't be empty!";
            return false;
        }

        if(it.max_length == 0)
        {
            err_msg = "Error: SEG's MaxLength can't be 0!";
            return false;
        }

        if(addr.count(it.start_addr) != 0)
        {
            err_msg = "Error: SEG's StartAddr can't be same!";
            return false;
        }
        addr.insert(it.start_addr);

        for(auto& iter : m_seg_infos_core0)
        {
            if(it.start_addr > iter.start_addr)
            {
                if(iter.overlay_idx == 0 && iter.start_addr + iter.max_length > it.start_addr)
                {
                    err_msg = "Error: One or some SEG's StartAddr + MaxLength > other SEG's StartAddr, will result in data loss!";
                    return false;
                }
            }
            else if(it.start_addr < iter.start_addr)
            {
                if(it.overlay_idx == 0 && it.start_addr + it.max_length > iter.start_addr)
                {
                    err_msg = "Error: One or some SEG's StartAddr + MaxLength > other SEG's StartAddr, will result in data loss!";
                    return false;
                }
            }
            else
            {
                continue;
            }
        } 
        for(auto& iter : m_seg_infos_core1)
        {
            if(it.start_addr > iter.start_addr)
            {
                if(iter.overlay_idx == 0 && iter.start_addr + iter.max_length > it.start_addr)
                {
                    err_msg = "Error: One or some SEG's StartAddr + MaxLength > other SEG's StartAddr, will result in data loss!";
                    return false;
                }
            }
            else if(it.start_addr < iter.start_addr)
            {
                if(it.overlay_idx == 0 && it.start_addr + it.max_length > iter.start_addr)
                {
                    err_msg = "Error: One or some SEG's StartAddr + MaxLength > other SEG's StartAddr, will result in data loss!";
                    return false;
                }
            }
            else
            {
                continue;
            }
        } 
    }
    return true;
}

bool IniManager::checkSingleSegInfos()
{
    if(m_seg_infos_core0.size() != 2 || m_seg_infos_core0.size() != m_seg_nums_core0)
    {
        err_msg = "Error: SEG nums error, must be 2 SEG ([CORE0_SEG0] and [CORE0_SEG1])!";
        return false;
    }
    std::unordered_set<int> addr;
    int atcm = 0,btcm = 0; 
    for(auto& it : m_seg_infos_core0)
    {
        if(it.file_name.empty())
        {
            err_msg = "Error: SEG's FileName can't be empty!";
            return false;
        }

        if(it.max_length == 0)
        {
            err_msg = "Error: SEG's MaxLength can't be 0!";
            return false;
        }

        if(addr.count(it.start_addr) != 0)
        {
            err_msg = "Error: SEG's StartAddr can't be same!";
            return false;
        }
        addr.insert(it.start_addr);

        if(it.file_name.find("ATCM") != std::string::npos)
        {
            atcm++;
        }

        if(it.file_name.find("BTCM") != std::string::npos)
        {
            btcm++;
        }

        for(auto& iter : m_seg_infos_core0)
        {
            if(it.start_addr > iter.start_addr)
            {
                if(iter.overlay_idx == 0 && iter.start_addr + iter.max_length > it.start_addr)
                {
                    err_msg = "Error: One or some SEG's StartAddr + MaxLength > other SEG's StartAddr, will result in data loss!";
                    return false;
                }
               
            }
            else if(it.start_addr < iter.start_addr)
            {
                if(it.overlay_idx == 0 && it.start_addr + it.max_length > iter.start_addr)
                {
                    err_msg = "Error: One or some SEG's StartAddr + MaxLength > other SEG's StartAddr, will result in data loss!";
                    return false;
                }
            }
            else
            {
                continue;
            }
        } 
    }
    if(atcm == 1 && btcm == 1)
    {
        return true;
    }
    else
    {
        err_msg = "Error: ATCM file nums not 1 or BTCM file nums not 1";
        return false;
    }
    
}

bool IniManager::getCoreSegInfo(inifile::IniFile& ini_parser, std::string& section, std::vector<cfgSegInfo>& seg_infos)
{
    int i = 0;
    while(1)
    {
        std::string sec = section + std::to_string(i);
        if (ini_parser.HasSection(sec))
        {
            cfgSegInfo info; 
            std::string str;
            if (!GetStringValue(ini_parser, sec, "StartAddr", str))
            {
                return false;
            }
            info.start_addr = strtol(str.c_str(), NULL, 16);

            if (!GetStringValue(ini_parser, sec, "MaxLength", str))
            {
                return false;
            }
            info.max_length = strtol(str.c_str(), NULL, 16);

            if (!GetStringValue(ini_parser, sec, "FileName", info.file_name))
            {
                return false;
            }

            int num = 0;
            if (!GetIntValue(ini_parser, sec, "CpuId", num))
            {
                return false;
            }
            info.cpu_id = num;

            if (!GetIntValue(ini_parser, sec, "Overlay", num))
            {
                return false;
            }
            info.overlay_idx = num;

            seg_infos.emplace_back(info);
            i++;
        }
        else
        {
            break;
        }
    }
    return true;
}

bool IniManager::GetIntValue(inifile::IniFile& ini_parser, const string &section, const string &key, int& value)
{
    if (ini_parser.HasKey(section, key))
    {
        ini_parser.GetIntValue(section, key, &value);
    }
    else
    {
        err_msg = "Error: The cfg section [" + section +"] have no [" + key + "]!";
        return false;
    }
    return true;
}

bool IniManager::GetStringValue(inifile::IniFile& ini_parser, const string &section, const string &key, std::string& value)
{
    if (ini_parser.HasKey(section, key))
    {
        ini_parser.GetStringValue(section, key, &value);
    }
    else
    {
        err_msg = "Error: The cfg section [" + section +"] have no [" + key + "]!";
        return false;
    }
    return true;
}
