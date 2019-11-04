#ifndef GIT_COMMIT_TAGGER_H
#define GIT_COMMIT_TAGGER_H

#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include "TString.h"
#include "TNamed.h"

// credit : https://stackoverflow.com/users/58008/waqas
// from : https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-output-of-command-within-c-using-posix

std::string exec(const char* cmd) {
	std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
	if (!pipe) {
		throw std::runtime_error("popen() failed!");
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		result += buffer.data();
	}
	return result;
}

TString gitcommitstr() {
	return TString(exec( "git rev-parse HEAD" ).c_str());
}

void WriteGitCommit( TString name="GitCommit" ){
	TNamed n( name, gitcommitstr() );
	n.Write();
}


#endif