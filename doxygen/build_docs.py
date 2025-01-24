#!/usr/bin/python
##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2020 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################

import os, sys, shutil, tarfile
from subprocess import call
import re

dir_out      = "./output/"

def docs_gen_all(build_release=False):
   release_version = '1'
   release_name    = 'ctrlm_all_release_' + release_version
   header_files = ['ctrlm_hal.h', 'ctrlm_hal_rf4ce.h', 'ctrlm_hal_btle.h', 'ctrlm_hal_ip.h', 'ctrlm_ipc.h', 'ctrlm_ipc_rcu.h', 'ctrlm_ipc_voice.h', 'ctrlm_ipc_key_codes.h', 'ctrlm_ipc_device_update.h']
   cmd = ["doxygen", "ctrlm_api_all"]
   ret = call(cmd)
   if build_release:
      build_pdf()
      package_release(release_name, header_files)
   else:
      launch_html()

def docs_gen_hal(build_release=False):
   release_version = '1'
   release_name    = 'ctrlm_hal_release_' + release_version
   header_files = ['ctrlm_hal.h', 'ctrlm_hal_rf4ce.h', 'ctrlm_hal_btle.h', 'ctrlm_hal_ip.h']
   cmd = ["doxygen", "ctrlm_api_hal", "ctrlm_hal_rf4ce.h", "ctrlm_hal_btle.h", "ctrlm_hal_ip.h" ]
   ret = call(cmd)
   if build_release:
      build_pdf()
      package_release(release_name, header_files)
   else:
      launch_html()

def docs_gen_hal_rf4ce(build_release=False):
   release_version = release_version_get('ctrlm_hal_rf4ce.h', 'CTRLM_HAL_RF4CE_API_VERSION')
   release_name    = 'ctrlm_hal_rf4ce_release_' + release_version
   header_files = ['ctrlm_hal.h', 'ctrlm_hal_rf4ce.h']
   cmd = ["doxygen", "ctrlm_api_hal_rf4ce" ]
   ret = call(cmd)
   if build_release:
      build_pdf()
      package_release(release_name, header_files)
   else:
      launch_html()

def docs_gen_hal_btle(build_release=False):
   release_version = '1'
   release_name    = 'ctrlm_hal_btle_release_' + release_version
   header_files = ['ctrlm_hal.h', 'ctrlm_hal_btle.h']
   cmd = ["doxygen", "ctrlm_api_hal_btle" ]
   ret = call(cmd)
   if build_release:
      build_pdf()
      package_release(release_name, header_files)
   else:
      launch_html()

def docs_gen_hal_ip(build_release=False):
   release_version = '1'
   release_name    = 'ctrlm_hal_ip_release_' + release_version
   header_files = ['ctrlm_hal.h', 'ctrlm_hal_ip.h']
   cmd = ["doxygen", "ctrlm_api_hal_ip" ]
   ret = call(cmd)
   if build_release:
      build_pdf()
      package_release(release_name, header_files)
   else:
      launch_html()

def docs_gen_ipc(build_release=False):
   release_version = '3'
   release_name    = 'ctrlm_ipc_release_' + release_version
   header_files = ['ctrlm_ipc.h', 'ctrlm_ipc_rcu.h', 'ctrlm_ipc_voice.h', 'ctrlm_ipc_key_codes.h', 'ctrlm_ipc_device_update.h']
   cmd = ["doxygen", "ctrlm_api_ipc" ]
   ret = call(cmd)
   if build_release:
      build_pdf()
      package_release(release_name, header_files)
   else:
      launch_html()

def docs_gen_ipc_device_update(build_release=False):
   release_version = '1'
   release_name    = 'ctrlm_ipc_device_update_release_' + release_version
   header_files = ['ctrlm_ipc.h', 'ctrlm_ipc_device_update.h']
   cmd = ["doxygen", "ctrlm_api_ipc_device_update" ]
   ret = call(cmd)
   if build_release:
      build_pdf()
      package_release(release_name, header_files)
   else:
      launch_html()

def docs_gen_ipc_voice(build_release=False):
   release_version = '1'
   release_name    = 'ctrlm_ipc_voice_release_' + release_version
   header_files = ['ctrlm_ipc.h', 'ctrlm_ipc_voice.h']
   cmd = ["doxygen", "ctrlm_api_ipc_voice" ]
   ret = call(cmd)
   if build_release:
      build_pdf()
      package_release(release_name, header_files)
   else:
      launch_html()

def docs_gen_ipc_rcu(build_release=False):
   release_version = '1'
   release_name    = 'ctrlm_ipc_rcu_release_' + release_version
   header_files = ['ctrlm_ipc.h', 'ctrlm_ipc_rcu.h', 'ctrlm_ipc_key_codes.h']
   cmd = ["doxygen", "ctrlm_api_ipc_rcu" ]
   ret = call(cmd)
   if build_release:
      build_pdf()
      package_release(release_name, header_files)
   else:
      launch_html()

def launch_html():
   dir_html  = os.path.join(dir_out, 'html')
   file_html = os.path.join(dir_html, 'index.html')
   cmd = ["firefox", file_html ]
   ret = call(cmd)

def build_pdf():
   os.chdir("./output/latex")
   cmd = ["pdflatex", "refman.tex" ]
   ret = call(cmd)
   os.chdir("../..")

def package_release(release_name, header_files):
   dir_html      = os.path.join(dir_out, 'html')
   dir_latex     = os.path.join(dir_out, 'latex')
   dir_rel       = os.path.join(dir_out, release_name)
   dir_rel_html  = os.path.join(dir_rel, 'html')
   file_pdf      = os.path.join(dir_latex, 'refman.pdf')
   file_rel_pdf  = os.path.join(dir_rel, 'hal_api.pdf')
   file_rel_html = os.path.join(dir_rel, 'hal_api.html')
   file_rel_tar  = os.path.join(dir_out, release_name + '.tar.gz')

   if os.path.isdir(dir_rel):
      shutil.rmtree(dir_rel)
      
   # Create release directory
   os.mkdir(dir_rel)
   # Copy html files
   shutil.copytree(dir_html, dir_rel_html)
   
   # Create html shortcut
   fh = open(file_rel_html, 'w')
   fh.write('<html><meta http-equiv="refresh" content="0;url=./html/index.html" /></html>')
   fh.close()
   
   # Copy pdf file
   shutil.copy2(file_pdf, file_rel_pdf)
   
   # Copy header files
   for file in header_files:
      shutil.copy2(os.path.join('../include', file), dir_rel)
   
   # tar up the release
   tar = tarfile.open(file_rel_tar, "w:gz")
   tar.add(dir_rel, arcname=release_name)
   tar.close()
   
   # Remove the release directory
   shutil.rmtree(dir_rel)

def release_version_get(filename, key):
   fh = open(os.path.join('../include', filename), 'r')
   contents = fh.read()
   fh.close()

   m = re.search('.*' + key + '\s*\((.*)\)', contents)
   
   return m.group(1)


#docs_gen_all()
#docs_gen_hal()
docs_gen_hal_rf4ce(True) 
#docs_gen_hal_btle()
#docs_gen_hal_ip()
#docs_gen_ipc()
#docs_gen_ipc_device_update(False)
#docs_gen_ipc_voice()
#docs_gen_ipc_rcu()
