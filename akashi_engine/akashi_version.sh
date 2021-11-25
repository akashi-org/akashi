#!/bin/bash

OUTDIR=$1
VERSION_FPATH=$OUTDIR/version.h
MAJOR_VER=$2
MINOR_VER=$3

GIT_REV=$(git rev-parse --short HEAD)
GIT_REV_LINE="#define AKASHI_GIT_REV \"${GIT_REV}\""

# if duplicate found, skip generating the header
if [[ -f $VERSION_FPATH ]]; then
  if grep "$GIT_REV_LINE" "$VERSION_FPATH" > /dev/null; then
      echo '[akashi-version] Found duplicate. Skipping.'
      exit 0
  fi
fi


if [[ ! -e $OUTDIR ]]; then
  mkdir $OUTDIR 
fi

echo '#pragma once' > "$VERSION_FPATH"
echo "#define AKASHI_ENGINE_VERSION_MAJOR ${MAJOR_VER}" >> "$VERSION_FPATH"
echo "#define AKASHI_ENGINE_VERSION_MINOR ${MINOR_VER}" >> "$VERSION_FPATH"
echo ${GIT_REV_LINE} >> "$VERSION_FPATH"

echo "[akashi-version] Version header (${MAJOR_VER}.${MINOR_VER}-${GIT_REV}) generated."
