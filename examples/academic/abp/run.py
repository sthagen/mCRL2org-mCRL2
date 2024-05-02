#!/usr/bin/env python3

import subprocess

subprocess.run(['mcrl22lps', '-v', 'abp.mcrl2', 'abp.lps'], shell=True, check=True)
subprocess.run(['lps2lts', '-v', 'abp.lps', 'abp.aut'], shell=True, check=True)
subprocess.run(['lps2pbes', '-v', '-f', 'nodeadlock.mcf', 'abp.lps', 'abp.nodeadlock.pbes'], shell=True, check=True)
subprocess.run(['pbes2bool', '-v', 'abp.nodeadlock.pbes'], shell=True, check=True)

subprocess.run(['lps2pbes', '-v', '-f', 'infinitely_often_enabled_then_infinitely_often_taken.mcf', 'abp.lps', 'abp.infinitely_often_enabled_then_infinitely_often_taken.pbes'], shell=True, check=True)
subprocess.run(['pbes2bool', '-v', 'abp.infinitely_often_enabled_then_infinitely_often_taken.pbes'], shell=True, check=True)

subprocess.run(['lps2pbes', '-v', '-f', 'infinitely_often_lost.mcf', 'abp.lps', 'abp.infinitely_often_lost.pbes'], shell=True, check=True)
subprocess.run(['pbes2bool', '-v', 'abp.infinitely_often_lost.pbes'], shell=True, check=True)

subprocess.run(['lps2pbes', '-v', '-f', 'infinitely_often_receive_d1.mcf', 'abp.lps', 'abp.infinitely_often_receive_d1.pbes'], shell=True, check=True)
subprocess.run(['pbes2bool', '-v', 'abp.infinitely_often_receive_d1.pbes'], shell=True, check=True)

subprocess.run(['lps2pbes', '-v', '-f', 'infinitely_often_receive_for_all_d.mcf', 'abp.lps', 'abp.infinitely_often_receive_for_all_d.pbes'], shell=True, check=True)
subprocess.run(['pbes2bool', '-v', 'abp.infinitely_often_receive_for_all_d.pbes'], shell=True, check=True)

subprocess.run(['lps2pbes', '-v', '-f', 'read_then_eventually_send.mcf', 'abp.lps', 'abp.read_then_eventually_send.pbes'], shell=True, check=True)
subprocess.run(['pbes2bool', '-v', 'abp.read_then_eventually_send.pbes'], shell=True, check=True)

subprocess.run(['lps2pbes', '-v', '-f', 'read_then_eventually_send_if_fair.mcf', 'abp.lps', 'abp.read_then_eventually_send_if_fair.pbes'], shell=True, check=True)
subprocess.run(['pbes2bool', '-v', 'abp.read_then_eventually_send_if_fair.pbes'], shell=True, check=True)

subprocess.run(['lps2pbes', '-v', '-f', 'no_generation_of_messages.mcf', 'abp.lps', 'abp.no_generation_of_messages.pbes'], shell=True, check=True)
subprocess.run(['pbes2bool', '-v', 'abp.no_generation_of_messages.pbes'], shell=True, check=True)

subprocess.run(['lps2pbes', '-v', '-f', 'no_duplication_of_messages.mcf', 'abp.lps', 'abp.no_duplication_of_messages.pbes'], shell=True, check=True)
subprocess.run(['pbes2bool', '-v', 'abp.no_duplication_of_messages.pbes'], shell=True, check=True)