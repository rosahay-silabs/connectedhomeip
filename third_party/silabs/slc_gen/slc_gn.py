#!/usr/bin/env python

import yaml
from pathlib import Path
import pprint


def read_key_or_pass(key, json):
    try:
        return json[key]
    except:
        return None


class SlcObject():
    condition: [str]
    unless: [str]

    def __init__(self, condition=None, unless=None):
        self.condition = condition
        self.unless = unless


class SlcPath(SlcObject):
    path: Path

    def __init__(self, json_payload):
        super().__init__()
        self.condition = read_key_or_pass('condition', json_payload)
        self.unless = read_key_or_pass('unless', json_payload)
        self.path = read_key_or_pass('path', json_payload)


class SlcFileList(SlcObject):
    path: [SlcPath]

    def __init__(self, json_payload):
        super().__init__()
        self.condition = read_key_or_pass('condition', json_payload)
        self.unless = read_key_or_pass('unless', json_payload)
        if json_payload is None:
            return
        for each_path in json_payload:
            self.path.append(SlcPath(each_path))


class SlcProvides(SlcObject):
    name: str

    def __init__(self, json_payload):
        super().__init__()
        self.name = read_key_or_pass('name', json_payload)
        self.unless = read_key_or_pass('unless', json_payload)
        self.condition = read_key_or_pass('condition', json_payload)


class SlcSource(SlcObject):
    path: [SlcPath]
    file_list: [SlcFileList]

    def __init__(self, json_payload):
        super().__init__()
        self.condition = read_key_or_pass('condition', json_payload)
        self.unless = read_key_or_pass('unless', json_payload)
        for each_path in read_key_or_pass('path', json_payload):
            self.path.append(SlcPath(each_path))
        self.file_list = SlcFileList(read_key_or_pass('file_list', json_payload))


class SlcDefine(SlcObject):
    name: str
    value: str

    def __init__(self, json_payload):
        super().__init__()
        self.name = read_key_or_pass('name', json_payload)
        self.value = read_key_or_pass('value', json_payload)
        self.condition = read_key_or_pass('condition', json_payload)
        self.unless = read_key_or_pass('unless', json_payload)


class SlcRequires(SlcObject):
    name: str


class SlcInclude(SlcObject):
    path: SlcPath
    file_list: [SlcFileList]

    def __init__(self, json_payload):
        super().__init__()
        self.condition = read_key_or_pass('condition', json_payload)
        self.unless = read_key_or_pass('unless', json_payload)
        self.file_list = SlcFileList(read_key_or_pass('file_list', json_payload))
        self.path = SlcPath(read_key_or_pass('path', json_payload))


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
    # INCLUDE
    slcc_includes = read_key_or_pass('include', slc_component)
    if slcc_includes is not None:
        for slcc_include in slcc_includes:
            slc_include = SlcInclude(slcc_include)
            sources.append(slc_include)
    # SOURCES
    slcc_sources = read_key_or_pass('source', slc_component)
    if slcc_sources is not None:
        for slcc_source in slcc_sources:
            slc_source = SlcSource(slcc_source)
            sources.append(slc_source)
    # REQUIRES
    slcc_requires = read_key_or_pass('requires', slc_component)
    if slcc_requires is not None:
        for slcc_require in slcc_requires:
            slc_require = SlcRequires(slcc_require)
            requires.append(slc_require)


root_path = ""


def main():
    parse_slcc_file("./sl_si91x_wireless.slcc")


if __name__ == "__main__":
    main()
