#include <iostream>
#include "eternity_common/CpkFile.h"

class CpkPatcher
{
private:
    bool _report_progress;
    bool _report_errors;
    bool _check_for_nested;

    bool _cpk_loaded;
    CpkFile* _cpk;

    std::map<std::string, CpkPatcher*> _nested_cpks;

    bool GetNestedCpk(const std::string& path_in_cpk, std::string& nested_cpk_path, CpkPatcher*& nested_cpk);
    bool SaveNestedCpks();

#define pout \
    if (!_report_progress) {} \
    else std::cout

#define eout \
    if (!_report_errors) {} \
    else std::cerr << "Error: "
public:
    CpkPatcher(bool report_progress = false, bool report_errors = true, bool check_for_nested_cpk = false);
    ~CpkPatcher();

    bool LoadCpk(const std::string& path);
    bool LoadCpk(uint8_t* buffer, size_t size);

    bool SaveCpk(const std::string& path);
    uint8_t* SaveCpk(size_t* size);

    bool PatchFile(const std::string& path, uint32_t id_in_cpk);
    bool PatchFile(const std::string& path, const std::string& path_in_cpk);

    bool PatchFromDirectory(const std::string& patch_directory);
    bool PatchFromDirectoryByID(const std::string& patch_directory);
};
