# Grepper
A simple subset grep implementation

## Usage
`sh
./grep [OPTIONS] <pattern> <path?>
`
Positional Arguments:
<pattern> 
The search pattern

<path?> 
Optional argument. When empty, this will be defaulting to search the entire file in 
the current working directory recursively. It also could be a file which means only 
search in that spesific file

Available Options:
[--ignore-case, -i] 
Ignore case distinction in both pattern and input file

[--recusive, -r] 
Search across the directory recursively

