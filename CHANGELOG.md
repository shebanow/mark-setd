# Changelog

## Version 2.0 (2025)

### Major Changes
- **Complete C++ rewrite**: Converted from C to modern C++ with classes and improved performance
- **Cloud storage support**: Added universal marks that sync across machines via cloud storage
  - Support for OneDrive, Google Drive, Dropbox, and iCloud
  - Cloud marks are marked with `-cl` option
  - Setup via `-cloud-setup` command
- **Improved space handling**: Fixed handling of spaces in directory names when escaped with backslashes
- **Better database management**: More efficient storage and retrieval of marks and directory queue

### Technical Improvements
- Object-oriented design with separate classes for MarkDatabase, SetdDatabase, CloudStorage
- Use of modern C++ features (C++14): smart pointers, STL containers
- Better error handling and validation
- Improved code maintainability

### Bug Fixes
- Fixed space handling in directory names (escaped backslashes)
- Fixed mark and cd alias functions to properly handle quoted arguments

### Credits
- Original implementation: Sunil William Savkar (1991)
- Modernization and enhancements: Michael Shebanow (2025)

## Version 1.7 (1992)
- Original version by Sunil William Savkar
- Basic mark and setd functionality
- Directory queue support
- Environment variable support

