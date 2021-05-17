import argparse

parser = argparse.ArgumentParser()
parser.add_argument('number', type=str)

args = parser.parse_args()



num = args.number
command = "tests/example" + str(num) + "_command"
trace = "tests/example" + str(num) + "_trace"
output = "tests/example" + str(num) + "_output"

with open(command, 'r') as file:
	string = file.readline()

cmd = "hw2sketch.py " + trace + " " + string
print(cmd)