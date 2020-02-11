#!/usr/bin/env python3

# -------------------------------------------------------------------------
# Copyright (C) 2020, Esperanto Technologies Inc.
# The copyright to the computer program(s) herein is the
# property of Esperanto Technologies, Inc. All Rights Reserved.
# The program(s) may be used and/or copied only with
# the written permission of Esperanto Technologies and
# in accordance with the terms and conditions stipulated in the
# agreement/contract under which the program(s) have been supplied.
# -------------------------------------------------------------------------

import re
import sys


def GetEmuCpp():
    '''Get emu.cpp section from coverage file'''

    with open("sysemu.info", "r") as f:
        lines = f.readlines()

    emucpp = []
    enable = False

    for line in lines:

        line = line.rstrip()
        
        if re.match(r'^SF:.*/emu.cpp', line):
            enable = True

        if enable:
            emucpp.append(line)
        
        if re.match(r'^end_of_record', line):
            enable = False

    return emucpp


def GetLineCoverage(emucpp):
    '''Get covered and uncovered lines'''

    tmp = [line for line in emucpp if re.match(r'^DA:\d+,0$', line)]
    not_covered = [re.sub(r'^DA:(\d+),0', r'\1', line) for line in tmp]
    #tmp = filter(lambda x: re.match(r'^DA:\d+,0$', x), emucpp)
    #not_covered = map(lambda x: re.sub(r'^DA:(\d+),0', r'\1', x), tmp)
    nc = {int(line):1 for line in not_covered}

    # Get covered lines

    tmp = [line for line in emucpp if re.match(r'^DA:\d+,[1-9]\d*$', line)]
    covered = [re.sub(r'^DA:(\d+),[1-9]\d*$', r'\1', line) for line in tmp]
    #tmp = filter(lambda x: re.match(r'^DA:\d+,[1-9]\d*$', x), emucpp)
    #covered = map(lambda x: re.sub(r'^DA:(\d+),[1-9]\d*$', r'\1', x), tmp)
    cov = {int(line):1 for line in covered}

    return (cov, nc)


def GetCSRCoverage():
    '''Get CSR read and write coverage'''

    emucpp = GetEmuCpp()
    (cov, nc) = GetLineCoverage(emucpp)
    
    with open("./emu.cpp", "r") as f:
        lines = f.readlines()

    (set_enable, get_enable) = (False, False)
    case_val = []
    case_enable = False
    csr_status = {}
    
    (cov_read_count, cov_write_count) = (0, 0)
    (nc_read_count, nc_write_count) = (0, 0)

    for idx, line in enumerate(lines):

        line = line.rstrip()
        
        if case_enable:
        
            # Track CSR status for every case label
            
            for v in case_val:
                if v not in csr_status:
                    csr_status[v] = {'nc_read': False, 'nc_write': False, 'cov_read': False, 'cov_write': False}
                    
                if idx in nc:
                    if set_enable:
                        csr_status[v]['nc_write'] += True
                        nc_write_count += 1
                    elif get_enable:
                        csr_status[v]['nc_read'] = True
                        nc_read_count += 1
                    case_val = []
                    case_enable = False
                elif idx in cov:
                    if set_enable:
                        csr_status[v]['cov_write'] = True
                        cov_write_count += 1
                    elif get_enable:
                        csr_status[v]['cov_read'] = True
                        cov_read_count += 1
                    case_val = []
                    case_enable = False
                    
            if re.match(r'^\s*break', line):
                case_val = []
                case_enable = False
                    
        # Detect end of get/set function
        
        if re.match(r'^}', line):
            set_enable = False
            get_enable = False

        # Detect start of csrset function
        
        if re.match(r'^.*csrset\(', line):
            set_enable = True
            get_enable = False
            
        if set_enable:
            # Store all case labels for the block
            m = re.match(r'^\s*case\s+(\D\S+)\s*:', line)
            if m:
                case_enable = True
                case_val.append(m.group(1))

        # Detect start of csrget function
        
        if re.match(r'^.*csrget\(', line):
            set_enable = False
            get_enable = True

        if get_enable:
            # Store all case labels for the block
            m = re.match(r'^\s*case\s+(\D\S+)\s*:', line)
            if m:
                case_enable = True
                case_val.append(m.group(1))

    return (csr_status, cov_read_count, cov_write_count, nc_read_count, nc_write_count)


def UpdateCoverageReport(csr_status, cov_read_count, cov_write_count, nc_read_count, nc_write_count):
    print("Sys_emu CSR high level coverage")

    print("Summary")
    total_read_csrs = nc_read_count + cov_read_count
    total_write_csrs = nc_write_count + cov_write_count
    cov_read_percent = (cov_read_count / (cov_read_count + cov_write_count))*100
    nc_read_percent = (nc_read_count / (nc_read_count + nc_write_count))*100
    cov_write_percent = (cov_write_count / (cov_write_count + cov_write_count))*100
    nc_write_percent = (nc_write_count / (nc_write_count + nc_write_count))*100

    with open("sysemu-coverage-insns-regs/bemu/index.html", "r") as f:
        lines = f.readlines()

    with open("sysemu-coverage-insns-regs/bemu/index.html", "w") as f:
        for line in lines:
            m = re.match(r'^</body>', line)
            if m:
                f.write('<h2>Sys_emu CSR Coverage Summary (see emu.cpp above for details)</h2>'
                      '<table width="100%" border=0 cellspacing=0 cellpadding=0>'
                      '<tr><td class="tableHead" colspan=3>Summary</td></tr>'
                      f'<tr><td>Total CSRs in csrget()</td><td> {total_read_csrs}</td></tr>'
                      f'<tr><td>Total CSRs in csrset()</td><td> {total_write_csrs}</td></tr>'
                      f'<tr><td>UnCovered CSR reads   </td><td> {nc_read_count}  </td><td>{nc_read_percent:.2f}%</td></tr>'
                      f'<tr><td>UnCovered CSR writes  </td><td> {nc_write_count} </td><td>{nc_write_percent:.2f}%</td></tr>'
                      f'<tr><td>Covered CSR reads     </td><td> {cov_read_count} </td><td>{cov_read_percent:.2f}%</td></tr>'
                      f'<tr><td>Covered CSR writes    </td><td> {cov_write_count}</td><td>{cov_write_percent:.2f}%</td></tr>'
                      '</table>'
                        )

                f.write('<table border=0 cellspacing=0 cellpadding=0>'
                        '<tr>'
                        '<td class="tableHead">CSR</td>'
                        '<td class="tableHead">Read</td>'
                        '<td class="tableHead">Written</td>'
                        '</tr>')

                for r in sorted(csr_status, key = lambda x: (csr_status[x]["cov_read"], csr_status[x]["cov_write"], x)):
                    print(r)
                    tmp = f'<tr><td>{r}</td><td>{csr_status[r]["cov_read"]}</td><td>{csr_status[r]["cov_write"]}</td></tr>'
                    tmp = re.sub(r'<td>True</td>', '<td bgcolor="#00FF00">True</td>', tmp)
                    tmp = re.sub(r'<td>False</td>', '<td bgcolor="#FF0000"><font color="white">False</font></td>', tmp)
                    f.write(tmp)
                    
                f.write('</table>')
                
            f.write(line)


(csr_status, cov_read_count, cov_write_count, nc_read_count, nc_write_count) = GetCSRCoverage()
UpdateCoverageReport(csr_status, cov_read_count, cov_write_count, nc_read_count, nc_write_count)
