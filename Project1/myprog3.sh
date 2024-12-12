#Create the writable directory
mkdir -p writable

#For number of files moved to writable
count=0

#Loop all files in the directory
for file in *; do
    #Check if file is writable
    if [[ -f "$file" && -w "$file" ]]; then
    	#Move writable and non-folder files to writable directory
        mv "$file" writable/
        ((count++))
    fi
done

# Step 3: Print the number of files moved
echo "$count files moved to writable directory."
