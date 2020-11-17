#!/bin/bash
#
# Silicon Labs mbedTLS release package script
#
# carl.amundsen@silabs.com
#
# This script needs to be run from super/util/third_party/mbedtls
# example: ~> cd super/util/third_party/mbedtls
#          ~> ./sl_crypto/scripts/sl_mbedtls_release.sh 1.0.0
#
LC_PACKAGE_PATH="util/third_party/mbedtls"
LC_PACKAGE="mbedtls"
LC_PACKAGE_VERSION="2.2.0"
UC_PACKAGE="MBEDTLS"
CHANGES_FILE="sl_crypto/Changes_sl_mbedtls.txt"
README_FILE="sl_crypto/README.txt"
RELEASE_NAME="gecko_sdk_release"

if [ "$CYGWIN" == "1" ]; then
  NUL='/dev/null'
else
  NUL='NUL'
fi

function Help
{
  echo -e "\033[0;33;40mUsage: `basename $0` <SDK_VERSION>\033[0m"
  echo -e "\033[0;33;40m       Example: `basename $0` 2.4.1\033[0m"
  echo
  echo -e "\033[0;33;40m   or:\033[0m"
  echo -e "\033[0;33;40m       `basename $0` --test for testrun\033[0m"
  exit 1
}

#
# Verify repo is clean
#
function VerifyGitClean
{
  CLEAN=`git status | grep modified`
  if [ "${CLEAN}" != "" ]; then
    if [ "${TESTRUN}" == "1" ]; then
      echo -e "\033[1;31;40mWarning: Git repo is modified, ignored now...!!!\033[0m"
    else
      echo -e "\033[1;31;40mError: Git repo is modified, need clean repo to start\033[0m"
      exit 1
    fi
  fi
}

#
# Verify correct path for running script
#
function VerifyCurrentPath
{
  CWD=`pwd`
  if [ `basename ${CWD}` != "${LC_PACKAGE}" ]; then
    echo -e "\033[1;31;40mError: This script must be run from \"${LC_PACKAGE}\" as, ./scripts/`basename $0` <SDK_VERSION>\033[0m"
    exit 1
  fi
}

#
# Verify Revision and Changes file
#
function VerifyRevision
{
  CHANGES=`grep ${SDK_VERSION}: ${CHANGES_FILE}`
  if [ "${TESTRUN}" == "0" ]; then
    if [ "${CHANGES}" == "" ]; then
      echo -e "\033[1;31;40mError: Missing information in ${CHANGES_FILE} about version ${SDK_VERSION}\033[0m"
      echo -n -e "\033[1;31;40m       Latest version in changes file is\033[0m "
      grep ":" ${CHANGES_FILE} | head -1 | sed -e 's/://'
      exit 1
    fi
    if [ `git tag -l | grep ${RELEASE_NAME}_${SDK_VERSION}` ]; then
      echo -e "\033[1;31;40mError: ${RELEASE_NAME}_${SDK_VERSION} already tagged (and released?)\033[0m"
      exit 1
    fi
  fi
}

#
# Verify existence of readme file
#
function VerifyReadme
{
  if [ ! -f ${README_FILE} ]; then
    echo -e "\033[1;31;40mError: No ${README_FILE} file is present\033[0m"
    exit 1
  fi
}

#
# Make sure repo is Clean
#
function CleanRepo
{
  if [ "${TESTRUN}" == "0" ]; then
    echo -e "\033[0;33;40mCleaning repo\033[0m"
    git clean -dxf
    git checkout -- '*.*'
  fi
}

#
# Make autogen result directory
#
function MakeAutogenDir
{
  if [ ! -d autogen ]; then
    mkdir -p autogen
  fi
}

#
# Update version information in files
#
function UpdateVersionNumbers
{
  if [ "${TESTRUN}" == "0" ]; then
    echo -e "\033[0;33;40mUpdating version information to ${SDK_VERSION}\033[0m"
    find inc -name \*.h | xargs -n1 ../../../utils/bin/fixversion.pl ${SDK_VERSION}
    find src -name \*.c | xargs -n1 ../../../utils/bin/fixversion.pl ${SDK_VERSION}
  fi
}

