#include "fileoperations.h"
#include <fstream>


ttvfs::VFSHelper vfs;

void init_fileio()
{
    // Make the VFS usable
    vfs.Prepare();
}

void shutdown_fileio()
{

}

std::string to_lowercase(std::string s)
{
	std::string ret;
	for(int i = 0; i < s.length(); i++)
		ret.push_back(tolower(s[i]));
	return ret;
}

std::deque<std::string> get_files_from_dir_rec(std::string sFolderName)
{
    std::set<std::string> lFileFilters;
    return get_files_from_dir_rec(sFolderName, lFileFilters);
}

std::deque<std::string> get_files_from_dir_rec(std::string sFolderName, std::set<std::string> lFileFilters)
{
    ttvfs::StringList lFilenames;

		//Recursively get the filenames of all files contained within this folder
		ttvfs::StringList lFolders;
		ttvfs::GetDirList(sFolderName.c_str(), lFolders, -1);
		if(!lFolders.empty())
		{
			for(ttvfs::StringList::iterator i = lFolders.begin(); i != lFolders.end(); i++)
				ttvfs::GetFileList(i->c_str(), lFilenames);
		}


    if(!lFileFilters.empty() && !lFilenames.empty())
    {
        for(ttvfs::StringList::iterator i = lFilenames.begin(); i != lFilenames.end(); i++) //TODO: Can hang here quite a while...
        {
            size_t pos = i->find_last_of('.');
            if(pos != std::string::npos && pos != i->length()-1)
            {
                std::string substr = i->substr(pos+1);   //Get filename extension of song
                if(!lFileFilters.count(to_lowercase(substr)))
                    i->clear();    //Remove from list if it doesn't pass our filter
            }
            else
                i->clear();
        }
    }
    return lFilenames;
}

std::set<std::string> get_filetypes_supported()
{
    std::set<std::string> ret;
    ret.insert("mp3");
    ret.insert("wma");
    ret.insert("ogg");
    ret.insert("flac");
    ret.insert("opus");
    ret.insert("m4a");
    ret.insert("wav");
    ret.insert("ay");
    ret.insert("gbs");
    ret.insert("gym");
    ret.insert("hes");
    ret.insert("kss");
    ret.insert("nsf");
    ret.insert("nsfe");
    ret.insert("sap");
    ret.insert("spc");
    ret.insert("vgm");
    ret.insert("vgz");

    return ret;
}
