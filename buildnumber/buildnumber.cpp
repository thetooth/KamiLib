#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char *argv[]){
	char tBuffer[1024] = {};
	int version = 0;

	FILE *fp = fopen("build.ver", "r");
	if (fp != NULL)
	{
		fseek(fp, 0, SEEK_END);
		int readSize = ftell(fp);
		rewind(fp);
		fread(tBuffer, 1, readSize, fp);
		version = atoi(tBuffer);
	}

	version++;
	printf("Revision: %d", version);
	fclose(fp);

	fp = fopen("build.ver", "w+");
	sprintf(tBuffer, "%d", version);
	fwrite(tBuffer, 1, strlen(tBuffer), fp);
	fclose(fp);

	fp = fopen("version.h", "w+");
	sprintf(tBuffer, "#define APP_BUILD_VERSION %d\n#define APP_BUILD_TIME 0x%08x", version, time(NULL));
	fwrite(tBuffer, 1, strlen(tBuffer), fp);
	fclose(fp);

	//system("pause");

	return 0;
}