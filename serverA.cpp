#include <fstream>
#include <string>
#include <iostream>
#include <map>

using namespace std;

int main()
{
    ifstream infile;
    infile.open("members.txt");
    if (!infile.is_open()) {
        cerr << "Server A failed to open members.txt" << endl;
        return 1;
    }
    map<string, string> loginInfo; // creates new map
    string info;
    while(getline(infile, info))
    {
        // cout << info << endl;
        int space = info.find(' '); // finds the space position between username and password in txt doc
        string username = info.substr(0, space);
        // cout << username <<endl;
        string password = info.substr(space + 1);
        // cout << password << endl;
        loginInfo[username] = password; // populates map
    }

    //string test = loginInfo.find("Mary");
    infile.close();

    // Print the contents of the map to test if it's populated
    /*cout << "Contents of loginInfo map:" << endl;
    for (map<string, string>::iterator it = loginInfo.begin(); it != loginInfo.end(); ++it) {
        cout << "Username: " << it->first << ", Password: " << it->second << endl;
    }*/

    return 0;
}
