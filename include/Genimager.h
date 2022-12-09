#pragma once
#include <iostream>
#include <fstream>
#include <memory>
#include <filesystem>
#include "Typedef.h"
#include "IniManager.h"

namespace fs = std::filesystem;

class DualGenimager{
public:
    DualGenimager();

    ~DualGenimager();

    bool Init(std::shared_ptr<IniManager>& iniManager);

    std::string GetErrString();

    bool BuildTargetFile();

protected:    
    bool getBinFiles(std::string& dir, std::string& file_name, std::vector<std::string>& core_files);

    bool searchFwString(std::fstream& pfile, char* res);

    unsigned short getCheckSum16(std::fstream& pfile, unsigned int byteCnt, unsigned short mode);

    unsigned short getCrc16(std::fstream& pfile, unsigned int byteCnt);

private:
    std::shared_ptr<IniManager>         m_ini_manager;
    std::vector<std::string>            m_bin_files_core0;
    std::vector<std::string>            m_bin_files_core1;
    int                                 m_target_file_size;
    std::string                         err_msg;
};

class SingleGenimager{
public:
    SingleGenimager();

    ~SingleGenimager();

    bool Init(std::shared_ptr<IniManager>& iniManager);

    std::string GetErrString();

    bool BuildTargetFile();

protected:
    bool getBinFiles(std::string& dir, std::string& file_name, std::string& core_files);

    unsigned short getCheckSum16(std::fstream& pfile, unsigned int byteCnt, unsigned short mode);

    unsigned short getCrc16(std::fstream& pfile, unsigned int byteCnt);

private:
    std::shared_ptr<IniManager>         m_ini_manager;
    std::vector<std::string>            m_bin_files;
    std::string                         m_rom_option_file;
    std::string                         err_msg;
};
