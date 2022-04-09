#ifndef __PARSER_H__
#define __PARSER_H__

#include <fstream>
#include <map>

#include "json.hpp"		// https://kezunlin.me/post/f3c3eb8/
#include "generica.hpp"

using namespace std;
using json = nlohmann::json;

class Parser
{
private:
	Tpath settings_path;
	Targuments args;

public:
	// Constructors
	Parser();
	Parser(const string& _str);

	// methods::getter/setter
	void parseJson();				// setter
	Targuments getArgs() const;		// getter

	// methods::other methods
	void checkJsonBool(const json& _j, const string& _valname, bool& _bool_arg);
	void checkJsonArray(const json& _j, const string& _valname, vector<int>& _vec, const int& _vec_size);

	// operator overload
	friend ostream& operator<<(ostream& _os, const Parser& _p);
};


#endif
