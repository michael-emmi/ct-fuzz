from utils import *
import argparse
import os
import sys

TOOL_NAME = 'ct-fuzz'

def arguments():
    parser = argparse.ArgumentParser()

    parser.add_argument('input_file', metavar='FILE', default=None, type=str, help='specification file (can also contain source functions to fuzz)')
    parser.add_argument('--obj-file', metavar='FILE', default=None, type=str, help='input object file')
    parser.add_argument('-d', '--debug', action="store_true", default=False, help='enable debugging output')
    parser.add_argument('--entry-point', metavar='PROC', default=None, type=str, help='entry point function to start with')
    parser.add_argument('--opt-level', metavar='NUM', default=2, type=int, help='compiler optimization level')
    parser.add_argument('--compiler-options', metavar='OPTIONS', default='', type=str, help='compiler options')
    parser.add_argument('-o', '--output-file', metavar='FILE', default=None, type=str, help='output binary')

    args = parser.parse_args()
    return args

def ct_fuzz_root():
    return os.path.dirname(os.path.dirname(os.path.abspath(sys.argv[0])))

def ct_fuzz_include_dir():
    return os.path.join(ct_fuzz_root(), 'share', TOOL_NAME, 'include')

def xxHash_include_dir():
    return os.path.join(ct_fuzz_root(), 'share', 'xxHash', 'include')

def ct_fuzz_lib_dir():
    return os.path.join(ct_fuzz_root(), 'share', TOOL_NAME, 'lib')

def ct_fuzz_instantiate_harness_lib():
    return os.path.join(ct_fuzz_root(), 'lib', TOOL_NAME, 'libInstantiateHarness.so')

def afl_clang_fast_path():
    return os.path.join(ct_fuzz_root(), 'bin', 'ct-fuzz-afl-clang-fast')

def ct_fuzz_shared_lib_dir():
    return os.path.join(ct_fuzz_root(), 'lib', TOOL_NAME)

def get_file_name(f):
    return os.path.splitext(os.path.basename(f))[0]

def prep_and_clean_up(func):
    def do(args):
        os.environ["ENABLE_CT_FUZZ"] = 'true'
        os.environ["AFL_DONT_OPTIMIZE"] = 'true'
        os.environ["AFL_PATH"] = ct_fuzz_shared_lib_dir()
        args.input_file_name = get_file_name(args.input_file)
        try:
            func(args)
        finally:
            remove_temp_files()
    return do

def compile_c_file(args, c_file_name):
    cmd = [afl_clang_fast_path(), '-O'+str(args.opt_level), '-c']
    cmd += [c_file_name]
    cmd += ['-emit-llvm']
    cmd += ['-I'+ct_fuzz_include_dir()]
    cmd += ['-I'+xxHash_include_dir()]
    cmd += args.compiler_options.split()
    output_file = make_file(get_file_name(c_file_name), '.bc', args)
    cmd += ['-o', output_file]
    try_command(cmd);
    return output_file

def build_libs(args):
    lib_file_paths = map(lambda l: os.path.join(ct_fuzz_lib_dir(), l), filter(
        lambda f: os.path.splitext(f)[1] == '.c', os.listdir(ct_fuzz_lib_dir())))
    return map(lambda l: compile_c_file(args, l), lib_file_paths)

def link_bc_files(input_bc_file, lib_bc_files, args):
    cmd = ['llvm-link', input_bc_file] + lib_bc_files
    args.opt_in_file = make_file(args.input_file_name+'_pre_inst', '.bc', args)
    cmd += ['-o', args.opt_in_file]
    try_command(cmd);

def run_pass(args):
    args.opt_out_file = make_file(args.input_file_name+'_post_inst', '.bc', args)
    cmd = ['opt', '-load']
    cmd += [ct_fuzz_instantiate_harness_lib()]
    cmd += ['-'+TOOL_NAME+'-instantiate-harness']
    cmd += ['-entry-point', args.entry_point]
    cmd += [args.opt_in_file, '-o', args.opt_out_file]
    try_command(cmd)

def compile_bc_to_exec(args):
    args.llc_out_file = make_file(get_file_name(args.opt_out_file), '.o', args)
    llc_cmd = ['llc', '-filetype=obj', args.opt_out_file, '-o', args.llc_out_file]
    try_command(llc_cmd)
    clang_cmd = [afl_clang_fast_path(), args.llc_out_file]
    if args.obj_file:
        clang_cmd += [args.obj_file]
    clang_cmd += ['-L`jemalloc-config --libdir`', '-Wl,-rpath,`jemalloc-config --libdir`', '-ljemalloc', '`jemalloc-config --libs`']
    clang_cmd = ' '.join(clang_cmd)
    clang_cmd += ' -lxxHash -L{0} -Wl,-rpath,{0}'.format(ct_fuzz_shared_lib_dir())
    clang_cmd += ' ' + args.compiler_options
    if args.output_file:
        clang_cmd += ' -o {0}'.format(args.output_file)
    try_command(clang_cmd, shell=True)

@prep_and_clean_up
def make_test_binary(args):
    input_bc_file = compile_c_file(args, args.input_file)
    lib_bc_files = build_libs(args)
    link_bc_files(input_bc_file, lib_bc_files, args)
    run_pass(args)
    compile_bc_to_exec(args)

def main():
    global args
    args = arguments()
    make_test_binary(args)
