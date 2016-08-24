#!/bin/bash -e

# RS Invocation script to FileCheck, used to check generated Java
# files or C++ files. This assumes that the .rs source file has the
# Java package name "foo".

print_help() {
  help_str="Usage: %s --output=<output-dir> \
--filecheck=<path-to-filecheck> \
--lang=[Java/C++] \
<.rs file>\n"

  printf "$help_str" $0
}

for arg in "$@"
do
  case $arg in
  --output=*)
    outdir="${arg#*=}"
    ;;
  --filecheck*)
    filecheck="${arg#*=}"
    ;;
  --lang*)
    lang="${arg#*=}"
    ;;
  --help)
    print_help
    exit 0
    ;;
  *)
    rsfile="$arg"
    ;;
  esac
done

if [[ (-z $outdir) || (-z $filecheck) || (-z $rsfile) ]]
then
  print_help
  exit 1
fi

if [[ ! -f $rsfile ]]
then
  echo "Input file $rsfile doesn't exist"
  exit 1
fi

rsfile_basename=$(basename "$rsfile")

if [[ $lang == "Java" ]]
then
  filecheck_inputfile=foo/ScriptC_${rsfile_basename%.*}.java
elif [[ $lang == "C++" ]]
then
  filecheck_inputfile=ScriptC_${rsfile_basename%.*}.h
else
  echo Unknown language "$lang"
  print_help
  exit 1
fi

if [[ ! -f $filecheck ]]
then
  echo "No file at supplied FileCheck path $filecheck"
  exit 1
fi

if [[ ! -f $outdir/$filecheck_inputfile ]]
then
  echo "Input file $outdir/$filecheck_inputfile doesn't exist"
  exit 1
fi

"$filecheck" -input-file "$outdir"/$filecheck_inputfile "$rsfile"
