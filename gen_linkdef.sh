#!/bin/bash

set -e

function print_usage() {
    echo "Usage: $0 [-l input_Linkdef.h] -o output [inputs...]" 1>&2;
    exit 1;
}

eval set -- "$(getopt -o "o:l:" -n "$0" -- "$@")"

while true; do
	case "$1" in
		-l)
			input_linkdef_path="$2" ;
			shift 2 ;;
		-o)
			output_linkdef_path="$2" ;
			shift 2 ;;
		--)
			shift ;
			break ;;
		*)
			echo "Unknown option '$1'" ;
			print_usage ;;
	esac
done

if [ -z "${output_linkdef_path}" ]; then
	echo "Missing -o"
	print_usage
fi

classes=()
stcontainer_classes=()

for INFILE in "$@"; do
	echo "Scanning $INFILE" >&2
	file_classes=( $(awk 'match($0,"^[[:space:]]*ClassDef[[:space:]]*\\(([^#]+),.*\\)", a) { print a[1] }' "$INFILE") )
	namespace="$(awk 'match($0,"namespace[[:space:]]+(\\w+).*\\$NMSPC", a) { print a[1] }' "$INFILE")"
	if [ -n "$namespace" ]; then
		for i in "${!file_classes[@]}"
		do
			file_classes[i]=$namespace"::"${file_classes[$i]}
		done
	fi
	classes+=( "${file_classes[@]}" )

	# check if array is not empty
	if [ "${#file_classes[@]}" -ne 0 ]; then
		# if there is at least one ClassDef, then use the header file
		echo -n "$INFILE;"
	else
		# take filename without extension as the class name
		header_filename="$(basename "$INFILE")"
		header_classname="${header_filename%.*}"

		# if classname is in the original linkdef, then use the header file
		if [ -n "$input_linkdef_path" ] && grep "$header_classname" "$input_linkdef_path" 1>/dev/null; then
			echo -n "$INFILE;"
		fi
	fi

	stcontainer_classes+=( $(awk 'match($0,"^[[:space:]]*StCollectionDef[[:space:]]*\\(([^#]+)\\)",a) { print a[1] }' "$INFILE") )
done

mkdir -p "$(dirname "$output_linkdef_path")"
echo "#ifdef __CINT__" > "$output_linkdef_path"
echo >> "$output_linkdef_path"
# add "link of all" lines if they are not in the input linkdef
if [ -z "$input_linkdef_path" ] || ! grep -E "pragma[ \t]+link[ \t]+off[ \t]+all" "$input_linkdef_path" 1>/dev/null; then
	echo "#pragma link off all globals;" >> "$output_linkdef_path"
	echo "#pragma link off all classes;" >> "$output_linkdef_path"
	echo "#pragma link off all functions;" >> "$output_linkdef_path"
	echo >> "$output_linkdef_path"
fi
if [ -n "$input_linkdef_path" ]; then
	# reuse lines from input linkdef that match the pattern
	grep -E "#pragma[ \t]+link" "$input_linkdef_path" >> "$output_linkdef_path" || true
fi
echo >> "$output_linkdef_path"
echo >> "$output_linkdef_path"
echo "// Collected dictionary entities" >> "$output_linkdef_path"
echo >> "$output_linkdef_path"
for class in "${classes[@]}"; do
	# do not duplicate lines from the input linkdef if they were already specified
	if [ -z "$input_linkdef_path" ] || ! grep -E "class[ ]+$class[< ;+-]+" "$input_linkdef_path" 1>/dev/null; then
		echo "#pragma link C++ class $class+;" >> "$output_linkdef_path"
	fi
done
echo >> "$output_linkdef_path"
echo >> "$output_linkdef_path"
for class in "${stcontainer_classes[@]}"; do
	echo "#pragma link C++ class StPtrVec${class}-;" >> "$output_linkdef_path"
	echo "#pragma link C++ class StSPtrVec${class}-;" >> "$output_linkdef_path"
done
echo "#endif" >> "$output_linkdef_path"
