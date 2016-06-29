/*
 *  This file is part of Dune Legacy.
 *
 *  Dune Legacy is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Dune Legacy is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Dune Legacy.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <FileClasses/FileManager.h>

#include <globals.h>

#include <FileClasses/TextManager.h>

#include <misc/FileSystem.h>

#include <config.h>
#include <misc/fnkdat.h>
#include <misc/md5.h>
#include <misc/string_util.h>

#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iomanip>

FileManager::FileManager(bool saveMode) {
    fprintf(stderr,"\n");
    fprintf(stderr,"FileManager is loading PAK-Files...\n\n");
    fprintf(stderr,"MD5-Checksum                      Filename\n");

    std::vector<std::string> searchPath = getSearchPath();
    std::vector<std::string> FileList = getNeededFiles();

    std::vector<std::string>::const_iterator filenameIter;
    for(filenameIter = FileList.begin(); filenameIter != FileList.end(); ++filenameIter) {

        std::vector<std::string>::const_iterator searchPathIter;
        for(searchPathIter = searchPath.begin(); searchPathIter != searchPath.end(); ++searchPathIter) {
            std::string filepath = *searchPathIter + "/" + *filenameIter;
            if(getCaseInsensitiveFilename(filepath) == true) {
                try {
                    fprintf(stderr,"%s  %s\n", md5FromFilename(filepath).c_str(), filepath.c_str());
                    pakFiles.push_back(std::make_pair<Pakfile*,bool> (new Pakfile(filepath),false));
                } catch (std::exception &e) {
                    if(saveMode == false) {
        				std::remove_if(pakFiles.begin(), pakFiles.end(), [=](std::pair<Pakfile*,bool> & item) { return item.second == false ;} );
        				pakFiles.pop_back();
                       /* while(pakFiles.empty()) {
                            delete pakFiles.back();
                            pakFiles.pop_back();
                        }*/

                        throw std::runtime_error("FileManager::FileManager(): Error while opening " + filepath + ": " + e.what());
                    }
                }

                // break out of searchPath-loop because we have opened the file in one directory
                break;
            }
        }

    }

    fprintf(stderr,"Size %i\n",pakFiles.size());
}

FileManager::~FileManager() {
    std::vector<std::pair<Pakfile*,bool>>::const_iterator iter;
    for(iter = pakFiles.begin(); iter != pakFiles.end(); ++iter) {
         delete iter->first;	 // To call ~Pakfile
         // delete iter->second; // is a constant bool type
     }
    pakFiles.erase(pakFiles.begin(),pakFiles.begin()+pakFiles.size());
    printf ("~FileManager : %d\n",pakFiles.size());

}

bool FileManager::Close(std::string filename) {
	bool ret = false;
	std::vector<std::pair<Pakfile*,bool>>::const_iterator iter;
	for(iter = pakFiles.begin(); iter != pakFiles.end(); ++iter) {
		 if (iter->first->getPakname() == filename) {
	         delete iter->first;	 // To call ~Pakfile
	         // delete iter->second; // is a constant bool type
	         ret=true;
		 }
	}
	iter=std::find_if(pakFiles.begin(), pakFiles.end(), [=](std::pair<Pakfile*,bool> & item) { return item.first->getPakname() == filename ;} );
	pakFiles.erase(iter);

	iter=std::find_if(pakFiles.begin(), pakFiles.end(), [=](std::pair<Pakfile*,bool> & item) { return item.first->getPakname() == filename ;} );
	if (iter != pakFiles.end()){
		 printf ("FileManager::Close partial failure on %s (%s,%s)\n",filename.c_str(), iter->first->getPakname().c_str(),iter->second ? "true": "false");
	}

	return ret;
}

void FileManager::FileManagerCustom(bool saveMode) {
    fprintf(stderr,"\n");
    fprintf(stderr,"FileManager is loading Custom PAK-Files...\n\n");
    fprintf(stderr,"MD5-Checksum                      Filename\n");

    std::vector<std::string> searchPath = getSearchPath();
    std::vector<std::string> FileList = getCustomedFiles();

    std::vector<std::string>::const_iterator filenameIter;
    for(filenameIter = FileList.begin(); filenameIter != FileList.end(); ++filenameIter) {

        std::vector<std::string>::const_iterator searchPathIter;
        for(searchPathIter = searchPath.begin(); searchPathIter != searchPath.end(); ++searchPathIter) {
            std::string filepath = *searchPathIter + "/" + *filenameIter;
            if(getCaseInsensitiveFilename(filepath) == true) {
                try {
                    fprintf(stderr,"%s  %s\n", md5FromFilename(filepath).c_str(), filepath.c_str());
                    pakFiles.push_back(std::make_pair<Pakfile*,bool> (new Pakfile(filepath,true),false));
                } catch (std::exception &e) {
                    if(saveMode == false) {

        				std::remove_if(pakFiles.begin(), pakFiles.end(), [=](std::pair<Pakfile*,bool> & item) { return item.second == true ;} );
        				pakFiles.pop_back();

                       /* while(pakFiles.empty()) {
                            delete pakFiles.back();
                            pakFiles.pop_back();
                        }*/

                        throw std::runtime_error("FileManager::FileManagerCustom(): Error while opening " + filepath + ": " + e.what());
                    }
                }

                // break out of searchPath-loop because we have opened the file in one directory
                break;
            }
        }

    }

    fprintf(stderr,"Size %i\n",pakFiles.size());
}


