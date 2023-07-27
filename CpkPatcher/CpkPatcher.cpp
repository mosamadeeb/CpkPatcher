#include <filesystem>
#include "CpkPatcher.h"

CpkPatcher::CpkPatcher(bool report_progress, bool report_errors, bool check_for_nested_cpk)
{
    _report_progress = report_progress;
    _report_errors = report_errors;
    _check_for_nested = check_for_nested_cpk;

    _cpk_loaded = false;
    _cpk = new CpkFile();
}

CpkPatcher::~CpkPatcher()
{
    delete _cpk;
    _cpk_loaded = false;
}

bool CpkPatcher::LoadCpk(const std::string& path)
{
    if (_cpk_loaded)
    {
        delete _cpk;
        _cpk_loaded = false;
    }

    _cpk_loaded = _cpk->LoadFromFile(path, _report_errors);
    return _cpk_loaded;
}

bool CpkPatcher::LoadCpk(uint8_t* buffer, size_t size)
{
    if (_cpk_loaded)
    {
        delete _cpk;
        _cpk_loaded = false;
    }

    if (buffer == nullptr || size <= 0)
        return false;

    _cpk_loaded = _cpk->Load(buffer, size);
    return _cpk_loaded;
}

bool CpkPatcher::SaveNestedCpks()
{
    for (auto iter = _nested_cpks.begin(); iter != _nested_cpks.end(); iter++)
    {
        size_t nested_size;
        auto nested_buffer = iter->second->SaveCpk(&nested_size);

        auto idx = _cpk->FindEntryByPath(iter->first + ".cpk");

        if (!_cpk->SetFile(idx, nested_buffer, nested_size, true))
        {
            eout << "Could not save nested CPK \"" << iter->first << "\"" << std::endl;
            return false;
        }

        pout << "Saved nested CPK \"" << iter->first << "\"" << std::endl;

        delete iter->second;
    }

    _nested_cpks.clear();
    return true;
}

bool CpkPatcher::SaveCpk(const std::string& path)
{
    if (!_cpk_loaded)
        return false;

    if (_check_for_nested)
        SaveNestedCpks();
    
    return _cpk->SaveToFile(path, _report_errors, true);
}

uint8_t* CpkPatcher::SaveCpk(size_t* size)
{
    if (!_cpk_loaded)
        return nullptr;

    if (_check_for_nested)
        SaveNestedCpks();

    return _cpk->Save(size);
}

bool CpkPatcher::PatchFile(const std::string& path, uint32_t id_in_cpk)
{
    if (!_cpk_loaded)
        return false;

    // Cannot check for nested CPKs when patching by ID

    if (!_cpk->SetFile(id_in_cpk, path))
    {
        eout << "Could not patch file " << id_in_cpk << " with path \"" << path << "\"" << std::endl;
        return false;
    }

    pout << "Patched file with ID \"" << id_in_cpk << "\"" << std::endl;
    return true;
}

bool CpkPatcher::PatchFile(const std::string& path, const std::string& path_in_cpk)
{
    if (!_cpk_loaded)
        return false;

    uint32_t idx = _cpk->FindEntryByPath(path_in_cpk);

    if (idx == (uint32_t)-1)
    {
        if (_check_for_nested)
        {
            std::string nested_cpk_path;
            CpkPatcher* nested_cpk;

            if (GetNestedCpk(path_in_cpk, nested_cpk_path, nested_cpk))
            {
                return nested_cpk->PatchFile(path, path_in_cpk.substr(nested_cpk_path.length()));
            }
        }
        
        eout << "Could not find file with path \"" << path_in_cpk << "\"" << std::endl;
        
        return false;
    }

    if (!_cpk->SetFile(idx, path))
    {
        eout << "Could not patch file with path \"" << path << "\"" << std::endl;
        return false;
    }

    pout << "Patched \"" << path_in_cpk << "\"" << std::endl;

    return true;
}