#
# Pack ZIP file together
#
function MakeZIPFile
{
  echo
  echo -e "\033[0;33;40mPacking ZIP file\033[0m"
  # Remove old file
  rm -rf autogen/${LC_PACKAGE}_${LC_PACKAGE_VERSION}_${SDK_VERSION}.zip
  # Generate new file
  (cd ../../..; \
   zip -qr ${LC_PACKAGE_PATH}/autogen/${LC_PACKAGE}_${LC_PACKAGE_VERSION}_${SDK_VERSION}.zip \
       ${LC_PACKAGE_PATH}/apache-2.0.txt  \
       ${LC_PACKAGE_PATH}/License         \
       ${LC_PACKAGE_PATH}/Makefile        \
       ${LC_PACKAGE_PATH}/README.md       \
       ${LC_PACKAGE_PATH}/ChangeLog       \
       ${LC_PACKAGE_PATH}/DartConfiguration.tcl       \
       ${LC_PACKAGE_PATH}/CMakeLists.txt; \
   zip -qru ${LC_PACKAGE_PATH}/autogen/${LC_PACKAGE}_${LC_PACKAGE_VERSION}_${SDK_VERSION}.zip \
       ${LC_PACKAGE_PATH}/configs       \
       ${LC_PACKAGE_PATH}/doxygen       \
       ${LC_PACKAGE_PATH}/include       \
       ${LC_PACKAGE_PATH}/library       \
       ${LC_PACKAGE_PATH}/programs      \
       ${LC_PACKAGE_PATH}/scripts       \
       ${LC_PACKAGE_PATH}/tests         \
       ${LC_PACKAGE_PATH}/visualc       \
       ${LC_PACKAGE_PATH}/yotta         \
       -x ${LC_PACKAGE_PATH}/configs/.gitignore  \
       ${LC_PACKAGE_PATH}/include/.gitignore  \
       ${LC_PACKAGE_PATH}/include/mbedtls/.gitignore  \
       ${LC_PACKAGE_PATH}/library/.gitignore  \
       ${LC_PACKAGE_PATH}/programs/.gitignore  \
       ${LC_PACKAGE_PATH}/tests/.gitignore  \
       ${LC_PACKAGE_PATH}/yotta/.gitignore;  \
   zip -qu ${LC_PACKAGE_PATH}/autogen/${LC_PACKAGE}_${LC_PACKAGE_VERSION}_${SDK_VERSION}.zip \
       ${LC_PACKAGE_PATH}/sl_crypto/*.*; \
   zip -qru ${LC_PACKAGE_PATH}/autogen/${LC_PACKAGE}_${LC_PACKAGE_VERSION}_${SDK_VERSION}.zip \
       ${LC_PACKAGE_PATH}/sl_crypto/include \
       ${LC_PACKAGE_PATH}/sl_crypto/src \
       -x ${LC_PACKAGE_PATH}/sl_crypto/include/.gitignore  \
       ${LC_PACKAGE_PATH}/sl_crypto/src/.gitignore;  \
  )
}

function CleanUp
{
  :
}

#
# Main script
#
ARGS=$*
TESTRUN="0"

if [ $# -lt 1 ]; then
  Help
fi

#
# Parse command line arguments
#
for x in ${ARGS} ; do

  if [ "${x}" == "--test" ]; then
    TESTRUN="1"
    SDK_VERSION="9.9.9"
    echo -e "\033[0;32;40mTest building ${UC_PACKAGE} Release Version ${SDK_VERSION}\033[0m"
  fi

  if [ "${x}" == "--help" ]; then
    Help
  fi
done

if [ "${TESTRUN}" == "0" ]; then
  echo $1 | egrep '[0-9]{1,2}\.[0-9]{1,2}\.[0-9]{1,2}$' >${NUL} 2>&1
  if [ "$?" != "0" ]; then
    echo "\"$1\" is not a valid version number"
    Help
  fi
  SDK_VERSION="$1"
fi

VerifyCurrentPath
#VerifyGitClean
VerifyRevision
VerifyReadme
#CleanRepo
MakeAutogenDir
#UpdateVersionNumbers
MakeZIPFile
CleanUp

#
# Instruct about manual procedure for tagging and releasing
#
echo -e "\033[0;32;40mNow perform the following manual procedure\033[0m"
echo -e "1) Move the \033[0;32;40mautogen/${LC_PACKAGE}_${LC_PACKAGE_VERSION}_${SDK_VERSION}.zip\033[0m into T:\\\03 Research & Development\\\03050 EFM32 Software Support\\\030503 EFM32_CMSIS"
echo "2) Tag these files and this version:"
echo "   git commit -am \"${UC_PACKAGE} files updated to version ${SDK_VERSION}\""
echo "   git push"
echo "   git tag -a -m \"${RELEASE_NAME}_${SDK_VERSION} ${LC_PACKAGE} package\" ${RELEASE_NAME}_${SDK_VERSION}"
echo "   git push --tags"
echo "   git checkout -- '*.*'"
