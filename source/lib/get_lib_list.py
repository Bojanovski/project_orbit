import os
import shutil

def find_files(dir_path, extension):
    file_list = []
    for root, dirs, files in os.walk(dir_path):
        for file in files:
            if file.endswith(extension):
                file_list.append(os.path.join(root, file))
    return file_list


def save_list_to_file(lst, filename):
    with open(filename, 'w') as file:
        file.write("ONE LINE:\n")
        for item in lst:
            file.write("%s;" % item)
        file.write("\n\nREADABLE:\n")
        for item in lst:
            file.write("%s\n" % item)


def remove_duplicates(lst):
    return list(set(lst))
    
    
def create_folder(folder_path):
    if not os.path.exists(folder_path):
        os.makedirs(folder_path)
    

def is_debug_dll(dll_name):
    debug_dll_names = [ 
        "Iex_d.dll",
        "IlmThread_d.dll",
        "MaterialXCore_d.dll",
        "MaterialXFormat_d.dll",
        "MaterialXGenGlsl_d.dll",
        "MaterialXGenMdl_d.dll",
        "MaterialXGenOsl_d.dll",
        "MaterialXGenShader_d.dll",
        "MaterialXRenderGlsl_d.dll",
        "MaterialXRenderHw_d.dll",
        "MaterialXRenderOsl_d.dll",
        "MaterialXRender_d.dll",
        "OpenEXRCore_d.dll",
        "OpenEXRUtil_d.dll",
        "OpenEXR_d.dll",
        "OpenImageIO_Util_d.dll",
        "OpenImageIO_d.dll",
        "imath_d.dll",
        "libgmpxx_d.dll",
        "openvdb_d.dll",
        "pyshellext_d.dll",
        "python310_d.dll",
        "python3_d.dll",
        "shaderc_shared_d.dll",
        "sqlite3_d.dll",
        "tbb_debug.dll",
        "tbbmalloc_debug.dll",
        "tbbmalloc_proxy_debug.dll",
        "usd_ms_d.dll",
        "sycl6d.dll",
        "OpenColorIO_d_2_2.dll",
        "openvdb.dll",
        "boost_thread-vc142-mt-gyd-x64-1_80.dll",
        "tbb.dll",
        "epoxy-0.dll",
        "SDL2.dll",
        ]
    return dll_name in debug_dll_names
    

def is_optimized_dll(dll_name):
    optimized_dll_names = [
        "epoxy-0.dll",
        "SDL2.dll",
        "OpenColorIO_2_2.dll",
        "openvdb.dll",
        "imath.dll",
        "boost_thread-vc142-mt-x64-1_80.dll",
        "OpenImageIO_Util.dll",
        "OpenImageIO.dll",
        "tbb.dll",
        "OpenEXRCore.dll",
        "OpenEXRUtil.dll",
        "OpenEXR.dll",
        "Iex.dll",
        "IlmThread.dll",
        ]
    return dll_name in optimized_dll_names


# first create the folder to hold the files (and make sure it's empty)
folder_path = './library_lists'
shutil.rmtree(folder_path)
create_folder(folder_path)
    
## then handle the LIBs
#lib_list = find_files(".", ".lib")
#debug_libs = []
#optimized_libs = []
#lib_dirs = []
#for line in lib_list:
#    dirname = os.path.dirname(line)
#    dirname = "$(SolutionDir)lib" + dirname[1:]
#    lib_dirs.append(dirname)
#    if line.endswith("_d.lib"):
#        debug_libs.append(os.path.basename(line))
#    else:
#        optimized_libs.append(os.path.basename(line))
#     
#debug_libs = remove_duplicates(debug_libs)
#optimized_libs = remove_duplicates(optimized_libs)
#lib_dirs = remove_duplicates(lib_dirs)
#
#save_list_to_file(debug_libs, folder_path + "\debug_libs.txt")
#save_list_to_file(optimized_libs, folder_path + "\optimized_libs.txt")
#save_list_to_file(lib_dirs, folder_path + "\lib_dirs.txt")

# now the DLLs
debug_dll_folder_path = './library_lists/debug_dll'
create_folder(debug_dll_folder_path)
optimized_dll_folder_path = './library_lists/optimized_dll'
create_folder(optimized_dll_folder_path)
current_folder = os.getcwd()
dll_list = find_files(current_folder, ".dll")
debug_dlls = []
optimized_dlls = []
for line in dll_list:
    dll_name = os.path.basename(line)
    if is_debug_dll(dll_name):
        shutil.copy(line, debug_dll_folder_path)
    if is_optimized_dll(dll_name):
        shutil.copy(line, optimized_dll_folder_path)
        

