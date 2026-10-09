#ifndef CT_APP_PATHS_H
#define CT_APP_PATHS_H

unsigned ct_path_root_len(const char *path);
int ct_mkdirs(char *path);
int ct_exe_relative(const char *exe_path, const char *rel, char *out, unsigned cap);
int ct_migrations_dir(char *out, unsigned cap);
int ct_data_dir(const char *dir_name, char *out, unsigned cap);

#endif
