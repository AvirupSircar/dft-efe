#  Copyright (c) 2021-2022.
#  The Regents of the University of Michigan and DFT-EFE developers.
#
#  This file is part of the DFT-EFE code.
#
#  DFT-EFE is free software: you can redistribute it and/or modify
#    it under the terms of the Lesser GNU General Public License as
#    published by the Free Software Foundation, either version 3 of
#    the License, or (at your option) any later version.
#
#  DFT-EFE is distributed in the hope that it will be useful, but
#    WITHOUT ANY WARRANTY; without even the implied warranty
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#    See the Lesser GNU General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#    License at the top level of DFT-EFE distribution.  If not, see
#    <https://www.gnu.org/licenses/>.

# @author Sambit Das 

import reframe as rfm
import reframe.utility.sanity as sn
from reframe.utility.sanity import evaluate
from reframe.core.backends import getlauncher
import os
DFTEFE_PATH=''
if not 'DFTEFE_PATH' in os.environ:
    raise Exception('''DFTEFE_PATH is not set. Please use 'export
                 DFTEFE_PATH=/path/to/dft-efe/parent/folder''')
else:
    DFTEFE_PATH = os.environ['DFTEFE_PATH']
parser = rfm.utility.import_module_from_file(DFTEFE_PATH+"/test/Parser.py")
cu = rfm.utility.import_module_from_file(DFTEFE_PATH+"/test/CompareUtil.py")
ss = rfm.utility.import_module_from_file(DFTEFE_PATH+"/test/SetupSystems.py")
cmflags = rfm.utility.import_module_from_file(DFTEFE_PATH+"/CMakeFlagsParser.py")

@rfm.simple_test
class BuildOnlyTestMPICommunicatorP2PUpdateGhosts(rfm.CompileOnlyRegressionTest):
    descr = 'Compile only test for TestMPICommunicatorP2PUpdateGhosts'
    build_system = 'CMake'
    make_opts = ['TestMPICommunicatorP2PUpdateGhosts']
    sourcesdir = './src'
    tagsDict = {'compileOrRun': 'compile', 'unitOrAggregate':
                'unit', 'slowOrFast': 'fast', 'arch': 'cpu',
                'serialOrParallel': 'parallel'}
    tags = {x.lower() for x in tagsDict.values()}
    valid_systems = ss.getValidSystems(tagsDict['arch']) 
    valid_prog_environs = ['*']
    keep_files = []
    config_opts = cmflags.getConfig()

    @run_before('compile')
    def set_compiler_flags(self):
        self.build_system.make_opts = self.make_opts
        self.build_system.config_opts = self.config_opts

    @sanity_function
    def validate_test(self):
        hasWarning = True
        hasError = True
        msgWarning = "Found warning(s) while compiling."
        msgError = "Found error(s) while compiling."
        matches = evaluate(sn.findall(r'(?i)warning', evaluate(self.stdout)))
        if len(matches) == 0:
            hasWarning = False

        matchesOut = evaluate(sn.findall(r'(?i)error', evaluate(self.stdout)))
        matchesErr = evaluate(sn.findall(r'(?i)error', evaluate(self.stderr)))
        if len(matchesOut) == 0 and len(matchesErr) == 0:
            hasError = False
        
        hasTestPassed = not hasWarning and not hasError
        msg = ""
        if hasError:
            msg = msgError

        elif hasWarning:
            msg = msgWarning

        else:
            msg = ""
            return sn.assert_true(hasTestPassed, msg=msg)




