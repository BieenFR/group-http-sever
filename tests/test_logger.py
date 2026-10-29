from pathlib import Path
import os
import shlex
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='sws-logger-') as directory:
    work=Path(directory); binary=work/'logger-test'; log=work/'requests.log'
    subprocess.run(shlex.split(os.environ.get('CC','cc'))+['-D_POSIX_C_SOURCE=200809L','-I'+str(root/'include'),'-std=c11','-Wall','-Wextra','-Wpedantic','-Werror',str(root/'src/logger.c'),str(root/'tests/test_logger.c'),'-o',str(binary)],check=True)
    subprocess.run([str(binary),str(log),'0'],check=True)
    lines=log.read_text().splitlines()
    assert len(lines)==100
    assert len(set(lines))==1
    assert '\\"quote\\\\path??injected' in lines[0], lines[0]
    assert lines[0].startswith('127.0.0.1 1970-01-01T00:00:00Z ')
    subprocess.run([str(binary),str(log),'0'],check=True)
    assert len(log.read_text().splitlines())==200
    missing=work/'not-created.log'
    subprocess.run([str(binary),str(missing),'2'],check=True)
    assert not missing.exists()
    result=subprocess.run([str(binary),str(missing),'1'],capture_output=True,check=True)
    assert len(result.stdout.splitlines())==100 and not missing.exists()
    print('PASS: logger concurrent append, sanitization, GMT, disabled logging and debug precedence')
