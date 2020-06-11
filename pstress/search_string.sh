############################################################################################
# Created by Mohit Joshi, Percona LLC                                                      #
# Creation date: 07-APR-2020                                                               #
#                                                                                          #
# The file is created to report errors/assertions found in error logs during pstress runs. #
# The file uses exclude_patterns.txt to ignore known Bugs/Crashes/Errors.                  #
# Both the files search_string.sh and exclude_patterns must exist in the same directory.   #
# Do not move the files from <pstress-repo>/pstress directory.                             #
############################################################################################

#!/bin/bash

ERROR_LOG=$1
if [ "$ERROR_LOG" == "" ]; then
  if [ -r ./log/master.err ]; then
    ERROR_LOG=./log/master.err
  else
    echo "$0 failed to extract string from an error log, as no error log file name was passed to this script"
    exit 1
  fi
fi

ASSERTION_FLAG=0;
ERROR_FLAG=0;
SEARCH_ERROR_PATTERN="\[ERROR\].*"
SEARCH_ASSERT_PATTERN="Assertion failure:|Failing assertion:|Assertion.*failed|mysqld got signal 11";
SEARCH_FILE=exclude_patterns.txt

# Search assertion string in Error log
PATTERN=$(egrep "$SEARCH_ASSERT_PATTERN" $ERROR_LOG |
         sed -e 's/^.*Assertion failure\:/Assertion failure:/' |
         sed -e 's/^.*Failing assertion\:/Failing assertion:/' |
         sed -e 's/^.*Assertion/Assertion/' |
         sed "s| thread [0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]||");

if [ "$PATTERN" != "" ]; then
  ASSERT_STRING=$PATTERN;
  ASSERTION_FLAG=1;
fi

# Search for errors in the Log, excluding assertion failure error &
# errors when  DML is executed on tables with no PK due to default PXC Strict Mode enabled.

PATTERN=$(egrep "$SEARCH_ERROR_PATTERN" $ERROR_LOG | grep -v "Assertion" |
         grep -v "Percona-XtraDB-Cluster prohibits use of DML" | sed -n -e 's/^.*\[ERROR\] //p');
if [ "$PATTERN" != "" ]; then
  ERROR_STRING=$PATTERN;
  ERROR_FLAG=1;
fi

# Search Assertion string in Known bug list
if [ $ASSERTION_FLAG -eq 1 ]; then
  while IFS= read -r line
  do
    ASSERT_FOUND=0
    while read SigTag
      do
        if [[ $line =~ ${SigTag} ]]; then
          echo "Known Bug reported in JIRA found. Please check the Bug status for more details";
          egrep -B1 "${SigTag}" $SEARCH_FILE
          ASSERT_FOUND=1
          break;
      fi
    done < <(egrep -v '^#|^$' $SEARCH_FILE)
  if [[ $ASSERT_FOUND -eq 0 ]]; then
    echo "$line"
  fi
  done < <(printf '%s\n' "$ASSERT_STRING")
fi

# Search Error string in Known bug list
if [ $ERROR_FLAG -eq 1 ]; then
  while IFS= read -r line
  do
    ERROR_FOUND=0
    while read SigTag
      do
        if [[ $line =~ ${SigTag} ]]; then
          ERROR_FOUND=1;
          break;
        fi
      done < <(egrep -v '^#|^$' $SEARCH_FILE)
  if [[ $ERROR_FOUND -eq 0 ]]; then
    echo "New Error has been found in error log(s). Potentially a Bug, please investigate";
    echo "$line"
  fi
  done < <(printf '%s\n' "$ERROR_STRING")
fi
