#pragma once
#include <vector>
#include <string>

static const int           FW_IMAGE_HEADER_SIZE                    = 1024;
static const int           SIZEOF_NVME_FIRMWARE_REVISION_STRING    = 8;
static const char*         FW_KEYWORD                              = "FIRMWAREREV_";
static const int           ALIGN_SIZE                              = 4096;
static const int           SECURITY_SIZE                           = 256;
static const int           FW_IMAGE_SECTION_MAX_NUM                = 16;

struct fwStructure
{
    uint32_t offset;
    uint32_t addr;
    uint32_t size;
    uint8_t  cpuId;
    uint8_t  overlayIdx;
    uint16_t rsvd;
};

struct binFileHeader
{
    unsigned int    header;
    unsigned int    codeByteLen;  // 512B aligned
    unsigned int    dataByteLen;  // 512B aligned
    unsigned short  headByteLen;
    unsigned short  fwChkValue;
    unsigned int    optionRomByteLen;
    unsigned short  optionRomChkValue;
    unsigned short  flag;         // bit0: 0: crc16, 1: sum16
    unsigned short  secLen;       // security data legth, 32B aligned
    unsigned short  secVer;       // security version
    unsigned int    rsvd0[9];
};

struct fwImageHeader
{
    fwStructure                 fwInfo[FW_IMAGE_SECTION_MAX_NUM];
    unsigned int                totalImageSize;
    unsigned int                rsvd;
    unsigned short              headerCrc16;
    unsigned short              headerSum16;
    unsigned short              fwCrc16;
    unsigned short              fwSum16;
    char                        FW_Image_Ver_Int[SIZEOF_NVME_FIRMWARE_REVISION_STRING];
    char                        FW_Image_Ver_Ext[SIZEOF_NVME_FIRMWARE_REVISION_STRING];
};

struct cfgSegInfo{
    uint32_t    start_addr;
    uint32_t    max_length;         // 保留字段
    uint8_t     overlay_idx;
    uint8_t     cpu_id;
    std::string file_name;
};