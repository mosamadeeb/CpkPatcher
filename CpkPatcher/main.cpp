#include <iostream>
#include <filesystem>

#include "argparse/argparse.hpp"
#include "CpkPatcher.h"

#define VERSION "v1.0"

int main(int argc, char* argv[])
{
    argparse::ArgumentParser program("CpkPatcher", VERSION);

    program.add_description("CpkPatcher " VERSION "\nPatches a CPK by updating file data without repacking the entire CPK");

    program.add_argument("source_cpk")
        .help("Path to the source cpk");

    program.add_argument("patch_directory")
        .help("Path to the directory containing the patch files");

    program.add_argument("output_cpk")
        .help("Path where the output cpk will be written");

    program.add_argument("-rp", "--report-progress")
        .help("Print a message after patching each file")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("-re", "--report-errors")
        .help("Print a message when a file could not be patched")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("-n", "--check-for-nested")
        .help("Check for nested CPKs while patching and patch them accordingly")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("-ow", "--overwrite")
        .help("Overwrite output CPK if it exists without prompting")
        .default_value(false)
        .implicit_value(true);

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    std::cout << "cpk_patcher " VERSION << std::endl << std::endl;

    std::string source_cpk = program.get("source_cpk");
    std::string patch_directory = program.get("patch_directory");
    std::string output_cpk = program.get("output_cpk");

    if (!std::filesystem::exists(source_cpk))
    {
        std::cerr << "Source CPK path does not exist!" << std::endl;
        return 1;
    }

    if (!std::filesystem::is_directory(patch_directory))
    {
        std::cerr << "Patch directory does not exist!" << std::endl;
        return 1;
    }

    if (std::filesystem::exists(output_cpk) && program["--overwrite"] == false)
    {
        char prompt;
        std::cout << "Output CPK already exists and will be overwritten. Continue (y/N)? ";
        std::cin >> prompt;

        if (tolower(prompt) != 'y')
        {
            std::cout << "Aborting." << std::endl;
            return 1;
        }
    }

    CpkPatcher* patcher = new CpkPatcher(
        program["--report-progress"] == true,
        program["--report-errors"] == true,
        program["--check-for-nested"] == true);

    std::cout << "Loading CPK...";
    patcher->LoadCpk(source_cpk);
    std::cout << " DONE!" << std::endl;

    std::cout << "Loading patch files...\n" << std::endl;
    patcher->PatchFromDirectory(patch_directory);
    std::cout << "Finished patching!" << std::endl;

    std::cout << "\nSaving output CPK..." << std::endl;
    patcher->SaveCpk(output_cpk);
    std::cout << "\nOutput CPK saved to \"" << output_cpk << "\"!" << std::endl;

    delete patcher;

    return 0;
}
