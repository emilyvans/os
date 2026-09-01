#ifndef INCLUDE_FS_VFAT_HPP_
#define INCLUDE_FS_VFAT_HPP_

#include "disk.hpp"

void parse_vfat(Partition *partition);
extern FileSystemType vfat_file_system_type;

#endif // INCLUDE_FS_VFAT_HPP_
