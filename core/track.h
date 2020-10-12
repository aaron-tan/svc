#ifndef tracker_h
#define tracker_h

// Create a copy of file_name with an extension .cpy
void copy_file(int hash_file, char* file_path, char* file_name);

/** Tracks any changes to filename and creates a patch file in the
*   HEAD directory. This patch file will be used to apply changes
*   when calling svc_commit
*/
FILE* create_diff_file(int hash_file, char* file_path, char* filename);

#endif
