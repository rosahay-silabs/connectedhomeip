#!/usr/bin/env python

import yaml
from pathlib import Path
import pprint


def read_key_or_pass(key, json):
    try:
        return json[key]
    except:
        return None


def read_key_or_error(key, json):
    try:
        return json[key]
    except:
        print(f"ERROR: failed to find %s" % key)
        pprint(json)
        os.exit(0)


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
    path = []

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
    path = []
    file_list = []

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


class SlcComponent():
    id: str
    package: str
    description: str
    label: str
    category: str
    quality: str
    component_root_path: str
    provides: [SlcProvides]
    source: [SlcSource]
    define: [SlcDefine]
    requires: [SlcRequires]
    include: [SlcInclude]

    def __init__(self):
        self.id = ""
        self.package = ""
        self.description = ""
        self.label = ""
        self.category = ""
        self.quality = ""
        self.component_root_path = ""
        self.provides = []
        self.source = []
        self.define = []
        self.requires = []
        self.include = []

    def parse(self, slcc_path):
        slcc_path = Path(slcc_path)
        if not slcc_path.exists():
            print("%s doesnt exist" % slcc_path)
            return
        with open(slcc_path, "r") as stream:
            try:
                slc_component = yaml.safe_load(stream)

            except yaml.YAMLError as exc:
                print(exc)
        # SLCC_ID
        self.id = read_key_or_error('id', slc_component)
        # COMPONENT ROOT PATH
        self.component_root_path = read_key_or_pass('component_root_path', slc_component)
        # PROVIDES
        slcc_provides = read_key_or_pass('provides', slc_component)
        if slcc_provides is not None:
            for slcc_provide in slcc_provides:
                slc_provides = SlcProvides(slcc_provide)
                self.provides.append(slc_provides)
        # DEFINES
        slcc_defines = read_key_or_pass('define', slc_component)
        if slcc_defines is not None:
            for slcc_define in slcc_defines:
                slc_define = SlcDefine(slcc_define)
                self.define.append(slc_define)
        # INCLUDE
        slcc_includes = read_key_or_pass('include', slc_component)
        if slcc_includes is not None:
            for slcc_include in slcc_includes:
                slc_include = SlcInclude(slcc_include)
                self.include.append(slc_include)
        # SOURCES
        slcc_sources = read_key_or_pass('source', slc_component)
        if slcc_sources is not None:
            for slcc_source in slcc_sources:
                slc_source = SlcSource(slcc_source)
                self.source.append(slc_source)
        # REQUIRES
        slcc_requires = read_key_or_pass('requires', slc_component)
        if slcc_requires is not None:
            for slcc_require in slcc_requires:
                slc_require = SlcRequires(slcc_require)
                self.requires.append(slc_require)

    def __str__(self):
        return '''
        config({}_config) {{

        }}

        source_set({}) {{

        }}'''.format(self.id, self.id)


class NinjaComponent():
    source_set: str
    include_dirs: [str]
    sources: [str]
    defines: [str]
    public_deps: [str]
    public_configs: [str]
    provides: [str]

    def __init__(self):
        self.source_set = ""
        self.include_dirs = []
        self.sources = []
        self.defines = []
        self.public_deps = []
        self.public_configs = []
        self.provides = []

    def __str__(self):
        return f"""
        config({self.source_set}_config) {{
            include_dirs = {self.include_dirs}
            defines = {self.defines}
            public_deps = {self.public_deps}
        }}
        source_set({self.source_set}) {{
            sources = {self.sources}
            public_configs = {self.public_configs}
        }}
        """

    def save(self, out_file):
        with open(out_file, "w") as build_gn:
            build_gn.write(f"config(\"%s_config\")" % (source_set))
            build_gn.write('{')
            build_gn.write('\n'.join(f"{x}" for x in provides))
            build_gn.write('\n')
            build_gn.write('include_dirs = ')
            build_gn.write('[')
            build_gn.write(','.join(f"\"{x}\"" for x in include_dirs))
            build_gn.write(']')
            build_gn.write('defines = ')
            build_gn.write('[')
            build_gn.write(','.join(f"\"{x}\"" for x in defines))
            build_gn.write(']')
            build_gn.write('public_deps = ')
            build_gn.write('[')
            build_gn.write(','.join(f"\"{x}\"" for x in public_deps))
            build_gn.write(']')
            build_gn.write('}')
            build_gn.write(f"source_set(\"%s\")" % (source_set))
            build_gn.write('{')
            build_gn.write('sources = ')
            build_gn.write('[')
            build_gn.write(','.join(f"\"{x}\"" for x in sources))
            build_gn.write(']')
            build_gn.write('public_configs = ')
            build_gn.write('[')
            build_gn.write(','.join(f"\"{x}\"" for x in public_configs))
            build_gn.write(']')
            build_gn.write('}')


### GLOBAL VARIABLES ###
debug = True
root_path = ""
output_path = ""


def main():
    slc_component = SlcComponent()
    slc_component.parse("./sl_si91x_wireless.slcc")
    print(slc_component)


if __name__ == "__main__":
    main()