@rfm.simple_test
class BuildAndRunTestMPICommunicatorP2PUpdateGhosts1Node4Procs(rfm.RegressionTest):
    target_name = 'TestMPICommunicatorP2PUpdateGhosts'
    descr = '''A run only test to verify the ghost data in MPICommunicatorP2P on 1
    node with 4 processors'''
    build_system = 'CMake'
    make_opts = [target_name]
    # NOTE: Need to specify the name of the executable, as
    # ReFrame has no way of knowing that while building from CMake
    executable = "./"+target_name+".x"
    tagsDict = {'compileOrRun': 'compile', 'unitOrAggregate':
        'unit','slowOrFast': 'fast', 'arch': 'cpu',
                'serialOrParallel': 'parallel'}
    tags = {x.lower() for x in tagsDict.values()}
    valid_systems = ss.getValidSystems(tagsDict['arch']) 
    valid_prog_environs = ['*']
    config_opts = cmflags.getConfig()


    @run_before('compile')
    def set_compiler_flags(self):
        self.build_system.make_opts = self.make_opts
        self.build_system.config_opts = self.config_opts


    @run_before('run')
    def set_launcher_and_resources(self):
        if "serial" in self.tags:
            self.job.launcher = getlauncher('local')()

        if "parallel" in self.tags:
            self.job.launcher.options = ['-n 4']
            self.extra_resources = ss.setResources(self.tagsDict['arch'], 
                                                   time_limit = "00:05:00", 
                                                   num_nodes = 1, 
                                                   num_tasks_per_node = 4,
                                                   ntasks = 4,
                                                   mem_per_cpu = '2gb')


    @sanity_function
    def validate_test(self):
        # This test does not generate any output. It throws an exception
        # if the logic of MPICommunicatorP2P fails to find the ghost data
        hasAssertFail = True
        hasThrownException = True
        hasError = True
        msgError = '''Found error(s) in
        BuildAndRunTestMPICommunicatorP2PUpdateGhosts1Node4Procs.'''
        msgThrownException = '''Found exceptions in 
        BuildAndRunTestMPICommunicatorP2PUpdateGhosts1Node4Procs.'''
        msgAssertFail = '''Found assert fail(s) in
        BuildAndRunTestMPICommunicatorP2PUpdateGhosts1Node4Procs.'''
        matchesOut = evaluate(sn.findall(r'(?i)error', evaluate(self.stdout)))
        matchesErr = evaluate(sn.findall(r'(?i)error', evaluate(self.stderr)))
        if len(matchesOut) == 0 and len(matchesErr) == 0:
            hasError = False

        matchesOut = evaluate(sn.findall(r'(?i)assert', evaluate(self.stdout)))
        matchesErr = evaluate(sn.findall(r'(?i)assert', evaluate(self.stderr)))
        if len(matchesOut) == 0 and len(matchesErr) == 0:
            hasAssertFail = False
        
        matchesOut = evaluate(sn.findall(r'(?i)throw', evaluate(self.stdout)))
        matchesErr = evaluate(sn.findall(r'(?i)throw', evaluate(self.stderr)))
        if len(matchesOut) == 0 and len(matchesErr) == 0:
            hasThrownException = False
        
        hasTestPassed = not any([hasError, hasAssertFail, hasThrownException]) 
        
        msg = ""
        if hasError:
            msg = msgError

        elif hasAssertFail:
            msg = msgAssertFail

        elif hasThrownException:
            msg = msgThrownException

        else:
            msg=""

        return sn.assert_true(hasTestPassed, msg=msg)


@rfm.simple_test
class RunOnlyTestMPICommunicatorP2PUpdateGhosts1Node4Procs(rfm.RunOnlyRegressionTest):
    target_name = 'TestMPICommunicatorP2PUpdateGhosts'
    descr = '''A run only test to verify the ghost data in MPICommunicatorP2P on 1
    node with 4 processors'''
    build_system = 'CMake'
    make_opts = [target_name]
    executable = os.path.dirname(os.path.abspath(__file__))+"/executable/"+target_name+".x"
    tagsDict = {'compileOrRun': 'run', 'unitOrAggregate':
        'unit','slowOrFast': 'fast', 'arch': 'cpu',
                'serialOrParallel': 'parallel'}
    tags = {x.lower() for x in tagsDict.values()}
    valid_systems = ss.getValidSystems(tagsDict['arch']) 
    valid_prog_environs = ['*']
    config_opts = cmflags.getConfig()

    
    @run_before('run')
    def set_launcher_and_resources(self):
        if "serial" in self.tags:
            self.job.launcher = getlauncher('local')()

        if "parallel" in self.tags:
            self.job.launcher.options = ['-n 4']
            self.extra_resources = ss.setResources(self.tagsDict['arch'], 
                                                   time_limit = "00:05:00", 
                                                   num_nodes = 1, 
                                                   num_tasks_per_node = 4,
                                                   ntasks = 4,
                                                   mem_per_cpu = '2gb')


    @sanity_function
    def validate_test(self):
        # This test does not generate any output. It throws an exception
        # if the logic of MPICommunicatorP2P fails to find the ghost data
        hasAssertFail = True
        hasThrownException = True
        hasError = True
        msgError = '''Found error(s) in
        RunOnlyTestMPICommunicatorP2PUpdateGhosts1Node4Procs.'''
        msgThrownException = '''Found exceptions in
        RunOnlyTestMPICommunicatorP2PUpdateGhosts1Node4Procs'''
        msgAssertFail = '''Found assert fail(s) in
        RunOnlyTestMPICommunicatorP2PUpdateGhosts1Node4Procs'''
        matchesOut = evaluate(sn.findall(r'(?i)error', evaluate(self.stdout)))
        matchesErr = evaluate(sn.findall(r'(?i)error', evaluate(self.stderr)))
        if len(matchesOut) == 0 and len(matchesErr) == 0:
            hasError = False

        matchesOut = evaluate(sn.findall(r'(?i)assert', evaluate(self.stdout)))
        matchesErr = evaluate(sn.findall(r'(?i)assert', evaluate(self.stderr)))
        if len(matchesOut) == 0 and len(matchesErr) == 0:
            hasAssertFail = False
        
        matchesOut = evaluate(sn.findall(r'(?i)throw', evaluate(self.stdout)))
        matchesErr = evaluate(sn.findall(r'(?i)throw', evaluate(self.stderr)))
        if len(matchesOut) == 0 and len(matchesErr) == 0:
            hasThrownException = False
        
        hasTestPassed = not any([hasError, hasAssertFail, hasThrownException]) 
        
        msg = ""
        if hasError:
            msg = msgError

        elif hasAssertFail:
            msg = msgAssertFail

        elif hasThrownException:
            msg = msgThrownException

        else:
            msg=""

        return sn.assert_true(hasTestPassed, msg=msg)
