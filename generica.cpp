#include "generica.hpp"

Generica::Generica()
{
	
}

/**
* Check the existance of a path
* 
* Given a path (directory or file) check its existance, otherwise throw an exception
* 
* @param (filesystem::path) the path to check
* @throws runtime_error
*/
bool Generica::checkExistance(const filesystem::path& _path)
{
	bool exist = false;

	if (filesystem::exists(_path))
	{
		exist = true;
	}
	else
	{
		throw runtime_error(
			"The specified path: "
			+ string(_path.string())
			+ " does not exists.\nQuitting...");
	}

	return exist;
}

/**
* Check the existance of a path
*
* Given a path, check if it is a file, otherwise throw an exception
*
* @param (filesystem::path) the path to check
* @throws runtime_error
*/
bool Generica::pathIsFile(const filesystem::path& _path)
{
	bool is_file = false;

	if (filesystem::is_regular_file(_path))
	{
		is_file = true;
	}
	else
	{
		throw runtime_error(
			"The specified path: "
			+ string(_path.string())
			+ " is not a file.\nQuitting...");
	}

	return is_file;
}

/**
* Simple way to return a bool
* 
* If the input is 0, false is returned. For any other integer values, true is returned
* 
* @param (int) val: the input value
* @return (bool)
*/
bool Generica::intToBool(const int& _val)
{
	bool ret = false;

	if (_val == 0)
		ret = false;
	else
		ret = true;

	return ret;
}

/**
* Splitting a given path
*
* Given a path described by filesystem::path (C++17 only), the function splits it into parts.
* It splits the full path into: dirname, basename, filename, extension
*
* @param (filesystem::path) inserted path value
* @param (Tpath) structure with sub paths
*
* @see [filesysem](https://docs.microsoft.com/it-it/cpp/standard-library/filesystem?view=msvc-160)
* @see [filesystem::path](https://docs.microsoft.com/it-it/cpp/standard-library/path-class?view=msvc-160)
*/
void Generica::splitPath(const filesystem::path& _path, Tpath& _tpath)
{
	try
	{
		Generica::checkExistance(_path);
		Generica::pathIsFile(_path);
	}
	catch (const runtime_error& msg)
	{
		cerr << "ERR::" << msg.what() << endl;
		exit(-1);
	}

	_tpath.full_filename = _path.string();
	_tpath.dirname = _path.parent_path().string();
	_tpath.basename = _path.filename().string();
	int found = _tpath.basename.find_last_of(".");
	_tpath.filename = _tpath.basename.substr(0, found);
	_tpath.ext = _path.extension().string();
}

/**
* Check boolean string
* 
* Given an input string, it returns true (bool 1) if the string is
* equal to one out of ("true", "yes", "y", "1"); false otherwise.
* 
* @param (string) str is the input string to compare
* @return (bool) true (1) or false (0)
* 
* @see [tolower](http://www.cplusplus.com/reference/locale/tolower/)
*/
bool Generica::str2Bool(const string& _str)
{
    locale loc;
    vector<string> solution { "true", "yes", "y", "1" };
    string to_low = _str;
    bool res = false;
    
    for (int i = 0; i < _str.length(); ++i)
        to_low[i] = tolower(_str[i], loc);

    int i = 0;
    while (i < solution.size() && res == false)
    {
        if (to_low.compare(solution[i]) == 0)
            res = true;

        i += 1;
    }

    return res;
}

/**
* Create a csv file for the specified feature
* 
* Given the feature you want to track, this function creates a <video_filename>_meta directory
* that will contain the CSV file.
* 
* @param (Tpath) path to the input video file
* @param (string) name of the feature
* @return (string) path to the new csv file
*/
string Generica::makeCSV(ofstream& _csv_file, Tpath& _tpath, const string& _feature_name, const int patch_info[])
{
	filesystem::path container_dir = filesystem::u8path(_tpath.dirname) / filesystem::u8path(_tpath.filename);
	container_dir += "_meta";

	filesystem::create_directory(container_dir);
	string csv_path = (container_dir / filesystem::u8path(_feature_name) += ".csv").string();

	// write header
	_csv_file.open(csv_path);
	
	if (_feature_name.compare("blur") == 0)
	{
		_csv_file << "frame_n";

		for (int y = 0; y < patch_info[1]; y++)
			for (int x = 0; x < patch_info[0]; x++)
				_csv_file << ",blur_" + to_string(y) + to_string(x) + ",var_" + to_string(y) + to_string(x);
		
		_csv_file << endl;
	}
	else
	{
		_csv_file << "frame_n," << _feature_name << endl;
	}

	_csv_file.close();

	return csv_path;
}


/**
* Get width according to original proportion
* 
* In order to properly resize the window or the frame, the original ratio must be
* preserved. Once the ratio is found, the new value for the height is passed and
* the new width is returned.
* 
* @param (double) _old_w: the old width value
* @param (double) _old_h: the old height value
* @param (int) _new_h: the desired value for the new picture's height
* @return (int) new_w
*/
int Generica::getNewW(const double& _old_w, const double& _old_h, const int& _new_h)
{
	double ratio = _old_w / _old_h;
	return static_cast<int>(ceil(ratio * _new_h));
}


ostream& operator<<(ostream& _os, const Tpath& _tpath)
{
	return _os << "full_filename: " << _tpath.full_filename << endl
				<< "dirname: " << _tpath.dirname << endl
				<< "filename: " << _tpath.filename << endl
				<< "extension: " << _tpath.ext << endl;
}
