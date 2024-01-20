import os
import errno
import stat
import shutil
from pathlib import Path
from datetime import datetime

from conan import ConanFile
from conan.tools.scm import Git

required_conan_version = ">=1.51.0"

def handleRemoveReadonly(func, path, exc):
  excvalue = exc[1]
  if excvalue.errno == errno.EACCES:
      os.chmod(path, stat.S_IRWXU| stat.S_IRWXG| stat.S_IRWXO) # 0777
      func(path)
  else:
      raise

class ClippConan(ConanFile):
    name = "clipp"
    description = "Command line interfaces for Modern C++"
    license = "MIT"
    topics = ("Windows", "c++")
    homepage = "https://github.com/risa2000/clipp"
    url = "https://github.com/risa2000/clipp"
    branch = "master"
    commit = "337f2264f240862bdc1eed3e9b0fc71107cacda8"
    exports_sources = "include/*"
    no_copy_source = True

    def _fix_isodatetime(self, tms):
        if len(tms) > 5 and (tms[-5] in ('+', '-')):
            return tms[:-2] + ':' + tms[-2:]

    def _get_commit_datetime(self):
        self.source()
        git = Git(self)
        return datetime.fromisoformat(self._fix_isodatetime(git.run(f'-C {self.branch} show -s --format="%ci" .')))

    def set_version(self):
        commit_datetime = self._get_commit_datetime()
        self.version = f'cci.{commit_datetime:%Y%m%d}'

    def source(self):
        git = Git(self)
        if Path(self.branch).exists():
            shutil.rmtree(self.branch, ignore_errors=False, onerror=handleRemoveReadonly)
        git.clone(url=self.url, target=self.branch)
        git.run(f"-C {self.branch} checkout {self.commit}")

    def layout(self):
        self.folders.source = self.branch

    def package(self):
        self.copy("LICENSE", src=self.folders.source, dst="licenses")
        self.copy("*.h", dst="include/clipp", src=os.path.join(self.folders.source, "include"))

    def package_id(self):
        self.info.clear()
