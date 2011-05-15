#!/bin/bash

if [ -d .git ]; then
  COMMIT=`git rev-parse --short HEAD`

  # Use this line for unstable dev builds
  REVISION="2.0.2.${COMMIT}"

  # Use this line for releases
  #REVISION="2.0.2"

  echo $REVISION > version
fi

cat version
