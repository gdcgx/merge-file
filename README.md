# 文件合并 Merge Bin 工具协议约定：

```ini
[BuildType]
Head_Type            =   0               ;  0=romcode_head_type,  1=sbl/burner_code_head_type

[DstInfo]
OutputbinFileName      = Lira_rdk_ramdisk_singlecore
InputbinFilePath_core0 = rdk_ramdisk_singlecore
InputbinFilePath_core1 = rdk_cpu1
crc16orsum16_flag   = 1      ;  romcode_head_type   0:crc16  1:sum16
FW_Image_Ver_Int = 1.0.0
FW_Image_Ver_Ext = 1.0.0

[CORE0_SEG0]        
StartAddr           = 0x00          ;512 byte  alignment
MaxLength           = 0x400         ;512 byte  alignment
FileName            = LIBRA_ATCM.bin

[CORE0_SEG1]        
StartAddr           = 0x500
MaxLength           = 0x400
FileName            = LIBRA_BTCM.bin

[CORE1_SEG0]        
StartAddr           = 0x1000         ;512 byte  alignment
MaxLength           = 0x400          ;512 byte  alignment
FileName            = LIBRA_ATCM.bin

[CORE1_SEG1]        
StartAddr           = 0x1500
MaxLength           = 0x400
FileName            = LIBRA_BTCM.bin

[OPTIONROM]        
FileName            = option.rom
```

BuildType 字段选择生成的 bin 文件头大小，选择 0 生成 64 Byte header 大小的 bin 文件，选择 1 生成 1024 Byte header 大小的 bin 文件。

配置文件应当放在 Genimage.exe 所在的目录下的 "cfg" 子文件夹中，合并完成后生成的 bin 文件会在 Genimage.exe 所在的目录下的 "bin" 文件夹中。

![](D:\software\MarkText\images\2022-11-22-15-08-08-image.png)

若有多个 SEG 段，以 COREX_SEGX 为例，COREX 后缀为 0 或者 1 ，对应 InputbinFilePath_core0 和 InputbinFilePath_core1。SEGX 后缀 X 必须为**从 0 开始**的连续的自然数，工具会从 0 为后缀的 SEG 段开始读取，若 SEG 段后缀不连续或者有同名 SEG 段，工具将会报错退出。

# 1、1024 Byte header 文件合并

## header 头部定义

```cpp
struct fwStructure
{
    uint32_t offset;     // 当前 bin 文件段在合成后的文件的偏移
    uint32_t addr;       // 配置文件配置的 StartAddr 值，工具只做校验不做处理
    uint32_t size;       // 当前 bin 文件按照 512Byte 对齐后的大小，由工具读取计算，补齐段填 0
    uint32_t rsvd;       // 保留字段，默认为 0
};
struct fwImageHeader
{  
    fwStructure                 fwInfo[FW_IMAGE_SECTION_MAX_NUM];                        // 合并的 bin 文件段信息
    unsigned int                totalImageSize;                                          // 合并后按照 4096Byte 对齐后的大小
    unsigned int                rsvd;                                                    // 保留字段默认为 0
    unsigned short              headerCrc16;                                             // headerCrc16 和 headerSum16 默认先置 0，将 header 其他字段全部填值后在计算填入
    unsigned short              headerSum16;                                             // 同上
    unsigned short              fwCrc16;                                                 // 将 bin 文件全部合并后计算值，fwCrc16 和 fwSum16 值计算完毕后再按照 4096Byte 对齐
    unsigned short              fwSum16;                                                 // 同上
    char                        FW_Image_Ver_Int[SIZEOF_NVME_FIRMWARE_REVISION_STRING];  // 内部版本号，配置文件配置，若配置文件为配置则工具会将其设置为 image version（工具版本号）
    char                        FW_Image_Ver_Ext[SIZEOF_NVME_FIRMWARE_REVISION_STRING];  // 工具版本号，配置文件配置，必填。读取后写入头。
};
```

## 配置文件约定

必填字段不可为空

