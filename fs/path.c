#include <string.h>
#include "sys/calls.h"
#include "fs/path.h"

int path_normalize(const char *path, char *out, bool follow_links) {
    const char *p = path;
    char *o = out;
    *o = '\0';
    int n = MAX_PATH - 1;

    // start with root or cwd, depending on whether it starts with a slash
    if (*p == '/') {
        strcpy(o, current->root);
        n -= strlen(current->root);
        o += strlen(current->root);
        // if it does start with a slash, make sure to skip all the slashes
        while (*p == '/')
            p++;
    } else {
        strcpy(o, current->pwd);
        n -= strlen(current->pwd);
        o += strlen(current->pwd);
    }

    while (*p != '\0') {
        // output a slash
        *o++ = '/'; n--;
        // copy up to a slash or null
        while (*p != '/' && *p != '\0' && --n > 0)
            *o++ = *p++;
        // eat any slashes
        while (*p == '/')
            p++;

        if (n == 0)
            return _ENAMETOOLONG;

        if (follow_links || *p != '\0') {
            // this buffer is used to store the path that we're readlinking, then
            // if it turns out to point to a symlink it's reused as the buffer
            // passed to the next path_normalize call
            char possible_symlink[MAX_PATH];
            strcpy(possible_symlink, out);
            possible_symlink[o - out] = '\0';
            struct mount *mount = find_mount_and_trim_path(possible_symlink);
            int res = mount->fs->readlink(mount, possible_symlink, out, MAX_PATH);
            if (res >= 0) {
                // readlink does not null terminate
                out[res] = '\0';
                // from this point on pretend possible_symlink is called expanded_path
                strcpy(possible_symlink, out);
                strcat(possible_symlink, "/");
                strcat(possible_symlink, p);
                return path_normalize(possible_symlink, out, follow_links);
            }
        }
    }

    *o = '\0';
    return 0;
}
