#!/usr/bin/env python3
"""Compile fixtures and validate the real executable and path checker."""
import argparse
import json
import os
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser()
for flag in ('clang', 'cfga', 'checker', 'output-dir'):
    parser.add_argument('--' + flag, required=True, type=Path)
args = parser.parse_args()
output = args.output_dir.resolve()
output.mkdir(parents=True, exist_ok=True)
fixtures = Path(__file__).resolve().parents[2] / 'Test-Cases'
expected = {'simple-call': 1, 'multi-call': 1, 'loop': 1, 'branch': 2,
            'buffer': 4, 'nested-call': 1, 'shared-callee-branch': 2,
            'external-call': 1}
results = []
for name, count in expected.items():
    case_dir = output / name
    case_dir.mkdir(exist_ok=True)
    bitcode = case_dir / (name + '.bc')
    env = dict(os.environ, CFGA_EXPECTED_PATHS=str(count))
    commands = [
        [str(args.clang.resolve()), '-O0', '-g', '-Wno-main-return-type',
         '-emit-llvm', '-c', str(fixtures / (name + '.c')), '-o', str(bitcode)],
        [str(args.cfga.resolve()), str(bitcode)],
        [str(args.checker.resolve()), str(bitcode)],
    ]
    logs = []
    passed = True
    try:
        for command in commands:
            process = subprocess.run(command, cwd=case_dir, env=env,
                                     stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                     text=True, timeout=30)
            logs.append('$ ' + ' '.join(command) + '\n' + process.stdout)
            if process.returncode:
                raise RuntimeError('exit status ' + str(process.returncode))
        paths = [line.strip() for line in Path(str(bitcode) + '.res.txt').read_text().splitlines()
                 if line.strip()]
        if len(paths) != count or len(set(paths)) != count:
            raise RuntimeError('executable path count/deduplication mismatch')
    except (RuntimeError, subprocess.TimeoutExpired, OSError) as error:
        passed = False
        logs.append(str(error))
    (case_dir / 'test.log').write_text('\n'.join(logs))
    results.append({'case': name, 'expected_paths': count, 'passed': passed})
    print(('PASS' if passed else 'FAIL') + ': ' + name, flush=True)
(output / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
raise SystemExit(0 if all(result['passed'] for result in results) else 1)
