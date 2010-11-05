#!/bin/bash

COMMIT=`git rev-parse --short HEAD`

# Use this line for unstable dev builds
REVISION="2.0.0.${COMMIT}"

# Use this line for releases
#REVISION="2.0.0"

echo $REVISION
