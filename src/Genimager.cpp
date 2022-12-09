#include "../include/Genimager.h"

#pragma region 双核合并
DualGenimager::DualGenimager()
{
}

DualGenimager::~DualGenimager()
{
}

bool DualGenimager::Init(std::shared_ptr<IniManager> &iniManager)
{
    m_ini_manager = iniManager;
    m_target_file_size = FW_IMAGE_HEADER_SIZE;
    for (auto it : m_ini_manager->m_seg_infos_core0)
    {
        if (!getBinFiles(m_ini_manager->m_input_file_path_core0, it.file_name, m_bin_files_core0))
        {
            return false;
        }
    }
    if (!m_ini_manager->m_seg_infos_core1.empty())
    {
        for (auto it : m_ini_manager->m_seg_infos_core1)
        {
            if (!getBinFiles(m_ini_manager->m_input_file_path_core1, it.file_name, m_bin_files_core1))
            {
                return false;
            }
        }
    }
    return true;
}

bool DualGenimager::BuildTargetFile()
{
    fwImageHeader header{0};
    header.rsvd = 0;

    fs::path binFile = (fs::current_path()/"bin"/m_ini_manager->m_output_file_name).string();
    std::fstream output_file;
    output_file.open(binFile, std::ios::binary | std::ios::out | std::ios::in | std::ios::trunc);
    if (!output_file.is_open())
    {
        err_msg = "Error: create file " + m_ini_manager->m_output_file_name + " faild!";
        return false;
    }

    size_t i,j;
    for (i = 0; i < m_bin_files_core0.size(); i++)
    {
        output_file.seekg(m_target_file_size, std::ios::beg);

        std::ifstream fs;
        fs.open(m_bin_files_core0[i], std::ios::binary);
        fs.seekg(0, std::ios::end);
        int size = fs.tellg();
        int n = size / 512;
        int align_size = (size % 512 == 0)? size: (n + 1) * 512;
        if(align_size > m_ini_manager->m_seg_infos_core0[i].max_length)
        {
            err_msg = "Error: " + m_bin_files_core0[i] + " size > cfg MaxLength!";
        }

        char buf[align_size];
        memset(buf, 0, align_size);
        fs.seekg(0, std::ios::beg);
        fs.read(buf, size);
        fs.close();

        header.fwInfo[i].addr = m_ini_manager->m_seg_infos_core0[i].start_addr;
        header.fwInfo[i].offset = m_target_file_size;
        header.fwInfo[i].size = align_size; 
        printf("Filename: %s [addr = 0x%x  -  offset = 0x%x  -  size = 0x%x]\n"
                , m_ini_manager->m_seg_infos_core0[i].file_name.c_str(), header.fwInfo[i].addr, header.fwInfo[i].offset, header.fwInfo[i].size);

        output_file.write(buf, align_size);
        m_target_file_size += align_size;
    }
    for (j = 0; j < m_bin_files_core1.size(); j++)
    {
        output_file.seekg(m_target_file_size, std::ios::beg);
        
        std::ifstream fs;
        fs.open(m_bin_files_core1[j], std::ios::binary);
        fs.seekg(0, std::ios::end);
        int size = fs.tellg();
        int n = size / 512;
        int align_size = (size % 512 == 0)? size: (n + 1) * 512;
        if(align_size > m_ini_manager->m_seg_infos_core1[j].max_length)
        {
            err_msg = "Error: " + m_bin_files_core1[j] + " size > cfg MaxLength!";
            fs.close();
            return false;
        }
        
        char buf[align_size];
        memset(buf, 0, align_size);
        fs.seekg(0, std::ios::beg);
        fs.read(buf, size);
        fs.close();

        header.fwInfo[i+j].addr = m_ini_manager->m_seg_infos_core1[j].start_addr;
        header.fwInfo[i+j].offset = m_target_file_size;
        header.fwInfo[i+j].size = align_size; 
        printf("Filename: %s [addr = 0x%x  -  offset = 0x%x  -  size = 0x%x]\n"
                , m_ini_manager->m_seg_infos_core1[j].file_name.c_str(), header.fwInfo[i+j].addr, header.fwInfo[i+j].offset, header.fwInfo[i+j].size);

        output_file.write(buf, align_size);
        m_target_file_size += align_size;
    }

    if(m_ini_manager->m_image_version.empty())
    {
        output_file.seekg(FW_IMAGE_HEADER_SIZE, std::ios::beg);
        if(!searchFwString(output_file, header.FW_Image_Ver_Ext))
        {
            err_msg = "Error: Read image version error!";
            return false;
        }
    }
    else
    {
        memcpy((void*)&(header.FW_Image_Ver_Ext[0]), (void *)m_ini_manager->m_image_version.c_str(), SIZEOF_NVME_FIRMWARE_REVISION_STRING);
    }

    header.totalImageSize = m_target_file_size - FW_IMAGE_HEADER_SIZE;
    memcpy((void*)&(header.FW_Image_Ver_Int[0]), (void *)m_ini_manager->m_inter_version.c_str(),SIZEOF_NVME_FIRMWARE_REVISION_STRING);
    std::cout << "FW_Image_Ver_Ext = " << m_ini_manager->m_image_version << "; FW_Image_Ver_Int = " << m_ini_manager->m_inter_version << std::endl;

    // 计算 fw 的 crc16 和 sum16的值，写入 header
    output_file.seekg(FW_IMAGE_HEADER_SIZE, std::ios::beg);
    header.fwCrc16 = getCrc16(output_file, (m_target_file_size - FW_IMAGE_HEADER_SIZE));
    output_file.seekg(FW_IMAGE_HEADER_SIZE, std::ios::beg);
    header.fwSum16 = getCheckSum16(output_file, (m_target_file_size - FW_IMAGE_HEADER_SIZE), 2);

    // 在 header 除 headerCrc16 和 headerSum16 之外，其余字段都已经赋值（保留字段赋0）的情况下将 header 写入目标文件
    output_file.seekg(0, std::ios::beg);
    output_file.write((char*)&header, sizeof(fwImageHeader));

    // 计算 header 的 headerCrc16 和 headerSum16 值
    output_file.seekg(0, std::ios::beg);
    header.headerCrc16 = getCrc16(output_file, FW_IMAGE_HEADER_SIZE);
    output_file.seekg(0, std::ios::beg);
    header.headerSum16 = getCheckSum16(output_file, FW_IMAGE_HEADER_SIZE, 2);

    // 根据其字段在结构体里面的偏移将计算出来的值写入目标文件
    output_file.seekg(offsetof(fwImageHeader, headerCrc16), std::ios::beg);
    output_file.write((char*)&header.headerCrc16, sizeof(unsigned short));
    output_file.seekg(offsetof(fwImageHeader, headerSum16), std::ios::beg);
    output_file.write((char*)&header.headerSum16, sizeof(unsigned short));

    output_file.seekg(0, std::ios::end);
    char cTmp = 0;
    unsigned int fillDummy;
    if (m_target_file_size % ALIGN_SIZE)
    {
        fillDummy = ALIGN_SIZE - (m_target_file_size % ALIGN_SIZE);
    }
    else
    {
        fillDummy = 0;
    }

    if (fillDummy < SECURITY_SIZE)
    {
        fillDummy += ALIGN_SIZE;
    }

    char dummyBuf[fillDummy];
    memset(dummyBuf, 0, sizeof(dummyBuf));
    output_file.write(dummyBuf, sizeof(dummyBuf));

    output_file.close();

    printf("**** Gen sys image done imagesSize = 0x%x ****\n", m_target_file_size);
    printf("**** header crc16=0x%04x sum16=0x%04x ****\n", header.headerCrc16, header.headerSum16);
    printf("**** fw     crc16=0x%04x sum16=0x%04x ****\n", header.fwCrc16, header.fwSum16);

    return true;
}

