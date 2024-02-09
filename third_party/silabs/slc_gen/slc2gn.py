#!/usr/bin/env python

import glob
import os
import subprocess
import sys
import yaml
from pathlib import Path


def generate_ninja_file(slcPath):
    slcPath = Path(slcPath)
    if not slcPath.exists():
        print("PATH DOESNT EXISTS")
        return
    with open(slcPath, "r") as stream:
        try:
            slc_file = yaml.safe_load(stream)

        except yaml.YAMLError as exc:
            print(exc)
    include_dirs = []
    sources = []
    defines = []
    public_deps = []
    public_configs = []
    provides = []
    component_root_path = slcPath
    try:
        source_set = slc_file['id']
    except:
        print(slc_file)
        exit(0)

    try:
        component_root_path = Path(slc_file['component_root_path'])
    except:
        print(slc_file)
        exit(0)
    # PROVIDES
    try:
        for provide in slc_file['provides']:
            provides.append(provide['name'] + " = true")
    except:
        pass
    # INCLUDE
    try:
        for include in slc_file['include']:
            include_path = component_root_path.joinpath(Path(include['path']))
            include_dirs.append(include_path)
            try:
                for file in include['file_list']:
                    sources.append(include_path.joinpath(file['path']))
            except:
                pass
    except:
        pass
    # SOURCE
    try:
        for item in slc_file['source']:
            sources.append(component_root_path.joinpath(item['path']))
    except:
        pass
    # DEFINE
    try:
        for item in slc_file['define']:
            item_define = item['name']
            try:
                item_define += item['value']
            except:
                pass
            defines.append(item_define)
    except:
        pass
    # REQUIRES
    try:
        for item in slc_file['requires']:
            public_deps.append(f":%s" % item['name'])
    except:
        pass

    public_configs.append(f":%s_config" % source_set)
    # BUILD.gn
    with open("BUILD.gn", "w") as build_gn:
        build_gn.write(f"config(\"%s_config\")" % (source_set))
        build_gn.write('{')
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
        build_gn.write('\n'.join(f"{x}" for x in provides))
        build_gn.write('}')
    print(f"config(\"%s_config\")" % (source_set))
    print('{')
    print('include_dirs = ')
    print(include_dirs)
    print('defines = ')
    print(defines)
    print('public_deps = ')
    print(public_deps)
    print('}')
    print(f"source_set(\"%s\")" % (source_set))
    print('{')
    print('sources = ')
    print(sources)
    print('public_configs = ')
    print(public_configs)
    print('}')


def main():
    # wiseconnect_path = Path("../wifi_sdk")
    # wiseconnect_slcc_items = list(wiseconnect_path.glob('components/device/**/*.slcc'))
    # for slcc_item in wiseconnect_slcc_items:
    #     generate_ninja_file(slcc_item)
    generate_ninja_file("./sl_si91x_wireless.slcc")


if __name__ == "__main__":
    main()

# | sed s/PosixPath\(//g | sed s/\'\)/\'/g | sed "s/\'/\"/g"
