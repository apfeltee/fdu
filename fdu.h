
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

typedef struct visitor_t visitor_t;
typedef struct iteminfo_t iteminfo_t;
typedef bool(*handler_t)(iteminfo_t*, void*);

enum
{
    kMaxPathLength = 1024,
};

enum itemtype_t
{
    FDU_T_File,
    FDU_T_Directory,
    FDU_T_Other,
};

struct iteminfo_t
{
    enum itemtype_t type;
    struct stat st;
    size_t size;
    char path[kMaxPathLength];
};

struct visitor_t
{
    const char* dirn;
    DIR* handle;
    struct dirent* dent;
};

void fdu_joinpath(char *destination, const char *path1, const char *path2)
{
    int cdir;
    size_t len;
    if (path1 && *path1)
    {
        len = strlen(path1);
        strcpy(destination, path1);
        cdir = destination[len - 1];
        if((cdir == '/') || (cdir == '\\'))
        {
            if (path2 && *path2)
            {
                strcpy(destination + len, ((*path2 == '/') || (*path2 == '\\')) ? (path2 + 1) : path2);
            }
        }
        else
        {
            if (path2 && *path2)
            {
                if((*path2 == '/') || (*path2 == '\\'))
                {
                    strcpy(destination + len, path2);
                }
                else
                {
                    destination[len] = '/';
                    strcpy(destination + len + 1, path2);
                }
            }
        }
    }
    else if (path2 && *path2)
    {
        strcpy(destination, path2);
    }
    else
    {
        destination[0] = '\0';
    }
}


bool fdu_open(visitor_t* d, const char* path)
{
    d->dirn = path;
    if((d->handle = opendir(path)) == NULL)
    {
        return false;
    }
    d->dent = NULL;
    return true;
}

void fdu_close(visitor_t* d)
{
    if(d->handle != NULL)
    {
        closedir(d->handle);
    }
}

bool fdu_prepandvisit(visitor_t* d, iteminfo_t* it, handler_t successfn, handler_t failfn, void* up)
{
    fdu_joinpath(it->path, d->dirn, d->dent->d_name);
    switch(d->dent->d_type)
    {
        case DT_REG:
            it->type = FDU_T_File;
            break;
        case DT_DIR:
            it->type = FDU_T_Directory;
            break;
        default:
            it->type = FDU_T_Other;
            break;
    }
    it->size = 0;
    if(stat(it->path, &it->st) == -1)
    {
        if(failfn(it, up) == false)
        {
            return false;
        }
    }
    else
    {
        it->size = it->st.st_size;
        if(successfn(it, up) == false)
        {
            return false;
        }
    }
    return true;
}

bool fdu_isdot(const char* s)
{
    return (
        ((s[0] == '.') && (s[1] == 0)) ||
        ((s[0] == '.') && (s[1] == '.') && (s[2] == 0))
    );
}

void fdu_visit(visitor_t* d, handler_t successfn, handler_t failfn, handler_t recfailfn, bool recurse, void* up)
{
    visitor_t subd;
    iteminfo_t it;
    while(true)
    {
        d->dent = readdir(d->handle);
        if(d->dent == NULL)
        {
            return;
        }
        if(!fdu_isdot(d->dent->d_name))
        {
            memset(&it, 0, sizeof(it));
            if(!fdu_prepandvisit(d, &it, successfn, failfn, up))
            {
                return;
            }
            if(recurse && (it.type == FDU_T_Directory))
            {
                if(fdu_open(&subd, it.path))
                {
                    fdu_visit(&subd, successfn, failfn, recfailfn, recurse, up);
                    fdu_close(&subd);
                }
                else
                {
                    if(!recfailfn(&it, up))
                    {
                        return;
                    }
                }
            }
        }
    }
}