bool DualGenimager::getBinFiles(std::string &dir, std::string &file_name, std::vector<std::string> &core_files)
{
    fs::path currentPath = fs::current_path();
    currentPath = currentPath.parent_path().parent_path();
    currentPath = currentPath / "objs";
    if (!fs::exists(currentPath))
    {
        err_msg = "Error: Have no this dir: " + currentPath.string();
        return false;
    }
    currentPath = currentPath / dir;
    if (!fs::exists(currentPath))
    {
        err_msg = "Error: Have no this dir: " + currentPath.string();
        return false;
    }
    for (const auto &entry : fs::directory_iterator(currentPath))
    {
        if (entry.path().filename().compare(file_name) == 0)
        {
            core_files.emplace_back(entry.path().string());
            return true;
        }
    }
    err_msg = "Error: no this file: " + dir + "/" + file_name;
    return false;
}

std::string DualGenimager::GetErrString()
{
    return err_msg;
}

bool DualGenimager::searchFwString(std::fstream& pfile, char* res)
{
    char ch;
    unsigned int len, position;
    char strPattern[13];
    strcpy(strPattern, FW_KEYWORD);

    len = strlen(strPattern);
    position = 0;
    while (!pfile.eof())
    {
        pfile.read(&ch, 1);
        if (ch == strPattern[position])
        {
            position++;
            int i = 0;
            if (position == len)
            {
                pfile.read(res, SIZEOF_NVME_FIRMWARE_REVISION_STRING);
                res[SIZEOF_NVME_FIRMWARE_REVISION_STRING] = '\0';
                return true;
            }
        }
        else
        {
            position = 0;
        }
    }
    return false;
}

