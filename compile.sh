#!/bin/bash
compilePrj=$1
compileProj=${compilePrj}
currentPath=`pwd`
DATE=`date "+%Y_%m_%d_%H_%M_%S"`

dirName=`echo ${compileProj%%.*}` 
LogName=${currentPath}/${dirName}.log

echo ${currentPath} > ${LogName}
echo "===============================" >> ${LogName}
echo $1 >> ${LogName}
echo "===============================" >> ${LogName}


echo "xxxxxxxxx enter compilexxxxxxxxxxxxxxxxxxx"  >> ${LogName}

if [ -d "${HOME}/deploy/prj/${dirName}" ]; then
	echo " ${HOME}/deploy/prj/${dirName}"
	rm -rf ${HOME}/deploy/prj/${dirName}
fi

echo "mkdir -p ${HOME}/deploy/prj/${dirName}" >> ${LogName}
mkdir -p ${HOME}/deploy/prj/${dirName}


echo "unzip  ${compileProj} -d ${HOME}/deploy/prj/${dirName}" >> ${LogName}
unzip  ${compileProj} -d ${HOME}/deploy/prj/${dirName}

echo "cd ${HOME}/deploy/prj/${dirName}" >> ${LogName}
cd  ${HOME}/deploy/prj/${dirName}

compilerDefs=`cat defines.txt`
echo "gcc ${compilerDefs}" >> ${LogName}
gcc ${compilerDefs} *.c -lm -o ${dirName}.elf>> ${LogName} 2>&1

build_result=$?
if [ "${build_result}" -eq "1" ];then
	echo "failed" >>  ${LogName}
else
	echo "success" >>  ${LogName}
fi

#${HOME}/deploy/prj/${dirName}/${dirName}.elf &
cp -f ${dirName}.elf ${currentPath} 

cd ${currentPath}

