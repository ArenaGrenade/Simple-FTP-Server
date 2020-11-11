# Run as ./generator.sh file_name size(in MB)
base64 -w 0 /dev/urandom | head -c $(($2 * 1000000)) > ./server/$1