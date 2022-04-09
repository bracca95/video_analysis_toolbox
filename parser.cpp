#include "parser.hpp"

Parser::Parser()
{
	this->settings_path.full_filename = "";
	this->settings_path.dirname = "";
	this->settings_path.basename = "";
	this->settings_path.filename = "";
	this->settings_path.ext = "";
}

Parser::Parser(const string& _str)
{	
	Generica::splitPath(filesystem::u8path(_str), this->settings_path);
	this->args.blur_roi = vector<int>(ROI_LEN, -1);
	this->args.patch_grid = vector<int>(GRID_EL, -1);
}

/**
* Parser's setter
*
* Set map from json file.
* It uses json.hpp header
* 
* @see [nlohmann/json](https://github.com/nlohmann/json)
*
*/
void Parser::parseJson()
{
	ifstream json_file(this->settings_path.full_filename);
	json j;
	
	try
	{
		json_file >> j;
	}
	catch (const exception& msg)
	{
		cout << msg.what() << endl;
		cout << "ERR::The file's path is unformatted. Please use '\\\\' if on Windows. Quitting..." << endl;
		exit(-1);
	}
	
	json_file.close();

	// parse video path
	string video_path_str;
	j.at("video_path").get_to(video_path_str);
	Generica::splitPath(filesystem::u8path(video_path_str), this->args.video_path);
	
	// parse booleans
	checkJsonBool(j, "blur", this->args.blur);
	checkJsonBool(j, "exposure", this->args.exposure);
	checkJsonBool(j, "entropy", this->args.entropy);
	checkJsonBool(j, "motion", this->args.motion);
	checkJsonBool(j, "debug", this->args.debug);
	checkJsonBool(j, "show", this->args.show);

	// parse json arrays with check
	checkJsonArray(j, "blur_roi", this->args.blur_roi, ROI_LEN);
	checkJsonArray(j, "patch_grid", this->args.patch_grid, GRID_EL);
}

/**
* Parser's getter
* 
* @return map
*/
Targuments Parser::getArgs() const
{
	return this->args;
}

/**
 * Parse json bool from bool or strings
 * 
 * Check if booleans in settings were specified as strings instead of bool.
 * In any case, a bool is assigned to the variable
 * 
 * @param _j (json): file json
 * @param _valname (string): attribute's name
 * @param _bool_arg (bool): the argument to which assign the value
 */

void Parser::checkJsonBool(const json& _j, const string& _valname, bool& _bool_arg)
{
	if (_j.at(_valname).is_boolean())
	{
		_j.at(_valname).get_to(_bool_arg);
	}
	else if (_j.at(_valname).is_string())
	{
		_bool_arg = Generica::str2Bool(_j.at(_valname));
	}
	else
	{
		cout << "ERR::booleans are either bool or string. Quitting..." << endl;
		exit(-1);
	}
}

/**
 * Check array for ROI
 * 
 * ROI must be specified as json arrays/lists. If that is the case, an integer array is
 * returned. Otherwise: exit with error.
 * 
 * @param _j (json): the json file
 * @param _valname (string): argument's name
 * @param _vec (std::vector<int>): the vector to which assign the value
 * @param _vec_size (int): size of the output vector
 */
void Parser::checkJsonArray(const json& _j, const string& _valname, vector<int>& _vec, const int& _vec_size)
{
	if (!_j.at(_valname).is_array())
	{
		cout << "ERR::Provide input as array only. Quitting..." << endl;
		exit(-1);
	}
	else if (_j.at(_valname).size() != _vec_size)
	{
		cout << "ERR::Array's length must be exactly "<< _vec_size <<". Quitting..." << endl;
		exit(-1);
	}
	else
	{
		// nlohomann requires predefined, correct size for arrays (i.e. arrays cannot be larger)
		// it does not even like dynamic array allocation. Yet it likes vectors: thus use std::vector
		_j.at(_valname).get_to(_vec);
	}
}

/** Custom print
* 
* Access Targument and define a way to print all its elements
* 
* @see [SO](https://stackoverflow.com/a/14696278)
* @see [MS operator overload](https://docs.microsoft.com/it-it/cpp/standard-library/overloading-the-output-operator-for-your-own-classes?view=msvc-160)
* @see [friend function par](https://docs.microsoft.com/it-it/cpp/cpp/friend-cpp?view=msvc-160)
*/
ostream& operator<<(ostream& _os, const Parser& _p)
{
	_os << "video_path::" << endl << _p.args.video_path << endl
		<< "blur: " << _p.args.blur << endl
		<< "exposure: " << _p.args.exposure << endl
		<< "entropy: " << _p.args.entropy << endl
		<< "motion: " << _p.args.motion << endl
		<< "debug: " << _p.args.debug << endl
		<< "show: " << _p.args.show << endl
		<< "blur roi: [";

	for (int i = 0; i < ROI_LEN; i++)
		_os << _p.args.blur_roi[i] << ",";

	_os << "]" << endl

		<< "patch_grid: [";

	for (int i = 0; i < GRID_EL; i++)
		_os << _p.args.patch_grid[i] << ",";

	_os << "]" << endl;

	return _os;
}