unsigned short DualGenimager::getCheckSum16(std::fstream &pfile, unsigned int byteCnt, unsigned short mode)
{
    unsigned short data_in, chksum = 0x77ae;
    unsigned int idx;

    for (idx = 0; idx < (byteCnt >> 1); idx++)
    {
        pfile.read((char *)&data_in, sizeof(unsigned short) * 1);
        chksum += data_in;
    }

    if (mode == 1) // 1's complement
    {
        return (~chksum);
    }
    else if (mode == 2) // 2's complement
    {
        return (~chksum + 1);
    }
    else
    {
        return (chksum);
    }
}

unsigned short DualGenimager::getCrc16(std::fstream &pfile, unsigned int byteCnt)
{
    unsigned short crc, i, data_in;
    unsigned int idx = 0;

    crc = 0;
    while (byteCnt--)
    {
        data_in = 0;
        pfile.read((char *)&data_in, 1);
        crc = crc ^ (data_in << 8);

        for (i = 0; i < 8; i++)
        {
            if (crc & 0x8000)
                crc = crc << 1 ^ 0x1021;
            else
                crc = crc << 1;
        }
    }
    return (crc & 0xFFFF);
}
#pragma endregion

#pragma region 单核合并
SingleGenimager::SingleGenimager()
{
}

SingleGenimager::~SingleGenimager() 
{
}

bool SingleGenimager::Init(std::shared_ptr<IniManager> &iniManager)
{
    m_ini_manager = iniManager;
    for (auto it : m_ini_manager->m_seg_infos_core0)
    {
        std::string str;
        if (!getBinFiles(m_ini_manager->m_input_file_path_core0, it.file_name, str))
        {
            return false;
        }
        m_bin_files.emplace_back(str);
    }
    if (!m_ini_manager->m_option_rom_path.empty())
    {
        if (!getBinFiles(m_ini_manager->m_input_file_path_core0, m_ini_manager->m_option_rom_path, m_rom_option_file))
        {
            return false;
        }
    }
    return true;
}

std::string SingleGenimager::GetErrString()
{
    return err_msg;
}