std::vector<std::string> FileManager::getSearchPath() {
    std::vector<std::string> searchPath;

    searchPath.push_back(DUNELEGACY_DATADIR);
    char tmp[FILENAME_MAX];
	fnkdat("data", tmp, FILENAME_MAX, FNKDAT_USER | FNKDAT_CREAT);
    searchPath.push_back(tmp);

    return searchPath;
}

std::vector<std::string> FileManager::getCustomedFiles() {
    std::vector<std::string> fileList;

    fileList.push_back("LEGACY_CUSTOM.PAK");

    std::sort(fileList.begin(), fileList.end());
    return fileList;
}



std::vector<std::string> FileManager::getNeededFiles() {
    std::vector<std::string> fileList;

    fileList.push_back("LEGACY.PAK");
    fileList.push_back("OPENSD2.PAK");
    fileList.push_back("DUNE.PAK");
    fileList.push_back("SCENARIO.PAK");
    fileList.push_back("MENTAT.PAK");
    fileList.push_back("VOC.PAK");
    fileList.push_back("MERC.PAK");
    fileList.push_back("FINALE.PAK");
    fileList.push_back("INTRO.PAK");
    fileList.push_back("INTROVOC.PAK");
    fileList.push_back("SOUND.PAK");

    std::string LanguagePakFiles = (pTextManager != NULL) ? _("LanguagePakFiles") : "";

    if(LanguagePakFiles.empty()) {
        LanguagePakFiles = "ENGLISH.PAK,HARK.PAK,ATRE.PAK,ORDOS.PAK";
    }

    std::vector<std::string> additionalPakFiles = splitString(LanguagePakFiles);
    std::vector<std::string>::iterator iter;
    for(iter = additionalPakFiles.begin(); iter != additionalPakFiles.end(); ++iter) {
        fileList.push_back(*iter);
    }

    std::sort(fileList.begin(), fileList.end());

    return fileList;
}

std::vector<std::string> FileManager::getMissingFiles() {
    std::vector<std::string> MissingFiles;
    std::vector<std::string> searchPath = getSearchPath();
    std::vector<std::string> FileList = getNeededFiles();

    std::vector<std::string>::const_iterator filenameIter;
    for(filenameIter = FileList.begin(); filenameIter != FileList.end(); ++filenameIter) {
        bool bFound = false;

        std::vector<std::string>::const_iterator searchPathIter;
        for(searchPathIter = searchPath.begin(); searchPathIter != searchPath.end(); ++searchPathIter) {
            std::string filepath = *searchPathIter + "/" + *filenameIter;
            if(getCaseInsensitiveFilename(filepath) == true) {
                bFound = true;
                break;
            }
        }

        if(bFound == false) {
            MissingFiles.push_back(*filenameIter);
        }
    }

    return MissingFiles;
}

