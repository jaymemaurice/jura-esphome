from pathlib import Path
import re
import subprocess
import shutil
import tempfile

ROOT = Path(__file__).resolve().parents[1]
WEB = (ROOT / "components/jura_web/jura_web.cpp").read_text()


def test_status_uses_supported_null_terminated_esp8266_string():
    status = WEB.split("void JuraWeb::send_status_(", 1)[1].split("void JuraWeb::send_snapshot_(",1)[0]
    assert "char b[224]" in status
    assert "std::snprintf(b,sizeof(b)," in status
    assert 'req->send(200,"application/json",String(b));' in status
    assert "const size_t n=std::snprintf" not in status
    assert not re.search(r"String\s*\(\s*b\s*,",status)


def test_no_unsupported_esp8266_string_buffer_length_ctors():
    assert not re.search(r"String\s*\(\s*[A-Za-z_]\w*\s*,\s*(?:n|len|length|size)\b",WEB)


def test_compilable_with_character_string_constructor_and_snprintf():
    compiler = shutil.which("g++") or shutil.which("c++")
    if not compiler:
        return  # source checks still run in ESPHome-only environments
    source = r'''#include <cstdio>
#include <cstring>
#include <string>
struct String { explicit String(const char *s) : data(s) {} std::string data; };
struct Request { void send(int,const char*,String){} };
int main() { char b[224]; std::snprintf(b,sizeof(b),"{\"pages\":%u}",16U);
  Request req; req.send(200,"application/json",String(b));
  return std::strcmp(b,"{\"pages\":16}"); }
'''
    with tempfile.TemporaryDirectory() as tmp:
        file = Path(tmp)/"string_ctor.cpp"
        binary = Path(tmp)/"string_ctor"
        file.write_text(source)
        result = subprocess.run([compiler,"-std=c++17","-Wall","-Wextra","-Werror",str(file),"-o",str(binary)],capture_output=True,text=True)
        assert result.returncode==0, result.stderr
        assert subprocess.run([str(binary)]).returncode==0
