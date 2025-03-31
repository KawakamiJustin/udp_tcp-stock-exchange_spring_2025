#include <fstream>
#include <string>
#include <iostream>
#include <map>

using namespace std;

map<string, string> onStartUp()
{
    ifstream infile;
    infile.open("members.txt");
    if (!infile.is_open()) {
        cerr << "Server A failed to open members.txt" << endl;
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

    return loginInfo;
}

bool matchCredentials(string user_name, string encrypted_pw, map<string, string> users)
{
    if(users.count(user_name) > 0 && encrypted_pw == users[user_name])
    {
        return true;
    }
    else
    {
        return false;
    }
}

int main()
{
    map<string, string> users = onStartUp();
    string user_name = "";
    string pw = "";
    cout << "enter username: ";
    cin >> user_name;
    cout << endl;
    cout << "enter password: ";
    cin >> pw;
    cout << endl;
    bool status = matchCredentials(user_name, pw, users);
    if(status)
    {
        cout << "login successful" << endl;
    }
    else
    {
        cout << "login failed" << endl;
    }
    
    return 0;
}
