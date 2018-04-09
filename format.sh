#/usr/bin/env bash
#
if [ -z $(which bcpp) ]; then
  echo "* BCPP is not installed!"
  exit 1
fi
#
#
cd $(dirname ${0})
#
for _file in $(find . -type f -iname '*.cpp' -o -iname '*.hpp' -o -iname '*.c' -o -iname '*.h'); do
  echo "- formatting ${_file}"
  bcpp -bcl -i 2 -s -tbcl -ybbi ${_file} ${_file}.new
  mv -f ${_file}.new ${_file}
done
#
