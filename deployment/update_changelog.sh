#!/bin/bash

# parameters
name=$1
if [ -z "$name" ]; then
	echo "Usage: $0 [name]"
	exit 1
fi

# variables
file=$(ls ./*.xml)

# get releases tag
releases=$(awk -v name="$name" -v today="$(date +"%Y-%m-%d")" '
function close_tags() {
	print "\t\t\t\t</ul>"
	print "\t\t\t</description>"
	print "\t\t</release>"
}

BEGIN {
	print "\t<releases>"
	ready = 0
}
$0 ~ "^"name {
	sub("^"name " ", "", $0)
	split($0, tokens, " ")
	version = tokens[1]
	date = tokens[3]
	if(date == "") {
		print "\t\t<release version=\""version"pre\" date=\""today"\"/>"
	}
	else {
		ready = 1
		print "\t\t<release version=\""version"\" date=\""date"\">"
		print "\t\t\t<description>"
		print "\t\t\t\t<ul>"
	}
}
ready && $0 ~ "^-" {
	sub(/^- /, "", $0)
	print "\t\t\t\t\t<li>" $0 "</li>"
}
ready && $0 ~ "^$" {
	close_tags()
	ready = 0
}
END {
	close_tags()
	print "\t</releases>"
}
' ../CHANGELOG)

# update xml file
awk -v releases="$releases" '
BEGIN {
	ignore = 0
}
$0 ~ "^.*<releases>" {
	ignore = 1
	print releases
	next
}
$0 ~ "^.*</releases>" {
	ignore = 0
	next
}
!ignore {
	print
}
' "${file}" > "${file}_new"

# update file
mv "${file}_new" "${file}"
