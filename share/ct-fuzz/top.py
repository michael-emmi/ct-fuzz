from utils import *
import argparse
import os
import sys

TOOL_NAME = 'ct-fuzz'

def arguments():
    parser = argparse.ArgumentParser()

    parser.add_argument('input_file', metavar='FILE', default=None, type=str,
            help='specification file (can also contain source functions to fuzz)')
    parser.add_argument('--obj-file', metavar='FILE', default=None, type=str,
            help='input object file')
    parser.add_argument('-d', '--debug', action="store_true", default=False,
            help='enable debugging output')
    parser.add_argument('--entry-point', metavar='PROC', default=None, type=str,
            help='entry point function to start with')
    parser.add_argument('--opt-level', metavar='NUM', default=2, type=int,
            help='compiler optimization level')
    parser.add_argument('--compiler-options', metavar='OPTIONS', default='', type=str,
            help='compiler options')
    parser.add_argument('-o', '--output-file', metavar='FILE', default=None, type=str,
            help='output binary')
    parser.add_argument('--seed-file', metavar='FILE', default=None, type=str,
            help='file containing generated seeds (optional)')
    parser.add_argument('--memory-leakage', metavar='OPTIONS', default='address', type=str,
            help='memory leakage model: address (default), cache')

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

def ct_fuzz_generate_seed_lib():
    return os.path.join(ct_fuzz_root(), 'lib', TOOL_NAME, 'libGenerateSeeds.so')

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

def compile_c_file(args, c_file_name, lib=False):
    cmd = [afl_clang_fast_path(), '-O'+str(args.opt_level), '-c']
    cmd += [c_file_name]
    cmd += ['-emit-llvm']
    cmd += ['-I'+ct_fuzz_include_dir()]
    cmd += ['-I'+xxHash_include_dir()]
    if not lib:
      cmd += args.compiler_options.split()
    output_file = make_file(get_file_name(c_file_name), '.bc', args)
    cmd += ['-o', output_file]
    if sys.stdout.isatty(): cmd += ['-fcolor-diagnostics']
    cmd += ['-Wno-incompatible-pointer-types-discards-qualifiers']
    try_command(cmd);
    return output_file

def build_libs(args):
    lib_file_paths = map(lambda l: os.path.join(ct_fuzz_lib_dir(), l), filter(
        lambda f: os.path.splitext(f)[1] == '.c', os.listdir(ct_fuzz_lib_dir())))
    return map(lambda l: compile_c_file(args, l, True), lib_file_paths)

def link_bc_files(input_bc_file, lib_bc_files, args):
    f = lambda f: not (('cache' if args.memory_leakage == 'address' else 'memory') in f)
    cmd = ['llvm-link', input_bc_file] + filter(f, lib_bc_files)
    args.opt_in_file = make_file(args.input_file_name+'_pre_inst', '.bc', args)
    cmd += ['-o', args.opt_in_file]
    try_command(cmd);

def run_opt(lib_path, pass_opt, input_file, output_file, additional_args=[]):
    cmd = ['opt', '-load']
    cmd += [lib_path]
    cmd += ['-'+TOOL_NAME+pass_opt]
    cmd += ['-entry-point', args.entry_point]
    cmd += [input_file, '-o', output_file]
    cmd += additional_args
    return try_command(cmd)

def instantiate_harness(args):
    args.opt_out_file = make_file(args.input_file_name+'_post_inst', '.bc', args)
    run_opt(ct_fuzz_instantiate_harness_lib(),
            '-instantiate-harness',
            args.opt_in_file, args.opt_out_file)

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

def generate_seeds(input_bc_file, args):
    output = run_opt(ct_fuzz_generate_seed_lib(),
            '-generate-seeds',
            input_bc_file, input_bc_file,
            ['-output-file', args.seed_file] if args.seed_file else [])
    if not args.seed_file:
        print output

@prep_and_clean_up
def make_test_binary(args):
    input_bc_file = compile_c_file(args, args.input_file)
    generate_seeds(input_bc_file, args)
    lib_bc_files = build_libs(args)
    link_bc_files(input_bc_file, lib_bc_files, args)
    instantiate_harness(args)
    compile_bc_to_exec(args)

def main():
    global args
    args = arguments()
    make_test_binary(args)
