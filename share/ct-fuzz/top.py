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

    args = parser.parse_args()
    return args

def ct_fuzz_root():
    return os.path.dirname(os.path.dirname(os.path.abspath(sys.argv[0])))

def ct_fuzz_include_dir():
    return os.path.join(ct_fuzz_root(), 'share', TOOL_NAME, 'include')

def ct_fuzz_lib_dir():
    return os.path.join(ct_fuzz_root(), 'share', TOOL_NAME, 'lib')

def ct_fuzz_dynamic_lib():
    return os.path.join(ct_fuzz_root(), 'build', 'libCTFuzzInstrument.so')

def compile_c_file(args, c_file_name):
    cmd = ['clang', '-O1', '-c']
    cmd += [c_file_name]
    cmd += ['-emit-llvm']
    cmd += ['-I'+ct_fuzz_include_dir()]
    try_command(cmd);

def build_lib(args):
    lib_file_path = os.path.join(ct_fuzz_lib_dir(), 'ct_fuzz.c')
    compile_c_file(args, lib_file_path)

def link_bc_files(args):
    cmd = ['llvm-link', args.input_file_name+'.bc', 'ct_fuzz.bc']
    args.opt_in_file = args.input_file_name+'_pre_inst.bc'
    cmd += ['-o', args.opt_in_file]
    try_command(cmd);

def run_pass(args):
    args.opt_out_file = args.input_file_name+'_post_inst.bc'
    cmd = ['opt', '-load']
    cmd += [ct_fuzz_dynamic_lib()]
    cmd += ['-'+TOOL_NAME]
    cmd += ['-entry-point', args.entry_point]
    cmd += [args.opt_in_file, '-o', args.opt_out_file]
    try_command(cmd)

def compile_bc_to_exec(args):
    llc_cmd = ['llc', '-filetype=obj', args.opt_out_file]
    try_command(llc_cmd)
    clang_cmd = ['clang', args.input_file_name+'_post_inst.o']
    try_command(clang_cmd)

def make_test_binary(args):
    compile_c_file(args, args.input_file)
    build_lib(args)
    link_bc_files(args)
    run_pass(args)
    compile_bc_to_exec(args)

def get_file_name(args):
    args.input_file_name = os.path.splitext(os.path.basename(args.input_file))[0]

def main():
    try:
        global args
        args = arguments()
        get_file_name(args)
        make_test_binary(args)
    except KeyboardInterrupt:
        sys.exit("SMACK aborted by keyboard interrupt.")
