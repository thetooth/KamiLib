#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <algorithm>

char* getOption(char ** begin, char ** end, const std::string & option){
	char ** itr = std::find(begin, end, option);
	if(itr != end && ++itr != end){
		return *itr;
	}
	return nullptr;
}

bool optionExists(char** begin, char** end, const std::string& option){
	return std::find(begin, end, option) != end;
}

int main(int argc, char *argv[]){
	char tBuffer[1024] = {};
	int version = 0;

	FILE *fp = fopen("build.ver", "r+");
	if (fp != NULL){
		fseek(fp, 0, SEEK_END);
		int readSize = ftell(fp);
		rewind(fp);
		fread(tBuffer, 1, readSize, fp);
		version = atoi(tBuffer);
	}

	version++;
	printf("Revision: %d", version);

	sprintf(tBuffer, "%d", version);
	rewind(fp);
	fwrite(tBuffer, 1, strlen(tBuffer), fp);
	fclose(fp);

	char* name = getOption(argv, argv+argc, "--name");
	if (name == nullptr){
		name = "APP";
	}

	fp = fopen("version.h", "w+");
	sprintf(tBuffer, "#define %s_BUILD_VERSION %d\n#define %s_BUILD_TIME 0x%08x", name, version, name, time(NULL));
	fwrite(tBuffer, 1, strlen(tBuffer), fp);
	fclose(fp);

	//system("pause");

	return 0;
}