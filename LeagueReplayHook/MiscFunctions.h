#include <Windows.h>
#include <string>
#include <queue>

using namespace std;

vector<string> split(const string& s, const string& delim, const bool keep_empty) {
	vector<string> result;
	if (delim.empty()) {
		result.push_back(s);
		return result;
	}
	string::const_iterator substart = s.begin(), subend;
	while (true) {
		subend = search(substart, s.end(), delim.begin(), delim.end());
		string temp(substart, subend);
		if (keep_empty || !temp.empty()) {
			result.push_back(temp);
		}
		if (subend == s.end()) {
			break;
		}
		substart = subend + delim.size();
	}
	return result;
}

void Error(LPCSTR text, BOOL exit){
	MessageBoxA(0, text, "ERROR", 0);
	if (exit) ExitProcess(-1);
}