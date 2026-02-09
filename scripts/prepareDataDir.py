Import("env");
import sys, os, re;
from shutil import copytree;

# Print environment variables for debugging
#print("Environment dump:")
#for key, value in env.items():
#   print(f"  {key}: {value}")
#print()


data_master_dir = "esp_files";
if (os.path.exists(data_master_dir +"/"+ env["BOARD"])):
    copytree(data_master_dir +"/"+ env["BOARD"] + "/" , ".", dirs_exist_ok=True);
    print("copy board specific files from:<" + data_master_dir +"/"+ env["BOARD"] + "> to <root>");
else:
    print("path not exists: " + data_master_dir +"/"+ env["BOARD"] + "/");