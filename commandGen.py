import argparse
import subprocess
import os

testsFolder = "tests2"
# testsFolder = "tests"

def copy2clip(string):
	cmd = 'echo | set /p=' + string.strip() + '|clip'
	return subprocess.check_call(cmd, shell=True)

parser = argparse.ArgumentParser()
parser.add_argument('prog', type=str)
parser.add_argument('number', type=str)
args = parser.parse_args()

num = args.number
command = testsFolder + "/example" + str(num) + "_command"
trace = testsFolder + "/example" + str(num) + "_trace"
output = testsFolder + "/example" + str(num) + "_output"

with open(command, 'r') as file:
	string = file.readline()
	if testsFolder == "tests2":
		toReplace = "./cacheSim example" + str(num) +  "_trace "
		string = string.replace(toReplace, "")

if args.prog == "py":
	program = "hw2sketch.py "
elif args.prog == 'u':
	program = "./cacheSim "
else:
	program = "hw2cpp\Debug\hw2cpp.exe "
	
cmd = program + trace + " " + string
if args.prog == 'u':
	print(cmd)
else:
	copy2clip(cmd)

with open(output, 'r') as file:
	string = file.readline()
	print(string)