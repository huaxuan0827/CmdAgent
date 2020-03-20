#!/bin/bash

set -e
RELEASE_FILES="libl0002-0.so*"
EXPORT_FILES="export/"
#echo "input parameter is $0;;;;$1;;;;$2;;;;$3;;;;$TARGET_CC"

function Add2Repo()   #$1 sw_no; $2 ver
{
	mkdir -p --mode=777 $BUILDDIR/$1.$2
	rm -fr $BUILDDIR/$1.$2/*
	for fil in ${RELEASE_FILES} ; do
    	cp -dpf $REPO_ROOT/$1/$2/$fil $BUILDDIR/$1.$2/
	done
}

function GetFromRepo() #$1 sw_no; $2 ver
{
    for fil in ${RELEASE_FILES} ; do
        cp -dpf $BUILDDIR/$1.$2/$fil $INSTALL_LIB/
		cp -dpf $BUILDDIR/$1.$2/$fil $PUBLIC_LIB/
    done
}

case "$3" in
	prepare)
		echo $1 $2 $3
		mkdir -p $PUBLIC_INCLUDE/$1/
		cp -Lrvf $REPO_ROOT/$1/$2/$EXPORT_FILES/* $PUBLIC_INCLUDE/$1/
        ;;

	build)
		pushd $REPO_ROOT/$1/$2
		make CFLAG="-I$PUBLIC_INCLUDE -L$PUBLIC_LIB/"
		Add2Repo $1 $2
		make clean
		popd
		GetFromRepo $1 $2
        ;;
	install)
		rm -rf $REPO_ROOT/$1/$2/release/*
		;;
	clean)
		rm -rf $REPO_ROOT/$1/$2/release/*
		;;    
	*)
		echo "Usage: $0 {prepare|build}"
		exit 1
		;;
esac

exit 0
