from utils import *
import argparse
import os
import sys

TOOL_NAME = 'ct-fuzz'

def arguments():
    parser = argparse.ArgumentParser()

    parser.add_argument('input_file', metavar='FILE', default=None, type=str, help='input file to fuzz')
    parser.add_argument('-d', '--debug', action="store_true", default=False, help='enable debugging output')
    parser.add_argument('--entry-point', metavar='PROC', default=None, type=str, help='entry point function to start with')
    parser.add_argument('--opt-level', metavar='NUM', default=2, type=int, help='compiler optimization level')

    args = parser.parse_args()
    return args

def ct_fuzz_root():
    return os.path.dirname(os.path.dirname(os.path.abspath(sys.argv[0])))

def ct_fuzz_include_dir():
    return os.path.join(ct_fuzz_root(), 'share', TOOL_NAME, 'include')

def xxHash_include_dir():
    return os.path.join(ct_fuzz_root(), 'xxHash')

def ct_fuzz_lib_dir():
    return os.path.join(ct_fuzz_root(), 'share', TOOL_NAME, 'lib')

#def ct_fuzz_src_dynamic_lib():
#    return os.path.join(ct_fuzz_root(), 'build', 'libCTFuzzInstrumentSrc.so')

def ct_fuzz_self_dynamic_lib():
    return os.path.join(ct_fuzz_root(), 'build', 'libCTFuzzInstrumentSelf.so')

def afl_clang_fast_path():
    return os.path.join(ct_fuzz_root(), 'afl-2.52b', 'afl-clang-fast')
#def afl_pass_dynamic_lib():
#    return os.path.join(ct_fuzz_root(), 'afl-2.52b', 'afl-llvm-pass.so')
#
#def afl_rt_obj():
#    return os.path.join(ct_fuzz_root(), 'afl-2.52b', 'afl-llvm-rt-64.o')

def xxHash_dir():
    return os.path.join(ct_fuzz_root(), 'build')

def compile_c_file(args, c_file_name, plugins=''):
    cmd = [afl_clang_fast_path(), '-O'+str(args.opt_level), '-c']
    cmd += plugins.split()
    cmd += [c_file_name]
    cmd += ['-emit-llvm']
    cmd += ['-I'+ct_fuzz_include_dir()]
    cmd += ['-I'+xxHash_include_dir()]
    try_command(cmd);

def build_libs(args):
    lib_file_paths = map(lambda l: os.path.join(ct_fuzz_lib_dir(), l), filter(
        lambda f: os.path.splitext(f)[1] == '.c', os.listdir(ct_fuzz_lib_dir())))
    map(lambda l: compile_c_file(args, l), lib_file_paths)
    return map(lambda l: replace_suffix(os.path.basename(l)), lib_file_paths)

def link_bc_files(args, libs):
    cmd = ['llvm-link', args.input_file_name+'.bc'] + libs
    args.opt_in_file = args.input_file_name+'_pre_inst.bc'
    cmd += ['-o', args.opt_in_file]
    try_command(cmd);

def run_pass(args):
    args.opt_out_file = args.input_file_name+'_post_inst.bc'
    cmd = ['opt', '-load']
    cmd += [ct_fuzz_self_dynamic_lib()]
    cmd += ['-'+TOOL_NAME+'-instrument-self']
    cmd += ['-entry-point', args.entry_point]
    cmd += [args.opt_in_file, '-o', args.opt_out_file]
    try_command(cmd)

def compile_bc_to_exec(args):
    llc_cmd = ['llc', '-filetype=obj', args.opt_out_file]
    try_command(llc_cmd)
    clang_cmd = [afl_clang_fast_path(), args.input_file_name+'_post_inst.o']
    clang_cmd += ['-L`jemalloc-config --libdir`', '-Wl,-rpath,`jemalloc-config --libdir`', '-ljemalloc', '`jemalloc-config --libs`']
    clang_cmd = ' '.join(clang_cmd)
    clang_cmd += ' -lxxHash -L{0} -Wl,-rpath,{0}'.format(xxHash_dir())
    try_command(clang_cmd, shell=True)

def make_test_binary(args):
    compile_c_file(args, args.input_file)
    libs = build_libs(args)
    link_bc_files(args, libs)
    run_pass(args)
    compile_bc_to_exec(args)

def get_file_name(f):
    return os.path.splitext(os.path.basename(f))[0]

def replace_suffix(f):
    return os.path.splitext(f)[0]+'.bc'

def main():
    try:
        global args
        args = arguments()
        args.input_file_name = get_file_name(args.input_file)
        make_test_binary(args)
    except KeyboardInterrupt:
        sys.exit("SMACK aborted by keyboard interrupt.")
