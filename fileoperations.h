#ifndef FILEOPERATIONS_H
#define FILEOPERATIONS_H

#include "VFS.h"
#include "VFSTools.h"
#include <list>
#include <string>
#include <set>

void init_fileio();
void shutdown_fileio();

std::deque<std::string> get_files_from_dir_rec(std::string sFolderName);
std::deque<std::string> get_files_from_dir_rec(std::string sFolderName, std::set<std::string> lFileFilters);

std::set<std::string> get_filetypes_supported();

























#endif
