# CpkPatcher
Tool for patching a Criware CPK archive by updating file data without repacking the entire CPK.

# Features
- Loads and patches a CPK with files from a patch directory.

- Any files existing in the CPK that are not found in the patch directory will remain unmodified. Any unpatched files that had compression will remain compressed.

- Supports patching nested CPKs with `--check-for-nested`. Any folder which exists in the main CPK as a CPK will be loaded and patched.

    ### Example:
    
    For the following CPK structure
    ```
    .
    └── main.cpk/
        ├── file1
        └── sub.cpk/
            └── file2
    ```

    A patch directory for patching `file1` and `file2` should look like this
    ```
    .
    └── <patch_directory>/
        ├── file1
        └── sub/
            └── file2
    ```

# Usage
Basic usage: `CpkPatcher <path_to_source_cpk> <path_to_patch_directory> <output_cpk_path>`

Full help message:

```
Usage: CpkPatcher [--help] [--version] [--report-progress] [--report-errors] [--check-for-nested] [--overwrite] source_cpk patch_directory output_cpk

CpkPatcher v1.0
Patches a CPK by updating file data without repacking the entire CPK

Positional arguments:
  source_cpk              Path to the source cpk
  patch_directory         Path to the directory containing the patch files
  output_cpk              Path where the output cpk will be written

Optional arguments:
  -h, --help              shows help message and exits
  -v, --version           prints version information and exits
  -rp, --report-progress  Print a message after patching each file
  -re, --report-errors    Print a message when a file could not be patched
  -n, --check-for-nested  Check for nested CPKs while patching and patch them accordingly
  -ow, --overwrite        Overwrite output CPK if it exists without prompting
```

# Limitations
Does not compress patched files, although unpatched files that were previously compressed will remain compressed.

Has the same limitations as the [CPK implementation](https://github.com/eterniti/eternity_common/blob/main/Criware/CpkFile.h).
Mainly, **files with a GToc table are not supported**.

# License
Distributed under the MIT License. See `LICENSE.txt` for more information.

# Acknowledgments
- [argparse](https://github.com/p-ranav/argparse)
- [eternity_common](https://github.com/eterniti/eternity_common) for the Criware CPK implementation.
