#!/bin/bash

# takes 2 arguments:
# arg1 = input_file_name
# arg2 = output_file_name
while IFS=, read -r field1 field2; do
    # echo "$field1 and $field2"
    printf "INSERT INTO TABLE1 VALUES (${field1}, \"${field2}\");\n" >>$2
done <$1
