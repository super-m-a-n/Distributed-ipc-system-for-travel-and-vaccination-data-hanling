#! /bin/bash

# read arguments
args=("$@")
len=${#args[@]}
int='^[0-9]+$' # integers

if [[ $len -ne 3 ]] # 4 arguments are required
then
	echo "Error : wrong number of args. Use: 4 args"
	exit 1
fi

# check if first argument (inputFile) is valid file
if ! [[ -f ${args[0]} ]]
then
	echo "Error: given inputFile txt does not exist"
	exit 1
else
	inputFile=${args[0]}
fi

# check if second argument (input_dir) is an already existing directory
if [[ -d ${args[1]} ]]
then
	echo "Error: given input directory name already exists"
	exit 1
else
	input_dir=${args[1]}
fi 

# check if third argument (numFilesPerDirectory) is valid integer
if ! [[ ${args[2]} =~ $int ]]
then
  echo "Error: numFilesPerDirectory is not an integer"
  exit 1
else
  numFilesPerDirectory=${args[2]}
fi

# create a new directory with name input_dir
mkdir -p $input_dir
declare -A hash_array # a helpful associative array that saves countries as keys, a file index for that country as value

# read from inputFile, extract all the different countries and mkdir for each country
while IFS= read -r line; do
  	tokens=( $line )				# tokenize a line from inputFile
 	country=${tokens[3]}			# extract the country, which is always after the id, name, surname
 	mkdir -p $input_dir/$country	# make a new sub-directory with that country name. If such sub-directory already exists, then nothing happens
 	hash_array["$country"]=1		# store countries in associative array
done < "$inputFile"

IFS=$'\n'
for country in "${!hash_array[@]}"		# for each country found
do
	country_lines=($(grep "$country" "$inputFile"))		# store lines of given country into an array
	for (( k=0; k<$numFilesPerDirectory; k++))			# round robin, assign each k-th line of given country to k-th .txt of given country
	do
		if [[ $k -eq 0 ]]
		then
			index="$numFilesPerDirectory"
		else
			index="$k"
		fi
		# echo outputs every line/record of given country to awk, which selects every k-th line and outputs it to k-th .txt
		echo "${country_lines[*]}" | awk "NR % $numFilesPerDirectory == $k"	>> "$input_dir/$country/$country-$index.txt"
	done
done