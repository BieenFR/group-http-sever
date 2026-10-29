from pathlib import Path
import os
import shlex
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='sws-request-') as directory:
    binary = Path(directory)/'request-test'
    subprocess.run(shlex.split(os.environ.get('CC', 'cc')) + ['-D_POSIX_C_SOURCE=200809L', '-I'+str(root/'include'), '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', str(root/'src/request.c'), str(root/'tests/test_request.c'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
