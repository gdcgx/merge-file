#include <memory>
#include "Genimager.h"
#include "IniManager.h"
#include "inifile.h"

namespace fs = std::filesystem;

void showUsage(std::string str)
{
    std::cout << "Error: don't get the configuration file!\n"
              << "Useage: " << str << " -cfg xxx.ini\n"
              << "Options:\n"
              << "    -cfg: the option specifying the configuration file\n";
}

std::string checkCfgPathExits(std::string &cfg_file)
{
    fs::path currentPath = fs::current_path();
    currentPath = currentPath / "cfg";
    if (!fs::exists(currentPath))
    {
        std::cout << "Error: Have no this dir: " << currentPath.string() << std::endl;
        return "";
    }
    for (const auto &entry : fs::directory_iterator(currentPath))
    {
        if (entry.path().filename().compare(cfg_file) == 0)
        {
            return entry.path().string();
        }
    }
    std::cout << "Error: No this file: " << (currentPath / cfg_file) << std::endl;
    return "";
}

void checkAndCreateBinPath()
{
    fs::path dir = fs::current_path();
    if (!fs::exists(dir / "bin"))
    {
        std::cout << "Info: No bin filepath,now create..." << std::endl;
        fs::create_directory(dir / "bin");
    }
}

int main(int argc, char **argv)
{
    std::cout << "****----------------------merge bin start----------------------****" << std::endl;
    int i = 1;
    // std::string ini_path = "./cfg/make_sbl_burner_head_type_daulcore_demo.ini";
    // ini_path = checkCfgPathExits(ini_path);
    checkAndCreateBinPath();

    std::string ini_path;
    if (argc <= 1)
    {
        showUsage(argv[0]);
        return -1;
    }
    else
    {
        std::string option = argv[1];
        if (option.compare("-cfg") == 0)
        {
            std::string path = argv[2];
            ini_path = checkCfgPathExits(path);
        }
        else
        {
            showUsage(argv[0]);
            return -1;
        }
    }

    if (ini_path.empty())
    {
        std::cout << "Error: Cfg Path Is Empty!" << std::endl;
        return -1;
    }

    inifile::IniFile ini_parser;
    if (ini_parser.Load(ini_path) != 0)
    {
        std::cout << "Error: " << ini_parser.GetErrMsg() << std::endl;
        return -1;
    }

    auto iniManager = std::make_shared<IniManager>();
    if(!iniManager->Init(ini_parser))
    {
        std::cout << iniManager->GetErrString() << std::endl;
        return -1;
    }
    // iniManager->Show();

    if(iniManager->m_head_type == 0)
    {
        auto builder = SingleGenimager();
        if(!builder.Init(iniManager))
        {
            std::cout << builder.GetErrString() << std::endl;
            return -1;
        }
        if(!builder.BuildTargetFile())
        {
            std::cout << builder.GetErrString() << std::endl;
            return -1;
        }
    }
    else
    {
        auto builder = DualGenimager();
        if(!builder.Init(iniManager))
        {
            std::cout << builder.GetErrString() << std::endl;
            return -1;
        }
        if(!builder.BuildTargetFile())
        {
            std::cout << builder.GetErrString() << std::endl;
            return -1;
        }
    }
    std::cout << "****-----------------------merge bin end-----------------------****" << std::endl;
    return 0;
}