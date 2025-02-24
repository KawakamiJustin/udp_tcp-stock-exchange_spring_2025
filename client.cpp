#include <iostream>
#include <string>

using namespace std;

int main()
{
    cout << "[Client] Booting up.\n[Client] Logging in.\n";
    string username = "";
    string password = "";
    cout << "Please enter the username: ";
    cin >> username;
    while(username.length() < 1 || username.length() > 50)
    {
        cout << "Please enter a valid username\n";
        cout << "Please enter the username: \t";
        cin >> username;
    }
    cout << "Please enter the password: ";
    cin >> password;
    while(password.length() < 1 || password.length() > 50)
    {
        cout << "Please enter a valid password\n";
        cout << "Please enter the password: ";
        cin >> password;
    }
    bool validity = false; //set to actual verification in serverM
    if(validity)
    {
        cout << "[Client] You have been granted access. \n";
    }
    return 0;
}
