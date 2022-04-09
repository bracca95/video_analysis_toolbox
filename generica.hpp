#ifndef __GENERICA_H__
#define __GENERICA_H__

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <locale> // string to bool
#include <filesystem>
#include <vector>
#include <math.h> // ceil/floor

#define ROI_LEN 4
#define GRID_EL 2	// n_patch x, n_patch y

using namespace std;

typedef struct
{
	string full_filename;
	string dirname;
	string basename;
	string filename;
	string ext;
}Tpath;

ostream& operator<<(ostream& _os, const Tpath& _tpath);

typedef struct
{
	Tpath video_path;
	bool blur, exposure, entropy, motion;
	bool debug, show;			// debug print stdout stderr, show video
	vector<int> blur_roi;		// (4) x,y,w,h
	vector<int> patch_grid;		// (2) n_patch x, n_patch y
}Targuments;

class Generica
{	

public:
	// Constructor
	Generica();

	// methods
	static bool checkExistance(const filesystem::path& _path);
	static bool pathIsFile(const filesystem::path& _path);
	static bool intToBool(const int& _val);
	static void splitPath(const filesystem::path& _path, Tpath& _tpath);
	static bool str2Bool(const string& _str);
	static string makeCSV(ofstream& _csv_file, Tpath& _tpath, const string& _feature_name, const int patch_info[]=nullptr);
	static int getNewW(const double& _old_w, const double& _old_h, const int& _new_h);
	
	/** 
	* Template methods must be in the header
	* 
	* @see [SO_1](https://stackoverflow.com/a/972197)
	* @see [SO_2](https://stackoverflow.com/questions/495021/why-can-templates-only-be-implemented-in-the-header-file)
	*/
	template <typename T>
	static void bufferize(vector<T>& t_vec, T& _elem)
	{
		t_vec.erase(t_vec.begin());
		t_vec.push_back(_elem);
	}
};

#endif
