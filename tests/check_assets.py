from pathlib import Path
from html.parser import HTMLParser
root = Path(__file__).resolve().parents[1] / 'www'
class Check(HTMLParser):
    def handle_starttag(self, tag, attrs):
        for key, value in attrs:
            if key in ('src', 'href') and value and not value.startswith(('http:', 'https:', '#')):
                if value.startswith('/cgi-bin/'):
                    assert (root.parent / 'cgi-bin' / value.removeprefix('/cgi-bin/')).is_file(), f'Missing CGI: {value}'
                else:
                    assert (root / value).is_file(), f'Missing asset: {value}'
Check().feed((root / 'index.html').read_text())
print('PASS: local HTML assets exist (not an HTTP server test).')
