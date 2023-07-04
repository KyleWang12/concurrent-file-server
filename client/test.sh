#!/bin/bash

# Set the base directory name
base_dir="base_test_dir"

# Create the log file
log_file="test_logs.txt"
echo "Client logs:" > $log_file

# Create the base directory
./fget MD $base_dir >> $log_file 2>&1

# Function for single client tests for MD (make directory)
single_client_md_tests() {
    for i in {1..5}; do
        nested_dir="$base_dir/single_client_test_$i"
        ./fget MD $nested_dir >> $log_file 2>&1
        echo "Created directory: $nested_dir" >> $log_file
    done
}

# Function for single client tests for PUT (upload file)
single_client_put_tests() {
    for i in {1..5}; do
        file_name="$base_dir/single_client_test_file_$i.txt"
        content=$(head /dev/urandom | tr -dc A-Za-z0-9 | head -c 32)
        echo $content > temp.txt
        ./fget PUT temp.txt $file_name >> $log_file 2>&1
        echo "PUT file: $file_name with content: $content" >> $log_file
    done
    rm temp.txt
}

# Function for single client tests for INFO (file information)
single_client_info_tests() {
    for i in {1..5}; do
        file_name="$base_dir/single_client_test_file_$i.txt"
        ./fget INFO $file_name >> $log_file 2>&1
        echo "INFO for file: $file_name" >> $log_file >> $log_file
    done
}

# Function for single client tests for GET (download file)
single_client_get_tests() {
    for i in {1..5}; do
        file_name="$base_dir/single_client_test_file_$i.txt"
        local_file="local_copy_$i.txt"
        ./fget GET $file_name $local_file >> $log_file 2>&1
        echo "GET file: $file_name to local file: $local_file" >> $log_file
    done
}

# Define the number of clients to run concurrently
num_clients=5

# Function to create nested directories, put files, and test INFO for each client
run_concurrent_tests() {
    client_id=$1
    client_base="$base_dir/client_$client_id"

    ./fget MD $client_base >> $log_file 2>&1

    # Create nested directories
    for i in {1..5}; do
        nested_dir="$client_base/nested_$i"
        ./fget MD $nested_dir >> $log_file 2>&1 &
        echo "Client $client_id created directory: $nested_dir" >> $log_file
    done
    wait

    # PUT command
    for i in {1..5}; do
        file_name="$client_base/client_file_$i.txt"
        content=$(head /dev/urandom | tr -dc A-Za-z0-9 | head -c 32)
        echo $content > temp_$client_id.txt
        ./fget PUT temp_$client_id.txt $file_name >> $log_file 2>&1 &
        echo "Client $client_id PUT file: $file_name with content: $content" >> $log_file
    done
    wait

    # INFO command
    for i in {1..5}; do
        file_name="$client_base/client_file_$i.txt"
        ./fget INFO $file_name >> $log_file 2>&1 &
        echo "Client $client_id INFO for file: $file_name" >> $log_file
    done
    wait

    # GET command
    for i in {1..5}; do
        file_name="$client_base/client_file_$i.txt"
        local_file="local_copy_$client_id.txt"
        ./fget GET $file_name $local_file >> $log_file 2>&1 &
        echo "Client $client_id GET file: $file_name to local file: $local_file" >> $log_file
    done
    wait

    # RM command
    for i in {1..5}; do
        file_name="$client_base/client_file_$i.txt"
        ./fget RM $file_name >> $log_file 2>&1 &
        echo "Client $client_id RM file: $file_name" >> $log_file
    done
    wait

    # Clean up
    rm temp_$client_id.txt
    rm local_copy_$client_id.txt
}

# Run single client tests
single_client_md_tests
single_client_put_tests
single_client_info_tests
single_client_get_tests

# Run concurrent tests
echo "Running concurrent tests..."  >> $log_file
for i in $(seq 1 $num_clients); do
    run_concurrent_tests $i &
done

# Wait for all clients to finish
wait

# Clean up
./fget RM $base_dir >> $log_file 2>&1

echo "Concurrent tests completed."  >> $log_file