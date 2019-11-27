import os
import sys
import subprocess

script_path = os.path.abspath(__file__)
script_dir = os.path.dirname(script_path)
os.chdir(script_dir)

glsl_dir = os.path.abspath('../../glsl')
spirv_dir = os.path.abspath('../../assets')

glsl_files = [file for file in os.listdir('../../glsl') if file[0] != '.']

for glsl_file in glsl_files:
  try:
    command = [
      './glslc',
      os.path.join(glsl_dir, glsl_file),
      '-o',
      os.path.join(spirv_dir, glsl_file + '.spv')
    ]
    subprocess.check_output(command, stderr=subprocess.STDOUT, text=True)
  except subprocess.CalledProcessError as e:
    print(e.output)
    sys.exit(1)




