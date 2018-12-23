#include <Core/Containers/Array.h>
#include <Core/String/String.h>
#include <Core/Ptr.h>
#include <IO/IO.h>

using namespace Oryol;

#ifdef ORYOL_WINDOWS
#include <direct.h>
#else
#include <dirent.h>
#endif

#include <string.h>

String processFileURI(String path) {
	path = IO::ResolveAssigns(path);
	StringBuilder s = path;
	if (s.GetSubString(0, 9) == "file:////")
		return s.GetSubString(8, EndOfString);
	//~ else if (s.GetSubString(0, 4) == "http") {
		//~ URL u = s.GetString();
		//~ return u.PathToEnd();
	//~ }
	return path;
}

Array<String> readDirectory(String path) {
	Array<String> contents;
	#if ORYOL_WINDOWS
	WIN32_FIND_DATA data;
	StringBuilder s ;
	s.Append("%s\\*", path.AsCStr());
	HANDLE hFind = FindFirstFile(s.GetString().AsCStr(), &data);      // DIRECTORY
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			contents.Add(data.cFileName);
		} while(FindNextFile(hFind, &data));
		FindClose(hFind);
	}
	else {
		Log::Error("Error opening directory: %s\n",path.AsCStr());
	}
	#else
	DIR *dir;
	struct dirent *entry;
	if ((dir = opendir(path.AsCStr())) != NULL) {
		while ((entry = readdir(dir)) != NULL) {
			contents.Add(entry->d_name);
		}
		closedir(dir);
	}
	else {
		Log::Error("Error opening directory: %s\n",path.AsCStr());
	}
	#endif
	bool changed = true;
	while (changed) {
		changed = false;
		for (int i = 0; i < contents.Size(); i++) {
			path = contents[i];
			if (path.AsCStr()[0] == '.') {
				changed = true;
				contents.Erase(i);
				break;
			}
		}
	}
	return contents;
}
