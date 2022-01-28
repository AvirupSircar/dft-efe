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

# @author Ian C. Lin

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
class RunOnlyTest_TestVectorNorms(rfm.RunOnlyRegressionTest):
    target_name = 'TestVectorNorms'
    descr = 'A build and run test using CMake'
    build_system = 'CMake'
    make_opts = [target_name]
    executable = os.path.dirname(os.path.abspath(__file__))+"/executable/"+target_name+".x"
    tagsDict = {'compileOrRun': 'run', 'unitOrAggregate':
        'unit','slowOrFast': 'fast', 'arch': 'cpu',
                'serialOrParallel': 'serial'}
    tags = {x.lower() for x in tagsDict.values()}

    valid_systems = ['*']
    valid_prog_environs = ['*']

    config_opts = cmflags.getConfig()

    @run_before('run')
    def set_launcher_and_resources(self):
        if "serial" in self.tags:
            self.job.launcher = getlauncher('local')()

        if "parallel" in self.tags:
            self.job.launcher.options = ['']
        self.extra_resources = ss.setResources(self.tagsDict['arch'])

    @sanity_function
    def validate_test(self):
        hasTestPassed = True
        msg = 'Failed for some reason'
        filename = self.target_name
        bmParser = parser.Parser.fromFilename(filename+'.benchmark')
        outParser = parser.Parser.fromFilename(filename+'.out')
        testSet = ["double vec norms", "complex double vec norms"]
        for testString in testSet:
            bmVal = bmParser.extractKeyValues(testString)
            outVal = outParser.extractKeyValues(testString)
            hasTestPassed, norm, msg = cu.Compare().cmp(bmVal, outVal, 1.0e-16, 'absolute', 'point')
            if not hasTestPassed:
                print(filename)
                print(testString)
                print('benchmark')
                print(bmVal)
                print('output')
                print(outVal)
                msg = "Failed in {}".format(testString)
                return sn.assert_true(hasTestPassed, msg=msg)

        return sn.assert_true(hasTestPassed, msg=msg)



@rfm.simple_test
class BuildAndRunTest_TestVectorNorms(rfm.RegressionTest):
    target_name = 'TestVectorNorms'
    descr = 'A build and run test using CMake'
    build_system = 'CMake'
    make_opts = [target_name]

    # NOTE: Need to specify the name of the executable, as
    # ReFrame has no way of knowing that while building from CMake
    executable = "./"+target_name+".x"
    tagsDict = {'compileOrRun': 'compile', 'unitOrAggregate':
        'unit','slowOrFast': 'fast', 'arch': 'cpu',
                'serialOrParallel': 'serial'}
    tags = {x.lower() for x in tagsDict.values()}

    valid_systems = ['*']
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
            self.job.launcher.options = ['']
        self.extra_resources = ss.setResources(self.tagsDict['arch'])

    @sanity_function
    def validate_test(self):
        hasTestPassed = True
        msg = 'Failed for some reason'
        filename = self.target_name
        bmParser = parser.Parser.fromFilename(filename+'.benchmark')
        outParser = parser.Parser.fromFilename(filename+'.out')
        testSet = ["double vec norms", "complex double vec norms"]
        for testString in testSet:
            bmVal = bmParser.extractKeyValues(testString)
            outVal = outParser.extractKeyValues(testString)
            hasTestPassed, norm, msg = cu.Compare().cmp(bmVal, outVal, 1.0e-16, 'absolute', 'point')
            if not hasTestPassed:
                print(filename)
                print(testString)
                print('benchmark')
                print(bmVal)
                print('output')
                print(outVal)
                msg = "Failed in {}".format(testString)
                return sn.assert_true(hasTestPassed, msg=msg)

        return sn.assert_true(hasTestPassed, msg=msg)