"""
gatchorhash module is not publicly available, so this is a placeholder for demonstration purposes.
This script allows users to input a string and get its Gatchor-1024 hash, as well as verify if a given string matches a provided hash.

"""
from gatchorhash import gatchor1024
def string_hash():
    input_str = input("Enter a string to hash: ")
    hash_result = gatchor1024(input_str)
    print(f"Gatchor-1024 hash: {hash_result}")

def hash_strings():
    hash_to_verify = input("Enter a hash to verify: ")
    original_string = input("Enter the original string: ")
    # if the hash of the original string matches the provided hash, it's valid
    if gatchor1024(original_string) == hash_to_verify:
        print("Valid hash!")
    else:        print("Invalid hash!")
     

if __name__ == "__main__":
    string_hash()
    print("\nRunning predefined test cases:")
    hash_strings()
