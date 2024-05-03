#!/usr/bin/env python3

import subprocess

subprocess.run(['mcrl22lps', '-v', 'par.mcrl2', 'par.lps'], check=True)

subprocess.run(['lps2pbes', '-v', '-f', 'nodeadlock.mcf', 'par.lps', 'par.nodeadlock.pbes'], check=True)
subprocess.run(['pbes2bool', '-v', 'par.nodeadlock.pbes'], check=True)

# The following creates state space, and stores trace to deadlock.
subprocess.run(['lps2lts', '-v', '-Dt', 'par.lps', 'par.aut'], check=True)
