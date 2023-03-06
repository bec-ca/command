#!/bin/bash -eu

repo_dir=$PWD

pkgs=$repo_dir/build/external-packages
rm -rf $pkgs
mkdir -p $pkgs

tmp_dir=$(mktemp -d)
pushd $tmp_dir

curl -L -o bee.tar.gz https://github.com/bec-ca/bee/archive/refs/tags/v1.0.0.tar.gz
tar -xf bee.tar.gz

mv */bee $pkgs/

popd

for file in command/*.cpp; do
  echo "Compiling $file..."
  clang++ -c $(cat compile_flags.txt) $file
done
