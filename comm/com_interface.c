/**
 * file name: com_interface.c
 * this file for common interface
 *
 * add by fanshuming@sivalley.com
 * date:2021-04-23
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "com_interface.h"

void getFileNameWithoutSuffix(const char* filename, char * filename_without_suffix)
{
    	unsigned int num = 0;
	const char *file = filename;


	if((NULL == filename) || (NULL == filename_without_suffix)){
		printf("%s:line:%d\n", __FUNCTION__, __LINE__);
		return;
	}
    	while(*file)
	{
        	if (*file == '.'){
            		break;
		}
        	file++;
        	num ++;
    	}

        memset(filename_without_suffix, 0, sizeof(filename_without_suffix));
    	strncpy(filename_without_suffix, filename, num);

    	printf("filename is %s, file name without suffix is:%s\n", filename, filename_without_suffix);
	return;
}

void getFileNameSuffix(const char *filename, char * suffix)
{
	unsigned int filelen = 0;
	unsigned int suffixlen = 1;
	if((NULL == filename) || (NULL == suffix)){
		printf("%s:line:%d error!\n", __FUNCTION__, __LINE__);
		return;
	}

	filelen = strlen(filename);
	while(filename[filelen - suffixlen])
	{
		if(filename[filelen - suffixlen] == '.'){
			break;
		}
		suffixlen++;
		if(suffixlen >= (filelen -1)){
			printf("%s:line:%d file may have not suffix! error!\n", __FUNCTION__, __LINE__);
			return;
		}
	}

	strncpy(suffix, filename+filelen-suffixlen, suffixlen);

	return;
}

#if 0
void running_process(char *file_name)
{
	char cmd[64]={0};
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd,"chmod a+x %s",file_name);
	system(cmd);
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd,"./%s &",file_name);
	system(cmd);
}
#endif

int fileIsExcutable(const char *filename)
{
	char suffix[20] = {0};

	getFileNameSuffix(filename, suffix);
	printf("The file:%s's suffix is %s\n", filename, suffix);
	if(strcmp(suffix, ".elf") == 0){
		printf("this file is a elf file.\n");
		return 1;
	}else if(strcmp(suffix, ".zip") == 0)
	{
		printf("this file is a zip file.\n");
		return 2;
	}else{
		printf("%s:line:%d file's suffix is not xx.elf or xx.zip! error!\n", __FUNCTION__, __LINE__);
		return -1;

	}

}


void unzip_compiler_process(const char* file_name, char * excutable_file)
{
	char filename_without_suffix[100] = {0};
	char compileCmd[100] = {0};
	char compileResult[100] = {0};
	char compileInfo[256] = {0};
	FILE *pfd = NULL;
	FILE *pfd1 = NULL;

        bzero(filename_without_suffix, 100);
        bzero(compileCmd, 100);
	getFileNameWithoutSuffix(file_name, filename_without_suffix);

        sprintf(compileCmd, "sh compile.sh %s", filename_without_suffix);
	printf("compileCmd:%s\n",compileCmd);
	pfd = popen(compileCmd, "r");
	if(pfd == NULL){
		printf("popen %s failed!\n", compileCmd);
		exit(1);
	}

        bzero(compileInfo, 256);
	while(fgets(compileInfo, 200, pfd) != NULL){
		printf("%s\n", compileInfo);
	}
	pclose(pfd);

        bzero(compileCmd, 100);
        sprintf(compileCmd, "cat %s.log | awk \'END {print}\'", filename_without_suffix);
	pfd1 = popen(compileCmd, "r");
	if(pfd1 == NULL){
		printf("popen %s failed!\n", compileCmd);
		exit(1);
	}

	while(fgets(compileResult, 100, pfd1) != NULL){
		printf("build result:%s\n", compileResult);
		if(strncmp(compileResult, "failed", 6) == 0){
			printf("build failed!\n");
			return;
		}else if(strncmp(compileResult, "success", 7) == 0){
			memcpy(excutable_file, filename_without_suffix, strlen(filename_without_suffix));
			strcat(excutable_file, ".elf");
			printf("Generate excutable file:%s\n",excutable_file);
			printf("build success!\n");
		}else{
			printf("warning:something wrong.\n");
			return;
		}
	}

	pclose(pfd1);

	return;
}
