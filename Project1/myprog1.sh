#Assign variables
input_str="$1"
input_num="$2"

#Check length of string and number
length_str=${#input_str}
length_num=${#input_num}

#Check for appropriate inputs
if [ "$length_num" -ne 1 ] && [ "$length_num" -ne "$length_str" ]; then
    echo "The number must be 1 digit or should be equal to string letter number"
    exit 1
fi

output=""
for (( i=0; i<length_str; i++ )); do
    #Find the char value of string
    str_char="${input_str:$i:1}"
    
    #Take the shift value
    if [ "$length_num" -eq 1 ]; then
        shift_letter=${input_num}
    else
        shift_letter=${input_num:$i:1}
    fi

    #Make the new character on new string
    char_on_ascii=$(printf "%d" "'$str_char")
    new_char_on_ascii=$((char_on_ascii + shift_letter))

    #When passing the "z" turn back to "a" and calculate again
    if [ "$new_char_on_ascii" -gt 122 ]; then
        new_char_on_ascii=$((new_char_on_ascii - 26))
    fi

    #Implement the new value
    new_char=$(printf "\\$(printf "%o" "$new_char_on_ascii")")
    output+="$new_char"
done

#Print the output
echo "$output"
