#include <iostream>
#include <string.h>

using namespace std;

int main(int argc, char const *argv[])
{
	string s;

	cout << "Message: ";
	
	while(getline(cin, s)){
		cout << "\t\t\t" << s << endl;
		if(!s.compare("quit")) break;
	}

	return 0;
}