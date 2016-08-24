#!/usr/bin/env python

import re
import sys
import SELinuxNeverallowTestFrame

usage = "Usage: ./gen_SELinux_CTS_neverallows.py <input policy file> <output cts java source>"

# extract_neverallow_rules - takes an intermediate policy file and pulls out the
# neverallow rules by taking all of the non-commented text between the 'neverallow'
# keyword and a terminating ';'
# returns: a list of strings representing these rules
def extract_neverallow_rules(policy_file):
    with open(policy_file, 'r') as in_file:
        policy_str = in_file.read()
        # remove comments
        no_comments = re.sub(r'#.+?$', r'', policy_str, flags = re.M)
        # match neverallow rules
        return re.findall(r'(^neverallow\s.+?;)', no_comments, flags = re.M |re.S);

# neverallow_rule_to_test - takes a neverallow statement and transforms it into
# the output necessary to form a cts unit test in a java source file.
# returns: a string representing a generic test method based on this rule.
def neverallow_rule_to_test(neverallow_rule, test_num):
    squashed_neverallow = neverallow_rule.replace("\n", " ")
    method  = SELinuxNeverallowTestFrame.src_method
    method = method.replace("testNeverallowRules()",
        "testNeverallowRules" + str(test_num) + "()")
    return method.replace("$NEVERALLOW_RULE_HERE$", squashed_neverallow)

if __name__ == "__main__":
    # check usage
    if len(sys.argv) != 3:
        print usage
        exit()
    input_file = sys.argv[1]
    output_file = sys.argv[2]

    src_header = SELinuxNeverallowTestFrame.src_header
    src_body = SELinuxNeverallowTestFrame.src_body
    src_footer = SELinuxNeverallowTestFrame.src_footer

    # grab the neverallow rules from the policy file and transform into tests
    neverallow_rules = extract_neverallow_rules(input_file)
    i = 0
    for rule in neverallow_rules:
        src_body += neverallow_rule_to_test(rule, i)
        i += 1

    with open(output_file, 'w') as out_file:
        out_file.write(src_header)
        out_file.write(src_body)
        out_file.write(src_footer)
