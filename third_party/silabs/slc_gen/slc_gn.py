#!/usr/bin/env python

import yaml
from pathlib import Path
import pprint


def read_key_or_pass(key, json):
    try:
        return json[key]
    except:
        print("returned none")
        return None


class SlcObject():
    condition: str
    unless: str

    def __init__(self, condition=None, unless=None):
        self.condition = condition
        self.unless = unless


class SlcPath(SlcObject):
    path: Path

    def __init__(self, slc_path):
        super().__init__()
        self.path = Path(slc_path)


class SlcFileList():
    file_list: [SlcPath]

    def __init__(self, slc_file_list):
        super().__init__()
        for slc_file in slc_file_list:
            self.file_list.append(SlcPath(slc_file))


class SlcProvides(SlcObject):
    name: str

    def __init__(self, provideYAML):
        super().__init__()
        self.name = provideYAML['name']


class SlcSource(SlcObject):
    path: [SlcPath]
    file_list: [SlcFileList]


class SlcDefine(SlcObject):
    name: str
    value: str

    def __init__(self, defineYAML):




class SlcRequires(SlcObject):
    name: str


class SlcInclude(SlcObject):
    path: [SlcPath]
    file_list: [SlcFileList]


include_dirs = []
sources = []
defines = []
public_deps = []
public_configs = []
provides = []


def parse_slcc_file(slcc_path):
    slcc_path = Path(slcc_path)
    if not slcc_path.exists():
        print("%s doesnt exist" % slcc_path)
        return
    with open(slcc_path, "r") as stream:
        try:
            slc_component = yaml.safe_load(stream)
            pprint.pprint(slc_component)

        except yaml.YAMLError as exc:
            print(exc)
    # COMPONENT ROOT PATH
    component_root_path = read_key_or_pass('component_root_path', slc_component)
    # PROVIDES
    slcc_provides = read_key_or_pass('provides', slc_component)
    if slcc_provides is not None:
        for slcc_provide in slcc_provides:
            slc_provides = SlcProvides(slcc_provide)
            provides.append(slc_provides)
    # DEFINES
    slcc_defines = read_key_or_pass('define', slc_component)
    if slcc_defines is not None:
        for slcc_define in slcc_defines:
            slc_define = SlcDefine(slcc_define)
            defines.append(slc_define)


root_path = ""


def main():
    parse_slcc_file("./sl_si91x_wireless.slcc")


if __name__ == "__main__":
    main()