> 1、[DstInfo]
> 
> > (必填) OutputbinFileName -> 生成的 bin 文件的文件名，若该文件已存在则会清空文件内容重新生成。
> > 
> > (必填) InputbinFilePath_core0 -> 需要合成的子文件所在的目录名，默认在 exe 文件所在目录向上 **两级** 的目录寻找名为 "objs" 文件夹，在 "objs" 文件夹下寻找该 InputbinFilePath_core0 目录。
> > 
> > (选填) InputbinFilePath_core1 -> 需要合成的子文件所在的目录名，默认在 exe 文件所在目录向上 **两级** 的目录寻找名为 "objs" 文件夹，在 "objs" 文件夹下寻找该 InputbinFilePath_core1 目录。(若所需子文件在一个目录下，则只需要填 InputbinFilePath_core0 字段即可，InputbinFilePath_core1 字段可以填空。若子文件分别在两个目录下，则 InputbinFilePath_core1 也必填。)
> > 
> > (选填) FW_Image_Ver_Int -> 内部版本号，若填空则工具会默认将其设置为工具版本号
> > 
> > (必填) FW_Image_Ver_Ext -> 工具版本号
> > 
> > (不需要，工具不读取) crc16orsum16_flag
> 
> 2、[COREX_SEGX]
> 
> 对应 [DstInfo] 的 InputbinFilePath_core0 和 InputbinFilePath_core1 两个字段，分别对应两个目录，有几个文件目录就应该有几个 COREX 段，最多只有两个。SEGX 代表对应的具体的子文件，有几个文件就应该有几个 SEGX 段，所有的 SEGX 加起来最多只允许有 16 个。
> 
> > (必填,十六进制) StartAddr -> 代表该 bin 文件内容在 RAM 中的地址，不可以与其他任何 SEGX 段重复
> > 
> > (必填,十六进制) VaildLength -> 代表该 bin 文件所允许的最大长度，若读取出来的内容大于这个长度则会触发报错，且不允许其他 SEGX 段的 StartAddr 值小于或者等任何一个 SEGX 段的 StartAddr + VaildLength 的值，因为会导致其他的 bin 文件内容在 RAM 中内容被覆盖
> > 
> > (必填) FileName -> 代表该 bin 文件的文件名，程序会在 InputbinFilePath_coreX 指定的目录下去找对应的文件并读取。
> 
> 3、[OPTIONROM] 不需要填写，工具不读取

# 2、64 Byte header 文件合并

## header 头部定义

```cpp
struct binFileHeader
{
    unsigned int    header;               // 当前默认 0xE41B2708 
    unsigned int    codeByteLen;          // 512Byte 对齐
    unsigned int    dataByteLen;          // 512Byte 对齐
    unsigned short  headByteLen;          // header的字节数，64Byte
    unsigned short  fwChkValue;           // 合并后的 bin 文件 data 段和 code 段的校验值，工具计算（不包括 header 和 optionrom 的长度和内容）
    unsigned int    optionRomByteLen;     // optiomrom 的字节数，工具读取，若不为512字节对齐则会报错
    unsigned short  optionRomChkValue;    // optionrom 的校验值，工具计算
    unsigned short  flag;                 // 选择校验方式：0: crc16, 1: sum1
    unsigned short  secLen;               // 目前默认置0
    unsigned short  secVer;               // 目前默认置0
    unsigned int    rsvd0[9];             // 目前默认置0，保留字段
};
```

## 配置问价约定

必填字段不可为空

生成 64 Byte header **有且必须有两个** SEG 段，默认是在 CORE0 所属的 SEG 段。且两个 SEG 段必须有且只有一个包含 "ATCM" 子字符串，作为 code。另一个必须包含 "BTCM" 子字符串，作为 data。

> 1、[DstInfo]
> 
> > (必填) OutputbinFileName -> 生成的 bin 文件的文件名，若该文件已存在则会清空文件内容重新生成。
> > 
> > (必填) InputbinFilePath_core0 -> 需要合成的子文件所在的目录名，默认在 exe 文件所在目录向上 **两级** 的目录寻找名为 "objs" 文件夹，在 "objs" 文件夹下寻找该 InputbinFilePath_core0 目录。
> > 
> > (必填) crc16orsum16_flag -> 计算 crc16 还是 sum16 的标志位，0 代表计算 crc16 值，1 代表计算 sum16 值
> > 
> > (不需要，工具不读取) FW_Image_Ver_Int
> > 
> > (不需要，工具不读取) FW_Image_Ver_Ext
> > 
> > (不需要，工具不读取) InputbinFilePath_core1
> 
> 2、[COREX_SEGX]
> 
> 对于生成 64 Byte header 文件来说，只需要 CORE0 即可，即所需文件在一个目录下，工具不会读取 CORE1 开头的段。SEGX 最多只允许有 16 个段。
> 
> > (必填,十六进制) StartAddr -> 工具校验值需要
> > 
> > (必填,十六进制) VaildLength -> 工具校验值需要，不允许其他 SEGX 段的 StartAddr 值小于或者等任何一个 SEGX 段的 StartAddr + VaildLength 的值
> > 
> > (必填) FileName -> 代表该 bin 文件的文件名，程序会在 InputbinFilePath_core0 指定的目录下去找对应的文件并读取。
> 
> 3、[OPTIONROM]
> 
> > (选填) FileName -> 代表该 rom 文件的文件名，程序会在 InputbinFilePath_core0 指定的目录下去找对应的文件并读取。

# 3、配置文件校验

## 公共校验项

> 1、校验 section 段 [BuildType] 是否存在

## 64Byte header 校验项

## 1024Byte header 校验项