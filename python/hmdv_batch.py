import os
import subprocess
import sys

from argparse import Namespace
from collections import namedtuple
from concurrent.futures import ThreadPoolExecutor
from functools import partial
from glob import glob
from pathlib import Path
from shutil import which

#   globals
#-------------------------------------------------------------------------------
COMMANDS = ['verify', 'transform']
DEFAULT_COMMAND = 'verify'
HMDV_EXE = 'hmdv.exe'

#   classes 
#-------------------------------------------------------------------------------
# result of the one datamark processing
Result = namedtuple('Result', ('fpath', 'result', 'text'))

#   functions
#-------------------------------------------------------------------------------
def process_one_file(fpath: Path, args: Namespace) -> Result:
    """Process one hmdq data file."""
    cargs = [args.hmdv_exe]
    if args.cmd == 'transform':
        #outfile = fpath.with_suffix('.hmdv.json')
        outfile = fpath
        cargs += ['all', '--out_json', str(outfile), f'-v{args.verbose}']
    else:
        cargs += ['verify']
    cargs.append(str(fpath))
    if args.verbose > 0:
        print(f'Processing: {cargs}')
    res = subprocess.run(cargs, capture_output=True)
    tres = 'OK' if res.returncode == 0 else 'ERROR'
    text = f'[{tres}] {str(fpath)}'
    return Result(fpath, res.stdout, text)

def process_one_file_done(future, results):
    """Print the data from the process_one_file.
    This is future callback attached by `future.add_done_callback`."""
    res = future.result()
    print(res.text)
    results.append(res)

def runner(args: Namespace) -> None:
    """run hmdv on each file in the directory structure"""
    ifiles = args.dir_path.rglob('*.json')
    # get the defaults for the render_hmdq.main
    results = []
    if args.parallel:
        worker_count = os.cpu_count() * 3 // 2
        if args.verbose > 0:
            print(f'Running {worker_count} workers in parallel')
        with ThreadPoolExecutor(max_workers=worker_count) as e:
            for f in ifiles:
                future = e.submit(process_one_file, f, args)
                future.add_done_callback(partial(process_one_file_done, results=results))
    else:
        for f in ifiles:
            result = process_one_file(f, args)
            results.append(result)
            print(result.text)
    print('Finished...')
    print()
    for r in results:
        if not r.result:
            print(f'ERROR: {r.text}')
    return results

#   main
#-------------------------------------------------------------------------------
if __name__ == '__main__':

    from argparse import ArgumentParser

    ap = ArgumentParser(description='run hmdv on a directory for verify or transform.')

    ap.add_argument('cmd', default=DEFAULT_COMMAND, choices=COMMANDS, help=f'command [%(default)s] out of {COMMANDS}')
    ap.add_argument('dir_path', help='input file/directory with JSON data files')
    ap.add_argument('-v', '--verbose', action='count', default=0, help='verbose level [%(default)s]')
    ap.add_argument('-p', '--parallel', action='store_true', help='run parallel jobs')
    ap.add_argument('-x', '--hmdv_exe', help='path to hmdv executable [%(default)s]', default=HMDV_EXE)

    args = ap.parse_args()
    args.prog_dir = Path(sys.argv[0]).parent

    if Path(args.dir_path).exists():
        args.dir_path = Path(args.dir_path)
    else:
        print(f'Directory {args.dir_path} does not exist', file=sys.stderr)
        sys.exit(1)

    args.hmdv_exe = which(args.hmdv_exe)

    runner(args)

