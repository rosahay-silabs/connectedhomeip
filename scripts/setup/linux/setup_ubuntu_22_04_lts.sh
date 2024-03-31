#!/usr/bin/env bash

#
#    Copyright (c) 2024 Project CHIP Authors
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

set -ex

sudo apt-get update
sudo apt-get install -fy \
  git \
  git-lfs \
  gcc \
  g++ \
  pkg-config \
  libssl-dev \
  libdbus-1-dev \
  libglib2.0-dev \
  libavahi-client-dev \
  ninja-build \
  python3-venv \
  python3-dev \
  python3-pip \
  unzip \
  libgirepository1.0-dev \
  libcairo2-dev \
  libreadline-dev &&
  true

pip install clang==16.0.6
pip install clang-format==16.0.6
