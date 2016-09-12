#/usr/bin/env bash

cd $(readlink -f $(dirname ${0}))/src

for _file in *.{c,h}pp; do
  echo "- formatting ${_file}"
  bcpp -bcl -i 2 -s -tbcl -ybbi ${_file} ${_file}.new
  mv -f ${_file}.new ${_file}
done
