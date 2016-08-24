#!/usr/bin/python2.7
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import itertools
import os
import re
import subprocess
import sys

class Error(Exception):
    """Module error class."""


class UnknownArchitectureError(Error):
    """Raised if an architecture can not be handled."""


def GetCrossTool(tool):
    chost = os.getenv('CHOST', '')
    if chost:
        return chost + '-' + tool
    return tool


def SymbolMap(object_filename, sort_numerically=True):
    """Run nm tool to list symbols from object file."""
    cmd = [GetCrossTool('nm')]
    if sort_numerically:
        cmd.append('-n')
    cmd.append(object_filename),
    out = subprocess.check_output(cmd)
    for line in out.splitlines():
        cols = line.split()
        if len(cols) == 2:
            addr = None
            symbol_type, symbol_name = cols
        elif len(cols) == 3:
            addr, symbol_type, symbol_name = cols
        else:
            raise Error('Unexpected number of columns')
        yield addr, symbol_type, symbol_name


def Disassemble(object_filename, start_address, stop_address):
    """Disassemble a portion of an object file using objdump."""
    return subprocess.check_output((
            GetCrossTool('objdump'), '-d', '--no-show-raw-insn',
            '--start-address', '0x'+start_address,
            '--stop-address', '0x'+stop_address,
            object_filename))


ASSEMBLY_RE = re.compile(
        r'^ +(?P<address>[0-9A-Fa-f]+):\t(?P<mnemonic>\S+)\s+(?P<operands>.*)$')
X86_CONDITIONAL_BRANCH_INSTRUCTIONS = set([
        'jo',           # opcode: 0x70
        'jno',                  # 0x71
        'jb', 'jnae', 'jc',     # 0x72
        'jnb', 'jae', 'jnc',    # 0x73
        'jz', 'je'              # 0x74
        'jnz', 'jne',           # 0x75
        'jbe', 'jna',           # 0x76
        'jnbe', 'ja',           # 0x77
        'js',                   # 0x78
        'jns',                  # 0x79
        'jp', 'jpe',            # 0x7a
        'jnp', 'jpo',           # 0x7b
        'jl', 'jnge',           # 0x7c
        'jnl', 'jge',           # 0x7d
        'jle', 'jng',           # 0x7e
        'jnle', 'jg',           # 0x7f
        'loopnz', 'loopne',     # 0xe0
        'loopz', 'loope',       # 0xe1
        'loop',                 # 0xe2
        'jcxz', 'jecxz', 'jrcxz',])  # 0xe3


def IsBranch_x86(mnemonic):
    return mnemonic in X86_CONDITIONAL_BRANCH_INSTRUCTIONS


ARM_BRANCH_INSTRUCTIONS = [
        'b', 'bl', 'blx', 'bx', 'bxj', 'cbz', 'cbnz']
ARM_CONDITIONS = [
        'eq', 'ne', 'cs', 'hs', 'cc', 'lo', 'mi', 'pl', 'vs', 'vc',
        'hi', 'ls', 'ge', 'lt', 'gt', 'le', 'al']
ARM_ALL_BRANCH_INSTRUCTIONS = set(
    instr + cond
    for instr, cond
    in itertools.product(ARM_BRANCH_INSTRUCTIONS, ARM_CONDITIONS))


def IsBranch_arm(mnemonic):
    if '.' in mnemonic:
        mnemonic, width = mnemonic.split('.', 1)
    return mnemonic in ARM_ALL_BRANCH_INSTRUCTIONS


X86_ARCH_CHOST_RE = re.compile(r'^(x86|i[2346]86)')


def ChooseIsBranchForChost(chost):
    if X86_ARCH_CHOST_RE.match(chost):
        return IsBranch_x86
    if chost.startswith('arm'):
        return IsBranch_arm
    raise UnknownArchitectureError(chost)


def _FindLoopBranches(disassembly, is_branch):
    for line in disassembly.splitlines():
        m = ASSEMBLY_RE.match(line)
        if not m:
             continue
        address, mnemonic, operands = m.group('address', 'mnemonic', 'operands')
        if is_branch(mnemonic):
            target_address, target_label = operands.split()
            yield address, target_address


def FindLoopBranches(disassembly):
    chost = os.getenv('CHOST', '')
    is_branch = ChooseIsBranchForChost(chost)
    return _FindLoopBranches(disassembly, is_branch)


def main():
    object_filename = sys.argv[1]
    for addr, symbol_type, symbol_name in SymbolMap(object_filename):
        if symbol_name == 'the_loop_start':
            loop_start = addr
        if symbol_name == 'the_loop_end':
            loop_end = addr
    disassembly = Disassemble(object_filename, loop_start, loop_end)
    try:
        for source, target in FindLoopBranches(disassembly):
            print source, target
    except UnknownArchitectureError as e:
        print >> sys.stderr, 'Unknown architecture for chost ' + e.args[0]



if __name__ == '__main__':
    main()
