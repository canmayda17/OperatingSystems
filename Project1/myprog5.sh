#wildcard as argument
wcard="$1"
#directory; if empty, default is current directory
dir="${2:-.}"

#Function to delete files
delete_files() {
    local temp_wcard="$1"
    local temp_dir="$2"
    
    #Find appropriate word on appropriate file
    find "$temp_dir" -type f -name "$temp_wcard" | while read -r file; do
        #Ask user for delete files step by step
        printf "Do you want to delete $file? (y/n): "
        #We had a problem on taking input correctly, after the searching for hours, we find </dev/tty on a forum and corrupted our code.
        read -r user_answer </dev/tty
        if [[ "$user_answer" == "y" ]]; then
            #Delete file
            rm "$file"
            echo "1 file deleted"
            exit 1
        elif [[ "$user_answer" == "n" ]]; then
            #Don't delete the file and skip to another match
            echo "Your answer is no. $file not deleted."
        else
            #Warning message for inappropriate inputs
            echo "You should answer with 'y' or 'n'. Please try again. Exiting..."
            exit 1
        fi
    done
}

#Calling delete function
delete_files "$wcard" "$dir"
