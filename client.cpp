#include <iostream>
#include <string>

using namespace std;

int main()
{
    cout << "Hello Welcome to the Stock Trading Platform\nPlease Enter Your User Name and Password\n";
    string username = "";
    string password = "";
    cout << "Username:\t";
    cin >> username;
    while(username.length() < 5 || username.length() > 50)
    {
        cout << "Please enter a valid Username\n";
        cout << "Username:\t";
        cin >> username;
    }
    cout << "Password:\t";
    cin >> password;
    while(password.length() < 5 || password.length() > 50)
    {
        cout << "Please enter a valid Password\n";
        cout << "Password:\t";
        cin >> password;
    }
    bool validity = false;
    if(validity)
    {
        cout << "Available Commands:\n";
    }
    return 0;
}
