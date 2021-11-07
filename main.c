
#include <stdint.h>
#include "fdu.h"

typedef struct options_t options_t;
typedef struct app_t app_t;

struct options_t
{
    bool summaryonly;
    bool readablesize;
};

struct app_t
{
    options_t opts;
    int64_t totalsize;
};

static size_t fprintl(FILE* fh, const char* str, size_t len)
{
    return fwrite(str, sizeof(char), len, fh);
}

static size_t fprint(FILE* fh, const char* str)
{
    return fprintl(fh, str, strlen(str));
}

char* readablesize(double size, char *buf, size_t maxbuf)
{
    int i;
    static const char* units[] = {"B", "K", "M", "G", "T", "P", "E", "Z", "Y"};
    i = 0;
    while(size > 1024)
    {
        size /= 1024;
        i++;
    }
    snprintf(buf, maxbuf, "%.*f%s", i, size, units[i]);
    return buf;
}


bool printitem(iteminfo_t* it, void* up)
{
    app_t* a;
    char szbuf[10 + 1];
    a = (app_t*)up;
    if(it->type == FDU_T_File)
    {
        a->totalsize += it->size;
        if(!a->opts.summaryonly)
        {
            if(a->opts.readablesize)
            {
                fprint(stdout, readablesize(it->size, szbuf, 10));
            }
            else
            {
                fprintf(stdout, "%ld", it->size);
            }
            fprint(stdout, "\t");
            fprint(stdout, it->path);
            fprint(stdout, "\n");
            fflush(stdout);
        }
    }
    return true;
}

bool failitem(iteminfo_t* it, void* up)
{
    (void)up;
    fprintf(stderr, "*ERROR* could not access '%s'\n", it->path);
    return true;
}

bool recfailitem(iteminfo_t* it, void* up)
{
    (void)up;
    fprintf(stderr, "*ERROR* opening directory '%s' during recursion\n", it->path);
    return true;
}

int main(int argc, char* argv[])
{
    int i;
    const char* dir;
    char tmpbuf[20 + 1];
    visitor_t d;
    app_t app;
    app.opts.summaryonly = false;
    app.opts.readablesize = true;
    app.totalsize = 0;
    if(argc > 1)
    {
        for(i=1; i<argc; i++)
        {
            if(argv[i][0] == '-')
            {
                switch(argv[i][1])
                {
                    case 's':
                        app.opts.summaryonly = true;
                        break;
                    case 'h':
                        app.opts.readablesize = true;
                        break;
                    default:
                        fprintf(stderr, "unrecognized option '-%c'\n", argv[i][1]);
                        return 1;
                        break;
                }
            }
            else
            {
                dir = argv[i];
                break;
            }
        }
        if(fdu_open(&d, dir))
        {
            fdu_visit(&d, printitem, failitem, recfailitem, true, &app);
            fdu_close(&d);
            if(app.opts.summaryonly)
            {
                fprintf(stdout, "%s\t%s\n", readablesize(app.totalsize, tmpbuf, 20), dir);
            }
        }
        else
        {
            fprintf(stderr, "could not open '%s'\n", dir);
            return 1;
        }
    }
    else
    {
        fprintf(stderr, "usage: %s <dir>\n", argv[0]);
    }
    return 0;
}

