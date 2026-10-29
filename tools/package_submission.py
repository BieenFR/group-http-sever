"""Package committed, tracked files; never changes Git history or sends email."""
import io, re, subprocess, sys, tarfile
from pathlib import Path
root = Path(__file__).resolve().parents[1]
def git(*args):
    return subprocess.check_output(['git', '-C', str(root), *args])
if len(sys.argv) != 5 or not all(re.fullmatch(r'[A-Za-z0-9_]+', x) for x in sys.argv[1:]):
    sys.exit('Usage: python3 tools/package_submission.py USER1 USER2 USER3 USER4 (letters/digits/underscore)')
if git('status', '--porcelain').strip():
    sys.exit('Working tree has changes/untracked files. Review and commit intended work first.')
name = '-'.join(sys.argv[1:])
required = ['Makefile', 'README.md', 'CHECKLIST.md', 'COLLABORATION.md']
for file in required:
    git('cat-file', '-e', 'HEAD:' + file)
archive = git('archive', '--format=tar', '--prefix=' + name + '/', 'HEAD')
log = git('log', '--date=iso-strict', '--format=fuller', '--stat')
out = root / 'submission' / (name + '.tar')
out.parent.mkdir(exist_ok=True)
if out.exists():
    sys.exit('Archive already exists; move it aside before packaging another version.')
with tarfile.open(fileobj=io.BytesIO(archive), mode='r:') as source, tarfile.open(out, 'w:') as dest:
    for member in source:
        dest.addfile(member, source.extractfile(member) if member.isfile() else None)
    info = tarfile.TarInfo(name + '/git-log.txt')
    info.size = len(log)
    dest.addfile(info, io.BytesIO(log))
print(out)
print('Review archive contents and completed checklist before submitting.')
