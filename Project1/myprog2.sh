#Check the number of input if it is 1
if [ $# -ne 1 ]; then
    echo "Please enter 1 piece input."
    exit 1
fi

#Define input files
giris="giris.txt"
gelisme="gelisme.txt"
sonuc="sonuc.txt"
input_file_name="$1"

if [ -e "$input_file_name" ]; then 
read -p "$input_file_name exists. Do you want it to be modified? (y,n): " user_answer
	if [ "$user_answer" == n ]; then
	echo "There is no change on $input_file_name , exiting..."
	exit 1
	else
	echo "Please enter only 'y' or 'n'!"
	fi
fi


#Implement texts to array
mapfile -t giris < "$giris"
mapfile -t gelisme < "$gelisme"
mapfile -t sonuc < "$sonuc"

#Calculate the number of lines (included blank lines)
giris_count=${#giris[@]}
gelisme_count=${#gelisme[@]}
sonuc_count=${#sonuc[@]}

#For preventing blank lines, divide by 2 of line number
giris_div2=$(( (giris_count + 1) / 2 ))
gelisme_div2=$(( (gelisme_count + 1) / 2 ))
sonuc_div2=$(( (sonuc_count + 1) / 2 ))

#Take even numbers for non-blank lines
random_giris=$(( RANDOM % giris_div2 * 2))
random_gelisme=$(( RANDOM % gelisme_div2 * 2))
random_sonuc=$(( RANDOM % sonuc_div2 * 2))

# Write a random story
{
    echo "${giris[$random_giris]}"
    echo ""
    echo "${gelisme[$random_gelisme]}"
    echo ""
    echo "${sonuc[$random_sonuc]}"
} > "$input_file_name"

echo "A random story is created and stored in $input_file_name."
