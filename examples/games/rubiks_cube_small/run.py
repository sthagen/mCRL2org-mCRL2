#!/usr/bin/env python3

import subprocess
import os

# Change working dir to the script path
os.chdir(os.path.dirname(os.path.abspath(__file__)))

subprocess.run(['mcrl22lps', '-f', 'small_cube.mcrl2', 'temp.lps'], check=True)

run = subprocess.run(['lpsparunfold', '-l', 'temp.lps', '-v', '-n4', '-s"List(List(Color], check=True)], check=True)"'], stdout=subprocess.PIPE, check=True)
run = subprocess.run(['lpsconstelm', '-v'], input=run.stdout, stdout=subprocess.PIPE, check=True)
run = subprocess.run(['lpsrewr', '-v'], input=run.stdout, stdout=subprocess.PIPE, check=True)
run = subprocess.run(['lpsparunfold', '-l', 'temp1.lps', '-v', '-n8', '-s"List(Color], check=True)"'], input=run.stdout, stdout=subprocess.PIPE, check=True)
run = subprocess.run(['lpsconstelm', '-v'], input=run.stdout, stdout=subprocess.PIPE, check=True)
subprocess.run(['lpsrewr', '-v', '-', 'temp.lps'], input=run.stdout, check=True)

subprocess.run(['lps2lts', '-asolved', '-t1', '-v', '-rjitty', 'temp.lps'], check=True)
subprocess.run(['tracepp', 'temp2.lps_act_0_solved.trc'], check=True)
