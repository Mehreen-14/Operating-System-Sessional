#!/bin/bash

rm -r -f targets
rm -r -f temp
#for -v and -noexecute
#######
submissions=$1
targets=$2
tests=$3
answers=$4

v=0
noex=0
p=0 # p=1 ; not executng anything

if [ -z "$1" ] || [ -z "$2" ] || [ -z "$3" ] || [ -z "$4" ]; then
  echo "Usage:"
  echo "./organize.sh <submission folder> <target folder> <test folder> <answer folder> [-v] [-noexecute]"
  echo "-v : verbose"
  echo "-noexecute : do not execute code files"
  exit 1
fi

if [[ "$5" == "-v" ]]; then
    v=1
fi

if [[ "$5" == "-noexecute" ]]; then
    p=1
fi

if [[ "$6" == "-noexecute" ]]; then
    noex=1
fi

#######

if [ "$p" -eq 0 ]; then
    mkdir temp

    #unzipping

    source_folder="submissions" #the source folder containing the zip files
    
    destination_folder="temp" #the destination folder where you want to move the extracted files

    # Unzip files in tree-style directory structure and move them to the destination folder
    find "$source_folder" -type f -name "*.zip" -exec sh -c '
    unzip -j -d "${0%.*}" "$0"     # Unzip file in the same directory without creating subdirectories
    mv "${0%.*}" "$1"               # Move the extracted files to the destination folder
    ' {} "$destination_folder" \;

    #making folders in  targets
    mkdir targets/C
    mkdir targets/Java
    mkdir targets/Python

    source_folder2="temp"
    # Specify the destination folder where i have to create the folders
    destination_folder1="targets/C"
    destination_folder2="targets/Java"
    destination_folder3="targets/Python"

    # Iterate through each folder in the source folder
    for folder in "$source_folder2"/*; do
        # Check if the item is a directory
        if [[ -d "$folder" ]]; then
            # Extract the folder name
            folder_name=$(basename "$folder")

            # Extract the last 7 digits from the folder name as student id
            student_id="${folder_name: -7}"

            # Create the corresponding folder in the destination folder
            mkdir -p "$destination_folder1/$student_id"
            mkdir -p "$destination_folder2/$student_id"
            mkdir -p "$destination_folder3/$student_id"

            #If you use the '-p' option, it ensures that all the parent directories leading up to the specified directory are created if they don't already exist. This allows you to create a directory hierarchy without explicitly creating each parent directory one by one.

            #show organising message for students using -v
            if [ "$v" -eq 1 ]; then
                echo "Organizing files for $student_id"
            fi

            # Copy .c,.py,.java files to the corresponding named folder
            find "$folder" -type f -name "*.c" -exec cp {} "$destination_folder1/$student_id" \;
            find "$folder" -type f -name "*.java" -exec cp {} "$destination_folder2/$student_id" \;
            find "$folder" -type f -name "*.py" -exec cp {} "$destination_folder3/$student_id" \;
        fi
    done

    #remove folders  which don't have .c,.java,.py extension files
    # Specify the folder to process
    main_folder1="targets"

    # Find and remove empty folders
    find "$main_folder1" -type d -empty -delete
    #renaming

    find "$destination_folder1" -type f -name "*.c" -execdir mv {} main.c \;
    find "$destination_folder2" -type f -name "*.java" -execdir mv {} Main.java \;
    find "$destination_folder3" -type f -name "*.py" -execdir mv {} main.py \;
    
    # if -noexecute is used then .c, .java, .py files will be executed
    if [ "$noex" -eq 0 ]; then
        # create .csv file
        echo "student_id,type,matched,not_matched" >targets/result.csv

        # Set the directory where the input an answer text file is located
        input_dir="tests"
        answer_dir="answers"

        # Set the range of input file numbers
        start_num=1
        end_num=3
        # array declaration for matched and not_matched file numbers
        declare -A match
        declare -A not_match

        directories=("targets/C/*" "targets/Java/*" "targets/Python/*")

        for path in ${directories[@]}; do
            type=$(basename $(dirname "$path"))
            student_id=$(basename "$path" '/')
            #show message for students using -v
            if [ "$v" -eq 1 ]; then
                echo "Executing files for $student_id"
            fi
            match["${student_id}"]=0
            not_match["${student_id}"]=0
        done

        # Loop through the file numbers
        for ((num = start_num; num <= end_num; num++)); do
            input_file="${input_dir}/test${num}.txt"
            answer_file="${answer_dir}/ans${num}.txt"

            # Loop through the .out files in the out directory
            for c_file in "$destination_folder1"/*/*.c; do
                dirname=$(dirname "$c_file")
                filename=$(basename "$c_file")
                student_id=$(basename "$(dirname "$c_file")")

                type=$(basename "$(dirname "$(dirname "$c_file")")")
                output_file="${dirname}/out${num}.txt"
                # Run the current .out file with the input file and save the output to the output file
                gcc "$c_file" -o "$destination_folder1"/"$student_id"/main.out
                "$destination_folder1"/"$student_id"/main.out <"$input_file" >"$output_file"
                diff_output=$(diff "$output_file" "$answer_file")
                if [ -z "$diff_output" ]; then
                    ((match["${student_id}"]++))
                else
                    ((not_match["${student_id}"]++))
                fi
            done
            for py_file in "${destination_folder3}"/*/*.py; do
                dirname=$(dirname "$py_file")
                filename=$(basename "$py_file")
                student_id=$(basename "$(dirname "$py_file")")
                type=$(basename "$(dirname "$(dirname "$py_file")")")
                output_file="${dirname}/out${num}.txt"
                # Run the current .py file with the input file and save the output to the output file
                python3 "$py_file" <"$input_file" >"$output_file"
                diff_output=$(diff "$output_file" "$answer_file")
                if [ -z "$diff_output" ]; then
                    ((match["${student_id}"]++))
                else
                    ((not_match["${student_id}"]++))
                fi
            done
            for java_file in "${destination_folder2}"/*/*.java; do
                dirname=$(dirname "$java_file")
                filename=$(basename "$java_file")
                student_id=$(basename "$(dirname "$java_file")")
                type=$(basename "$(dirname "$(dirname "$java_file")")")
                output_file="${dirname}/out${num}.txt"
                # Run the current .class file with the input file and save the output to the output file
                javac "$java_file"
                java -cp "${destination_folder2}"/*/* "$(basename "$java_file" .class)" <"$input_file" >"$output_file"
                diff_output=$(diff "$output_file" "$answer_file")
                if [ -z "$diff_output" ]; then
                    ((match["${student_id}"]++))
                else
                    ((not_match["${student_id}"]++))
                fi
            done
        done
        for path in ${directories[@]}; do
            type=$(basename $(dirname "$path"))
            student_id=$(basename "$path" '/')
            echo "$student_id,$type,${match[$student_id]},${not_match[$student_id]}" >>targets/result.csv
        done
    fi
fi
