// main.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <chrono>

#include "videostream.hpp"
#include "parser.hpp"

using namespace std;

int main(int argc, char* argv[])
{
    // define settings path
    if (argc < 1)
    {
        cerr << "ERR::No path to settings.json file specified. Quitting..." << endl;
        exit(-1);
    }
        
    filesystem::path full_settings_file = filesystem::u8path(argv[1]);

    try
    {
        Generica::checkExistance(full_settings_file);
        Generica::pathIsFile(full_settings_file);
    }
    catch (const runtime_error& msg)
    {
        cerr << "ERR::" << msg.what() << endl;
        exit(-1);
    }
    
    // parse file to retrieve input arguments
    Parser pars = Parser(full_settings_file.string());
    pars.parseJson();
    Targuments args = pars.getArgs();

    if (args.debug)
    {
        cout << "DEBUG::full settings file: " << full_settings_file.string() << endl;
        cout << endl << "DEBUG::args" << endl << pars;
    }
        
    // video processing
    auto start_time = chrono::high_resolution_clock::now();
    
    Videostream vs = Videostream(args.video_path);
    vs.processing(args);

    auto stop_time = chrono::high_resolution_clock::now();
    auto exec_time = chrono::duration_cast<chrono::seconds>(stop_time - start_time);
    cout << endl << "DEBUG::time = " << exec_time.count() << " seconds" << endl;

    // EOP
    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
