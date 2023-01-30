import os
from datetime import datetime

from conan import ConanFile
from conan.tools.scm import Git

required_conan_version = ">=1.51.0"

class ClippConan(ConanFile):
    name = "clipp"
    description = "Command line interfaces for Modern C++"
    license = "MIT"
    topics = ("Windows", "c++")
    homepage = "https://github.com/GerHobbelt/clipp"
    url = "https://github.com/GerHobbelt/clipp"
    branch = "master"
    commit = "02783b6782ebedbb2bebc2e6ceda738ee51c7df2"
    commit_datetime = datetime.fromisoformat("2022-12-23 16:14:30 +01:00")
    exports_sources = "include/*"
    no_copy_source = True

    def _fix_isodatetime(self, tms):
        if len(tms) > 5 and (tms[-5] in ('+', '-')):
            return tms[:-2] + ':' + tms[-2:]

    def set_version(self):
        self.version = f'cci.{self.commit_datetime:%Y%m%d}'

    def source(self):
        git = Git(self)
        git.clone(url=self.url, target=self.branch)
        git.run(f"-C {self.branch} checkout {self.commit}")
        commit_datetime = datetime.fromisoformat(self._fix_isodatetime(git.run(f'-C {self.branch} show -s --format="%ci" .')))
        assert self.commit_datetime == commit_datetime

    def layout(self):
        self.folders.source = self.branch

    def package(self):
        self.copy("LICENSE", dst="licenses")
        self.copy("*.h", dst="include/clipp", src=os.path.join(self.folders.source, "include"))

    def package_id(self):
        self.info.clear()