SDL_RWops* FileManager::openFile(std::string filename) {
	SDL_RWops* ret;
	//fprintf(stdout,"Loading file %s\n",filename.c_str());

    // try loading external file
    std::vector<std::string> searchPath = getSearchPath();
    std::vector<std::string>::const_iterator searchPathIter;
    for(searchPathIter = searchPath.begin(); searchPathIter != searchPath.end(); ++searchPathIter) {

        std::string externalFilename = *searchPathIter + "/" + filename;
        if(getCaseInsensitiveFilename(externalFilename) == true) {
            if((ret = SDL_RWFromFile(externalFilename.c_str(), "rb")) != NULL) {
                return ret;
            }
        }
    }
    // now try loading from pak file
    //std::vector<Pakfile*>::const_iterator iter;
    std::vector<std::pair<Pakfile*,bool>>::const_iterator iter;
    const char * tempfile = {filename.c_str()};
    for(iter = pakFiles.begin(); iter != pakFiles.end(); ++iter) {

    	if (filename == "PlanetDay.bmp" &&  (getBasename((*iter).first->getPakname().c_str()) == "LEGACY_CUSTOM.PAK") ) {
			ret = (*iter).first->openFile(tempfile);
			if(ret != NULL) {
				if ((*iter).second)
					fprintf(stdout,"Loading Custom file %s\n",filename.c_str());
				else
					fprintf(stdout,"Loading file %s\n",filename.c_str());
				return ret;
			} else {
				fprintf(stdout,"Failed opening  file %s in %s\n",filename.c_str(),(*iter).first->getPakname().c_str());
			}
    	} else {
    		ret = (*iter).first->openFile(tempfile);
    		if (ret !=NULL ) return ret ;
    	}


    }

    throw std::runtime_error("FileManager::OpenFile(): Cannot find " + filename + "!");
}
SDL_RWops*  FileManager::addFile(std::string packfile, std::string filename) {
	SDL_RWops* ret, *add=NULL;

	fprintf(stdout,"Try Adding file %s to Pakfile %s\n",filename.c_str(), packfile.c_str());

    // try loading external file
    std::vector<std::string> searchPath = getSearchPath();
    std::vector<std::string>::const_iterator searchPathIter;
    std::string externalFilename;
    for(searchPathIter = searchPath.begin(); searchPathIter != searchPath.end(); ++searchPathIter) {

        externalFilename = *searchPathIter + "/" + filename;
        if(getCaseInsensitiveFilename(externalFilename) == true) {
            if((ret = SDL_RWFromFile(externalFilename.c_str(), "r+b")) != NULL) {
                add=ret;
                break;
            }
        }
    }

    if (exists(filename)) {
    	 throw std::runtime_error("FileManager::addFile(): Already a file by that name in *.PAK : Cannot add " + filename + "!");
    }

    // now try adding from pak file

    // From an external location first
    if (add != NULL) {
    	// pakFiles.push_back(std::make_pair<Pakfile*,bool> (new Pakfile(externalFilename),true));
    	std::vector<std::pair<Pakfile*,bool>>::const_iterator iter;
    	iter=find_if(pakFiles.begin(), pakFiles.end(), [=](std::pair<Pakfile*,bool> item){ return item.first->getPakname() == packfile && item.second == true;});
    	if (iter != pakFiles.end()) {
    		((*iter).first)->addFile(add,filename);
    		printf("Found %s adding %s\n",packfile.c_str(),filename.c_str());
    		return add;
    	} else {
    		throw std::runtime_error("FileManager::OpenFile(): Cannot find " + packfile + "!");
    	}

    }
    // From an already opened packfile location then
    else {

		std::vector<std::pair<Pakfile*,bool>>::const_iterator iter;
		iter=find_if(pakFiles.begin(), pakFiles.end(), [=](std::pair<Pakfile*,bool> item){ return item.first->getPakname() == packfile && item.second == true;});
		if (iter != pakFiles.end()) {
			if((ret = SDL_RWFromFile(packfile.c_str(), "r+b")) != NULL) {
				((*iter).first)->addFile(ret,filename);
			//	((*iter).first)->;
				printf("Found %s adding %s\n",packfile.c_str(),filename.c_str());

				return ret;
			} else {
				throw std::runtime_error("FileManager::OpenFile(): Cannot find " + packfile + "!");
			}
		}

		for(iter = pakFiles.begin(); iter != pakFiles.end(); ++iter) {
				fprintf(stdout,"Point in  %s(%s)\n",(*iter).first->getPakname().c_str(),(*iter).second ? "rw":"r");

		}

    }
   throw std::runtime_error("FileManager::addFile(): Cannot find " + filename + "!");
	//return 0;
}

bool FileManager::exists(std::string filename) const {

    // try finding external file
    std::vector<std::string> searchPath = getSearchPath();
    std::vector<std::string>::const_iterator searchPathIter;
    for(searchPathIter = searchPath.begin(); searchPathIter != searchPath.end(); ++searchPathIter) {

        std::string externalFilename = *searchPathIter + "/" + filename;
        if(getCaseInsensitiveFilename(externalFilename) == true) {
            return true;
        }
    }

    // now try finding in one pak file
    std::vector<std::pair<Pakfile*,bool>>::const_iterator iter;
    for(iter = pakFiles.begin(); iter != pakFiles.end(); ++iter) {
        if((*iter).first->exists(filename) == true) {
            return true;
        }
    }

    return false;
}


std::string FileManager::md5FromFilename(std::string filename) {
	unsigned char md5sum[16];

	if(md5_file(filename.c_str(), md5sum) != 0) {
		throw std::runtime_error("Cannot open or read " + filename + "!");
	} else {

		std::stringstream stream;
		stream << std::setfill('0') << std::hex;
		for(int i=0;i<16;i++) {
			stream << std::setw(2) << (int) md5sum[i];
		}
		return stream.str();
	}
}
