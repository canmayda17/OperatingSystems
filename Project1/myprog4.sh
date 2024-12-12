input=$1 

#Check if there exists 1 argument
if [ $# -ne 1 ]; then
    printf "Please enter 1 number argument, exiting...\n"
    exit 1
fi

#Check if the argument is a number
if ! [[ $input =~ ^[0-9]+$ ]]; then
    printf "Argument must be a number, exiting...\n"
    exit 1
fi

#Loop numbers from 2 to given input-1
for (( find_prime=2; find_prime<input; find_prime++ )); do 
    prime_status=1
    # Check if find_prime is divisible by any number from 2 to find_prime-1
    for (( i=2; i<find_prime; i++ )); do 
        if (( find_prime % i == 0 )); then 
         #not prime
         prime_status=0
         break
        fi 
    done 

    # If prime_status is still 1, find_prime is prime
    if (( prime_status == 1 )); then 
        #Print the hexadecimal representation of the prime number (X is big, because it makes hex letter big letter)
        printf "Hexadecimal of %d is %X\n" "$find_prime" "$find_prime"
    fi 
done
