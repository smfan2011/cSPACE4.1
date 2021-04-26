/**
 * header file name: com_interface.c
 * this file for common interface
 *
 * add by fanshuming@sivalley.com
 * date:2021-04-23
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define ZIP_FILE   2
#define ELF_FILE   1

extern void getFileNameWithoutSuffix(const char* filename, char * filename_without_suffix);
extern void getFileNameSuffix(const char *filename, char * suffix);
extern int fileIsExcutable(const char *filename);
extern void unzip_compiler_process(const char* file_name, char * excutable_file);