bool SingleGenimager::BuildTargetFile()
{
    binFileHeader header{0};
    header.headByteLen = 64;
    header.flag = m_ini_manager->m_flag;
    //header.header = 0xBEEFCAFE;
    header.header = 0xE41B2708;

    int target_file_size = 64;
    fs::path binFile = (fs::current_path()/"bin"/m_ini_manager->m_output_file_name).string();
    std::fstream output_file(binFile, std::ios::binary | std::ios::out | std::ios::in | std::ios::trunc);
    if (!output_file.is_open())
    {
        err_msg = "Error: create file " + m_ini_manager->m_output_file_name + " faild!";
        return false;
    }

    for (size_t i = 0; i < m_bin_files.size(); i++)
    {
        output_file.seekg(target_file_size, std::ios::beg);
        std::ifstream fs;
        fs.open(m_bin_files[i], std::ios::binary);
        fs.seekg(0, std::ios::end);
        int len = fs.tellg();
        int n = len / 512;
        if (len % 512 != 0)
        {
            n += 1;
        }
        int align_size = 512 * n;
        if(align_size > m_ini_manager->m_seg_infos_core0[i].max_length)
        {
            err_msg = "Error: " + m_bin_files[i] + " size > cfg MaxLength!";
            fs.close();
            return false;
        }

        if (m_bin_files[i].find("_ATCM") != std::string::npos)
        {
            header.codeByteLen = align_size;
        }
        else if (m_bin_files[i].find("_BTCM") != std::string::npos)
        {
            header.dataByteLen = align_size;
        }
        else
        {
            err_msg = "Error: The file [" + m_bin_files[i] + "] name is error!";
            return false;
        }
        char buf[align_size];
        memset(buf, 0, sizeof(buf));
        fs.seekg(0, std::ios::beg);
        fs.read(buf, len);
        output_file.write(buf, sizeof(buf));
        std::cout << m_bin_files[i] << " size = " << len << "; After 512 Byte alignment = " << align_size << std::endl;
        target_file_size += sizeof(buf);
    }

    if (!m_rom_option_file.empty())
    {
        std::fstream fs;
        fs.open(m_rom_option_file, std::ios::binary | std::ios::in);
        fs.seekg(0, std::ios::end);
        header.optionRomByteLen = fs.tellg();
        if (header.optionRomByteLen % 512 != 0)
        {
            err_msg = "Error: OptionRom length not align 512 Byte";
            return false;
        }
        fs.seekg(0, std::ios::beg);
        header.optionRomChkValue = (m_ini_manager->m_flag == 0) ? getCrc16(fs, header.optionRomByteLen) : getCheckSum16(fs, header.optionRomByteLen, 2);
        printf("option rom: %s Size=%d(0x%08x) chkVal=0x%x chkFlag=%d\n", m_rom_option_file.c_str(), header.optionRomByteLen, header.optionRomByteLen, header.optionRomChkValue, m_ini_manager->m_flag);
        fs.seekg(0, std::ios::beg);
        char buf[header.optionRomByteLen];
        memset(buf, 0, sizeof(buf));
        fs.read(buf, header.optionRomByteLen);
        output_file.seekg(target_file_size, std::ios::beg);
        output_file.write(buf, sizeof(buf));
    }

    output_file.seekg(0, std::ios::beg);
    output_file.write((char *)&header, sizeof(binFileHeader));

    // fwChkValue 需要的参数只需要文件头、ATCM、BTCM三个段的大小和，不需要加上optionrom段的大小
    output_file.seekg(0, std::ios::beg);
    header.fwChkValue = (m_ini_manager->m_flag == 0) ? getCrc16(output_file, target_file_size) : getCheckSum16(output_file, target_file_size, 2);

    output_file.seekg(offsetof(binFileHeader, fwChkValue), std::ios::beg);
    output_file.write((char *)&header.fwChkValue, sizeof(unsigned short));
    target_file_size += header.optionRomByteLen;

    output_file.seekg(0, std::ios::end);
    char cTmp = 0;
    unsigned int fillDummy;
    if (target_file_size % ALIGN_SIZE)
    {
        fillDummy = ALIGN_SIZE - (target_file_size % ALIGN_SIZE);
    }
    else
    {
        fillDummy = 0;
    }

    if (fillDummy < SECURITY_SIZE)
    {
        fillDummy += ALIGN_SIZE;
    }

    for (int i = 0; i < fillDummy; i++)
    {
        output_file.write(&cTmp, 1);
    }

    printf("outputFile: %s fwChkVal=0x%04x size=%d(0x%08x)\n", m_ini_manager->m_output_file_name.c_str(), header.fwChkValue, target_file_size, target_file_size);
    output_file.close();
    return true;
}

bool SingleGenimager::getBinFiles(std::string &dir, std::string &file_name, std::string &core_files)
{
    fs::path currentPath = fs::current_path();
    currentPath = currentPath.parent_path().parent_path();
    currentPath = currentPath / "objs";
    if (!fs::exists(currentPath))
    {
        err_msg = "Error: Have no this dir: " + currentPath.string();
        return false;
    }
    currentPath = currentPath / dir;
    if (!fs::exists(currentPath))
    {
        err_msg = "Error: Have no this dir: " + currentPath.string();
        return false;
    }
    for (const auto &entry : fs::directory_iterator(currentPath))
    {
        if (entry.path().filename().compare(file_name) == 0)
        {
            core_files = entry.path().string();
            return true;
        }
    }
    err_msg = "Error: no this file: " + dir + "/" + file_name;
    return false;
}

unsigned short SingleGenimager::getCheckSum16(std::fstream &pfile, unsigned int byteCnt, unsigned short mode)
{
    unsigned short data_in, chksum = 0x77ae;
    unsigned int idx;

    for (idx = 0; idx < (byteCnt >> 1); idx++)
    {
        pfile.read((char *)&data_in, sizeof(unsigned short) * 1);
        chksum += data_in;
    }

    if (mode == 1) // 1's complement
    {
        return (~chksum);
    }
    else if (mode == 2) // 2's complement
    {
        return (~chksum + 1);
    }
    else
    {
        return (chksum);
    }
}

unsigned short SingleGenimager::getCrc16(std::fstream &pfile, unsigned int byteCnt)
{
    unsigned short crc, i, data_in;
    unsigned int idx = 0;

    crc = 0;
    while (byteCnt--)
    {
        data_in = 0;
        pfile.read((char *)&data_in, 1);
        crc = crc ^ (data_in << 8);

        for (i = 0; i < 8; i++)
        {
            if (crc & 0x8000)
                crc = crc << 1 ^ 0x1021;
            else
                crc = crc << 1;
        }
    }
    return (crc & 0xFFFF);
}
#pragma endregion