bool CpkPatcher::PatchFromDirectory(const std::string& patch_directory)
{
    if (!_cpk_loaded)
        return false;

    std::filesystem::path dir_entry_parent_path;
    std::unordered_set<std::string> nested_dir_set;

    auto patch_path = std::filesystem::path(patch_directory);
    auto dir_start_pos = patch_path.string().length() + 1;

    for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(patch_path))
    {
        if (dirEntry.is_directory())
        {
            if (_check_for_nested)
            {
                dir_entry_parent_path = std::filesystem::path(dirEntry.path().string().substr(dir_start_pos)).parent_path();

                if (nested_dir_set.contains(dir_entry_parent_path.string()))
                {
                    nested_dir_set.insert(dirEntry.path().string().substr(dir_start_pos));
                }
            }

            continue;
        }

        auto entry_path = dirEntry.path().string();
        auto entry_path_in_cpk = entry_path.substr(dir_start_pos);
        
        bool may_be_nested = true;

        if (_check_for_nested)
        {
            dir_entry_parent_path = std::filesystem::path(entry_path_in_cpk).parent_path();

            if (nested_dir_set.contains(dir_entry_parent_path.string()))
                may_be_nested = false;
        }

        uint32_t idx = _cpk->FindEntryByPath(entry_path_in_cpk);

        if (idx == (uint32_t)-1)
        {
            if (_check_for_nested && may_be_nested)
            {
                std::string nested_cpk_path;
                CpkPatcher* nested_cpk;

                auto nested_dir = dir_entry_parent_path.string();
                nested_dir_set.insert(nested_dir);

                if (GetNestedCpk(entry_path_in_cpk, nested_cpk_path, nested_cpk))
                {
                    if (!nested_cpk->PatchFromDirectory(dirEntry.path().parent_path().string()))
                    {
                        eout << "Could not patch directory of nested CPK \"" << nested_cpk_path << "\"" << std::endl;
                    }
                    else
                    {
                        pout << "Patched directory of nested CPK \"" << nested_cpk_path << "\"" << std::endl;
                    }

                    continue;
                }
            }

            eout << "Could not find file with path \"" << entry_path_in_cpk << "\"" << std::endl;

            continue;
        }

        if (!_cpk->SetFile(idx, entry_path))
        {
            eout << "Could not patch file with path \"" << entry_path << "\"" << std::endl;
            continue;
        }

        pout << "Patched \"" << entry_path_in_cpk << "\"" << std::endl;
    }

    return true;
}

bool CpkPatcher::PatchFromDirectoryByID(const std::string& patch_directory)
{
    if (!_cpk_loaded)
        return false;

    auto patch_path = std::filesystem::path(patch_directory);
    auto dir_start_pos = patch_path.string().length() + 1;

    for (const auto& dirEntry : std::filesystem::directory_iterator(patch_path))
    {
        auto entry_path = dirEntry.path().string();

        if (dirEntry.is_directory())
        {
            if (_check_for_nested)
            {
                std::string nested_cpk_path;
                CpkPatcher* nested_cpk;
                if (GetNestedCpk(entry_path.substr(dir_start_pos), nested_cpk_path, nested_cpk))
                {
                    if (!nested_cpk->PatchFromDirectoryByID(entry_path))
                    {
                        eout << "Could not find patch directory of nested CPK \"" << nested_cpk_path << "\"" << std::endl;
                    }
                }
            }

            continue;
        }

        int idx;
        try
        {
            idx = std::stoi(dirEntry.path().stem().string());
        }
        catch (std::exception e)
        {
            eout << "Skipping file with invalid ID \"" << dirEntry.path().stem().string() << "\"" << std::endl;
            continue;
        }

        if (!_cpk->SetFile(idx, entry_path))
        {
            eout << "Could not patch file " << idx << " with path \"" << entry_path << "\"" << std::endl;
            continue;
        }

        pout << "Patched file with ID \"" << idx << "\"" << std::endl;
    }

    return true;
}

bool CpkPatcher::GetNestedCpk(const std::string& path_in_cpk, std::string& nested_cpk_path, CpkPatcher*& nested_cpk)
{
    uint32_t idx;
    nested_cpk_path = path_in_cpk;

    idx = _cpk->FindEntryByPath(nested_cpk_path + ".cpk");
    while (idx == (uint32_t)-1)
    {
        // Stop once we can't go back any further
        if (nested_cpk_path.find('/') == std::string::npos && nested_cpk_path.find('\\') == std::string::npos)
            break;

        nested_cpk_path = Utils::GetDirNameString(nested_cpk_path);
        idx = _cpk->FindEntryByPath(nested_cpk_path + ".cpk");
    }

    if (idx == (uint32_t)-1)
        return false;

    auto iter = _nested_cpks.find(nested_cpk_path);

    if (iter != _nested_cpks.end())
    {
        nested_cpk = iter->second;
    }
    else
    {
        nested_cpk = new CpkPatcher(_report_progress, _report_errors, _check_for_nested);
        _nested_cpks[nested_cpk_path] = nested_cpk;

        size_t nested_size;
        auto nested_data = _cpk->ExtractFile(idx, &nested_size);

        if (!nested_cpk->LoadCpk(nested_data, nested_size))
        {
            eout << "Could not load nested CPK \"" << nested_cpk_path << "\"" << std::endl;
            return false;
        }

        pout << "Loaded nested CPK \"" << nested_cpk_path << "\"" << std::endl;
    }

    return true;
}
