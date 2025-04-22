import sys
import os

script_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.dirname(script_dir)  # Assumes script is in python_scripts/
build_dir = os.path.join(project_root, "cpp_integration", "build")
sys.path.insert(0, build_dir)

import cpp_astar

print("Successfully imported C++ module 'my_astar_module'")
result = cpp_astar.add(1, 2)
print(result)
