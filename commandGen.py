import argparse
import subprocess
import os

def copy2clip(string):
	cmd = 'echo | set /p=' + string.strip() + '|clip'
	return subprocess.check_call(cmd, shell=True)

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
copy2clip(cmd)

with open(output, 'r') as file:
	string = file.readline()
	print(string)