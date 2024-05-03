#!/usr/bin/env python3

import subprocess

subprocess.run(['mcrl22lps', '-v', 'tree.mcrl2', 'tree.lps'], check=True)

subprocess.run(['lps2pbes', '-v', '-f', 'nodeadlock.mcf', 'tree.lps', 'tree.nodeadlock.pbes'], check=True)
subprocess.run(['pbes2bool', '-v', 'tree.nodeadlock.pbes'], check=True)
# The above shows that there is a deadlock in the specification.
# Let's investigate

subprocess.run(['lps2lts', '-v', 'tree.lps', 'tree.aut'], check=True)
# The following creates state space, and stores traces to 512 deadlocks.
subprocess.run(['lps2lts', '-v', '-Dt', 'tree.lps', 'tree.aut'], check=True)
# Print the trace and find out what's wrong:
subprocess.run(['tracepp', 'tree.lps_dlk_0.trc'], check=True)

# The state space in ltsview resembles modern art...
# subprocess.run(['ltsview', 'tree.aut'], check=True)
